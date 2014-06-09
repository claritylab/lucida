/*
* Copyright 1999-2002 Carnegie Mellon University.
* Portions Copyright 2002 Sun Microsystems, Inc.
* Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
* All Rights Reserved.  Use is subject to license terms.
*
* See the file "license.terms" for information on usage and
* redistribution of this file, and for a DISCLAIMER OF ALL
* WARRANTIES.
*
*/

package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import edu.cmu.sphinx.frontend.FloatData;
import edu.cmu.sphinx.util.LogMath;

import java.io.Serializable;
import java.util.Arrays;

/**
 * Defines the set of shared elements for a GaussianMixture. Since these elements are potentially
 * shared by a number of {@link GaussianMixture GaussianMixtures}, these elements should not be
 * written to. The GaussianMixture defines a single probability density function along with a set of
 * adaptation parameters.
 * <p/>
 * Note that all scores and weights are in LogMath log base
 */
// TODO: Since many of the subcomponents of a MixtureComponent are shared, are 
// there some potential opportunities to reduce the number of computations in scoring
// senones by sharing intermediate results for these subcomponents?
@SuppressWarnings("serial")
public class MixtureComponent implements Cloneable, Serializable {

    private float[] mean;
    /** Mean after transformed by the adaptation parameters. */
    private float[] meanTransformed;
    private float[][] meanTransformationMatrix;
    private float[] meanTransformationVector;
    private float[] variance;
    /** Precision is the inverse of the variance. This includes adaptation. */
    private float[] precisionTransformed;
    private float[][] varianceTransformationMatrix;
    private float[] varianceTransformationVector;

    private float distFloor;
    private float varianceFloor;

    public static final float DEFAULT_VAR_FLOOR = 0.0001f; // this also seems to be the default of SphinxTrain
    public static final float DEFAULT_DIST_FLOOR = 0.0f;

    private float logPreComputedGaussianFactor;
    private LogMath logMath;


    /**
     * Create a MixtureComponent with the given sub components.
     *
     * @param mean     the mean vector for this PDF
     * @param variance the variance for this PDF
     */
    public MixtureComponent(float[] mean, float[] variance) {
        this(mean, null, null, variance, null, null, DEFAULT_DIST_FLOOR, DEFAULT_VAR_FLOOR);
    }


    /**
     * Create a MixtureComponent with the given sub components.
     *
     * @param mean                         the mean vector for this PDF
     * @param meanTransformationMatrix     transformation matrix for this pdf
     * @param meanTransformationVector     transform vector for this PDF
     * @param variance                     the variance for this PDF
     * @param varianceTransformationMatrix var. transform matrix for this PDF
     * @param varianceTransformationVector var. transform vector for this PDF
     */
    public MixtureComponent(
            float[] mean,
            float[][] meanTransformationMatrix,
            float[] meanTransformationVector,
            float[] variance,
            float[][] varianceTransformationMatrix,
            float[] varianceTransformationVector) {
        this(mean, meanTransformationMatrix, meanTransformationVector, variance,
                varianceTransformationMatrix, varianceTransformationVector, DEFAULT_DIST_FLOOR, DEFAULT_VAR_FLOOR);
    }


    /**
     * Create a MixtureComponent with the given sub components.
     *
     * @param mean                         the mean vector for this PDF
     * @param meanTransformationMatrix     transformation matrix for this pdf
     * @param meanTransformationVector     transform vector for this PDF
     * @param variance                     the variance for this PDF
     * @param varianceTransformationMatrix var. transform matrix for this PDF
     * @param varianceTransformationVector var. transform vector for this PDF
     * @param distFloor                    the lowest score value (in linear domain)
     * @param varianceFloor                the lowest value for the variance
     */
    public MixtureComponent(
            float[] mean,
            float[][] meanTransformationMatrix,
            float[] meanTransformationVector,
            float[] variance,
            float[][] varianceTransformationMatrix,
            float[] varianceTransformationVector,
            float distFloor,
            float varianceFloor) {

        assert variance.length == mean.length;

        logMath = LogMath.getInstance();
        this.mean = mean;
        this.meanTransformationMatrix = meanTransformationMatrix;
        this.meanTransformationVector = meanTransformationVector;
        this.variance = variance;
        this.varianceTransformationMatrix = varianceTransformationMatrix;
        this.varianceTransformationVector = varianceTransformationVector;

        assert distFloor >= 0.0 : "distFloot seems to be already in log-domain";
        this.distFloor = logMath.linearToLog(distFloor);
        this.varianceFloor = varianceFloor;

        transformStats();

        logPreComputedGaussianFactor = precomputeDistance();
    }


    /**
     * Returns the mean for this component.
     *
     * @return the mean
     */
    public float[] getMean() {
        return mean;
    }


    /**
     * Returns the variance for this component.
     *
     * @return the variance
     */
    public float[] getVariance() {
        return variance;
    }


    /**
     * Calculate the score for this mixture against the given feature.
     * <p/>
     * Note: The support of <code>DoubleData</code>-features would require an array conversion to
     * float[]. Because getScore might be invoked with very high frequency, features are restricted
     * to be <code>FloatData</code>s.
     *
     * @param feature the feature to score
     * @return the score, in log, for the given feature
     */
    public float getScore(FloatData feature) {
        return getScore(feature.getValues());
    }


    /**
     * Calculate the score for this mixture against the given feature. We model the output
     * distributions using a mixture of Gaussians, therefore the current implementation is simply
     * the computation of a multi-dimensional Gaussian. <p/> <p><b>Normal(x) = exp{-0.5 * (x-m)' *
     * inv(Var) * (x-m)} / {sqrt((2 * PI) ^ N) * det(Var))}</b></p>
     * <p/>
     * where <b>x</b> and <b>m</b> are the incoming cepstra and mean vector respectively,
     * <b>Var</b> is the Covariance matrix, <b>det()</b> is the determinant of a matrix,
     * <b>inv()</b> is its inverse, <b>exp</b> is the exponential operator, <b>x'</b> is the
     * transposed vector of <b>x</b> and <b>N</b> is the dimension of the vectors <b>x</b> and
     * <b>m</b>.
     *
     * @param feature the feature to score
     * @return the score, in log, for the given feature
     */
    public float getScore(float[] feature) {
        // float logVal = 0.0f;
        float logDval = 0.0f;

        // First, compute the argument of the exponential function in
        // the definition of the Gaussian, then convert it to the
        // appropriate base. If the log base is <code>Math.E</code>,
        // then no operation is necessary.

        for (int i = 0; i < feature.length; i++) {
            float logDiff = feature[i] - meanTransformed[i];
            logDval += logDiff * logDiff * precisionTransformed[i];
        }
        // logDval = -logVal / 2;

        // At this point, we have the ln() of what we need, that is,
        // the argument of the exponential in the javadoc comment.

        // Convert to the appropriate base.
        logDval = logMath.lnToLog(logDval);

        // Add the precomputed factor, with the appropriate sign.
        logDval -= logPreComputedGaussianFactor;

        // System.out.println("MC: getscore " + logDval);

        // TODO: Need to use mean and variance transforms here

        if (Float.isNaN(logDval)) {
            System.out.println("gs is Nan, converting to 0");
            logDval = LogMath.LOG_ZERO;
        }

        if (logDval < distFloor) {
            logDval = distFloor;
        }

        return logDval;
    }


    /**
     * Pre-compute factors for the Mahalanobis distance. Some of the Mahalanobis distance
     * computation can be carried out in advance. Specifically, the factor containing only variance
     * in the Gaussian can be computed in advance, keeping in mind that the the determinant of the
     * covariance matrix, for the degenerate case of a mixture with independent components - only
     * the diagonal elements are non-zero - is simply the product of the diagonal elements. <p/>
     * We're computing the expression: <p/> <p><b>{sqrt((2 * PI) ^ N) * det(Var))}</b></p>
     *
     * @return the precomputed distance
     */
    public float precomputeDistance() {
        float logPreComputedGaussianFactor = 0.0f; // = log(1.0)
        // Compute the product of the elements in the Covariance
        // matrix's main diagonal. Covariance matrix is assumed
        // diagonal - independent dimensions. In log, the product
        // becomes a summation.
        for (int i = 0; i < variance.length; i++) {
            logPreComputedGaussianFactor +=
                    logMath.linearToLog(precisionTransformed[i] * -2);
            //	     variance[i] = 1.0f / (variance[i] * 2.0f);
        }

        // We need the minus sign since we computed
        // logPreComputedGaussianFactor based on precision, which is
        // the inverse of the variance. Therefore, in the log domain,
        // the two quantities have opposite signs.

        // The covariance matrix's dimension becomes a multiplicative
        // factor in log scale.
        logPreComputedGaussianFactor =
                logMath.linearToLog(2.0 * Math.PI) * variance.length
                        - logPreComputedGaussianFactor;

        // The sqrt above is a 0.5 multiplicative factor in log scale.
        return logPreComputedGaussianFactor * 0.5f;
    }


    /** Applies transformations to means and variances. */
    public void transformStats() {
        int featDim = mean.length;
        /*
        * The transformed mean vector is given by:
        *
        * <p><b>M = A * m + B</b></p>
        *
        * where <b>M</b> and <b>m</b> are the mean vector after and
        * before transformation, respectively, and <b>A</b> and
        * <b>B</b> are the transformation matrix and vector,
        * respectively.
        *
        * if A or B are <code>null</code> the according substeps are skipped
        */
        if (meanTransformationMatrix != null) {
            meanTransformed = new float[featDim];
            for (int i = 0; i < featDim; i++)
                for (int j = 0; j < featDim; j++)
                    meanTransformed[i] += mean[j] * meanTransformationMatrix[i][j];
        } else {
            meanTransformed = mean;
        }

        if (meanTransformationVector != null)
            for (int k = 0; k < featDim; k++)
                meanTransformed[k] += meanTransformationVector[k];

        /**
         * We do analogously with the variance. In this case, we also
         * invert the variance, and work with precision instead of
         * variance.
         */
        if (varianceTransformationMatrix != null) {
            precisionTransformed = new float[variance.length];
            for (int i = 0; i < featDim; i++)
                for (int j = 0; j < featDim; j++)
                    precisionTransformed[i] += variance[j] * varianceTransformationMatrix[i][j];
        } else
            precisionTransformed = variance.clone();

        if (varianceTransformationVector != null)
            for (int k = 0; k < featDim; k++)
                precisionTransformed[k] += varianceTransformationVector[k];

        for (int k = 0; k < featDim; k++) {
            float flooredPrecision = (precisionTransformed[k] < varianceFloor ? varianceFloor : precisionTransformed[k]);
            precisionTransformed[k] = 1.0f / (-2.0f * flooredPrecision);
        }
    }

    @Override
    public MixtureComponent clone() throws CloneNotSupportedException {
        MixtureComponent mixComp = (MixtureComponent)super.clone();

        mixComp.distFloor = distFloor;
        mixComp.varianceFloor = varianceFloor;
        mixComp.logPreComputedGaussianFactor = logPreComputedGaussianFactor;

        mixComp.mean = this.mean != null ? this.mean.clone() : null;
        if (meanTransformationMatrix != null) {
            mixComp.meanTransformationMatrix = this.meanTransformationMatrix.clone();
            for (int i = 0; i < meanTransformationMatrix.length; i++)
                mixComp.meanTransformationMatrix[i] = meanTransformationMatrix[i].clone();
        }
        mixComp.meanTransformationVector = this.meanTransformationVector != null ?
                this.meanTransformationVector.clone() : null;
        mixComp.meanTransformed = this.meanTransformed != null ? this.meanTransformed.clone() : null;

        mixComp.variance = this.variance != null ? this.variance.clone() : null;
        if (varianceTransformationMatrix != null) {
            mixComp.varianceTransformationMatrix = this.varianceTransformationMatrix.clone();
            for (int i = 0; i < varianceTransformationMatrix.length; i++)
                mixComp.varianceTransformationMatrix[i] = varianceTransformationMatrix[i].clone();
        }
        mixComp.varianceTransformationVector = this.varianceTransformationVector != null ?
                this.varianceTransformationVector.clone() : null;
        mixComp.precisionTransformed = this.precisionTransformed != null ?
                this.precisionTransformed.clone() : null;

        return mixComp;
    }


    @Override
    public String toString() {
        return "mu=" + Arrays.toString(mean) + " cov=" + Arrays.toString(variance);
    }

    public float[] getMeanTransformed() {
        return meanTransformed;
    }

    public float[] getPrecisionTransformed() {
        return precisionTransformed;
    }

    public float getLogPreComputedGaussianFactor() {
        return logPreComputedGaussianFactor;
    }
}


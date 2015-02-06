/**
 * Contains Linear Prediction Coefficient functions.
 *
 * @author <a href="mailto:rsingh@cs.cmu.edu">rsingh</a>
 * @version 1.0
 */

package edu.cmu.sphinx.frontend.frequencywarp;

import java.util.Arrays;

/**
 * Computes the linear predictive model using the Levinson-Durbin algorithm. Linear prediction assumes that a signal can
 * be model as a linear combination of previous samples, that is, the current sample x[i] can be modeled as:
 * <p/>
 * <pre> x[i] = a[0] + a[1] * x[i - 1] + a[2] * x[i - 2] + ... </pre>
 * <p/>
 * The summation on the right hand side of the equation involves a finite number of terms. The number of previous
 * samples used is the order of the linear prediction.
 * <p/>
 * This class also provides a method to compute LPC cepstra, that is, the cepstra computed from LPC coefficients, as
 * well as a method to compute the bilinear transformation of the LPC
 */
public class LinearPredictor {

    private int order;
    private int cepstrumOrder;
    private double[] reflectionCoeffs;
    private double[] ARParameters;
    private double alpha;
    private double[] cepstra;
    private final double[] bilinearCepstra;


    /**
     * Constructs a LinearPredictor with the given order.
     *
     * @param order the order of the LinearPredictor
     */
    public LinearPredictor(int order) {
        this.order = order;

        // Set the rest to null values
        reflectionCoeffs = null;
        ARParameters = null;
        alpha = 0;
        cepstra = null;
        bilinearCepstra = null;
    }


    /**
     * Method to compute Linear Prediction Coefficients for a frame of speech. Assumes the following 
     * sign convention:<br> prediction(x[t]) = Sum_i {Ar[i] * x[t-i]}
     *
     * @param autocor
     * @return the energy of the frame (alpha in the Levinson recursion)
     */
    public double[] getARFilter(double[] autocor) {
        /* No signal */
        if (autocor[0] == 0) {
            return null;
        }
        reflectionCoeffs = new double[order + 1];
        ARParameters = new double[order + 1];
        double[] backwardPredictor = new double[order + 1];

        alpha = autocor[0];
        reflectionCoeffs[1] = -autocor[1] / autocor[0];
        ARParameters[0] = 1.0;
        ARParameters[1] = reflectionCoeffs[1];
        alpha *= (1 - reflectionCoeffs[1] * reflectionCoeffs[1]);

        for (int i = 2; i <= order; i++) {
            for (int j = 1; j < i; j++) {
                backwardPredictor[j] = ARParameters[i - j];
            }
            reflectionCoeffs[i] = 0;
            for (int j = 0; j < i; j++) {
                reflectionCoeffs[i] -= ARParameters[j] * autocor[i - j];
            }
            reflectionCoeffs[i] /= alpha;

            for (int j = 1; j < i; j++) {
                ARParameters[j] += reflectionCoeffs[i] * backwardPredictor[j];
            }
            ARParameters[i] = reflectionCoeffs[i];
            alpha *= (1 - reflectionCoeffs[i] * reflectionCoeffs[i]);
            if (alpha <= 0.0) {
                return null;
            }
        }
        return ARParameters;
    }


    /**
     * Computes AR parameters from a given set of reflection coefficients.
     *
     * @param RC       double array of reflection coefficients. The RC array must begin at 1 (RC[0] is a dummy value)
     * @param lpcorder AR order desired
     * @return AR parameters
     */
    public double[] reflectionCoeffsToARParameters(double[] RC, int lpcorder) {
        double[][] tmp = new double[lpcorder + 1][lpcorder + 1];

        order = lpcorder;
        reflectionCoeffs = RC.clone();

        for (int i = 1; i <= lpcorder; i++) {
            for (int m = 1; m < i; m++) {
                tmp[i][m] = tmp[i - 1][m] - RC[i] * tmp[i - 1][i - m];
            }
            tmp[i][i] = RC[i];
        }
        ARParameters[0] = 1;
        for (int m = 1; m <= lpcorder; m++) {
            ARParameters[m] = tmp[m][m];
        }
        return ARParameters;
    }


    /**
     * Computes LPC Cepstra from the AR predictor parameters and alpha using a recursion invented by Oppenheim et al.
     * The literature shows the optimal value of cepstral order to be:
     * <p/>
     * <pre>0.75 * LPCorder <= ceporder <= 1.25 * LPCorder</pre>
     *
     * @param ceporder is the order of the LPC cepstral vector to be computed.
     * @return LPC cepstra
     */
    public double[] getData(int ceporder) {
        int i;
        double sum;

        if (ceporder <= 0) {
            return null;
        }

        cepstrumOrder = ceporder;
        cepstra = new double[cepstrumOrder];

        cepstra[0] = Math.log(alpha);
        if (cepstrumOrder == 1) {
            return cepstra;
        }

        cepstra[1] = -ARParameters[1];

        for (i = 2; i < Math.min(cepstrumOrder, order + 1); i++) {
            sum = i * ARParameters[i];
            for (int j = 1; j < i; j++) {
                sum += ARParameters[j] * cepstra[i - j] * (i - j);
            }
            cepstra[i] = -sum / i;
        }
        for (; i < cepstrumOrder; i++) { // Only if cepstrumOrder > order+1
            sum = 0;
            for (int j = 1; j <= order; j++) {
                sum += ARParameters[j] * cepstra[i - j] * (i - j);
            }
            cepstra[i] = -sum / i;
        }
        return cepstra;
    }


    /**
     * Computes a bi-linear frequency warped version of the LPC cepstrum from the LPC cepstrum. The recursive algorithm
     * used is defined in Oppenheim's paper in Proceedings of IEEE, June 1972 The program has been written using g[x,y]
     * = g_o[x,-y] where g_o is the array used by Oppenheim. To handle the reversed array index the recursion has been
     * done DOWN the array index.
     *
     * @param warp          is the warping coefficient. For 16KHz speech 0.6 is good valued.
     * @param nbilincepstra is the number of bilinear cepstral values to be computed from the linear frequency
     *                      cepstrum.
     * @return a bi-linear frequency warped version of the LPC cepstrum
     */
    public double[] getBilinearCepstra(double warp, int nbilincepstra) {
        double[][] g = new double[nbilincepstra][cepstrumOrder];

        // Make a local copy as this gets destroyed
        double[] lincep = Arrays.copyOf(cepstra, cepstrumOrder);

        bilinearCepstra[0] = lincep[0];
        lincep[0] = 0;
        g[0][cepstrumOrder - 1] = lincep[cepstrumOrder - 1];
        for (int i = 1; i < nbilincepstra; i++) {
            g[i][cepstrumOrder - 1] = 0;
        }

        for (int i = cepstrumOrder - 2; i >= 0; i--) {
            g[0][i] = warp * g[0][i + 1] + lincep[i];
            g[1][i] = (1 - warp * warp) * g[0][i + 1] + warp * g[1][i + 1];
            for (int j = 2; j < nbilincepstra; j++) {
                g[j][i] = warp * (g[j][i + 1] - g[j - 1][i]) + g[j - 1][i + 1];
            }
        }

        for (int i = 1; i <= nbilincepstra; i++) {
            bilinearCepstra[i] = g[i][0];
        }

        return bilinearCepstra;
    }
}

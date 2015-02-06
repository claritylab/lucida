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
package edu.cmu.sphinx.frontend.feature;

import edu.cmu.sphinx.frontend.*;

/**
 * Computes the delta and double delta of input cepstrum (or plp or ...). The delta is the first order derivative and
 * the double delta (a.k.a. delta delta) is the second order derivative of the original cepstrum. They help model the
 * speech signal dynamics. The output data is a {@link FloatData} object with a float array of size three times the
 * original cepstrum, formed by the concatenation of cepstra, delta cepstra, and double delta cepstra. The output is the
 * feature vector used by the decoder. Figure 1 shows the arrangement of the output feature data array:
 * <p/>
 * <img src="doc-files/feature.jpg"> <br> <b>Figure 1: Layout of the returned features. </b>
 * <p/>
 * Suppose that the original cepstrum has a length of N, the first N elements of the feature are just the original
 * cepstrum, the second N elements are the delta of the cepstrum, and the last N elements are the double delta of the
 * cepstrum.
 * <p/>
 * Figure 2 below shows pictorially the computation of the delta and double delta of a cepstrum vector, using the last 3
 * cepstra and the next 3 cepstra. <img src="doc-files/deltas.jpg"> <br> <b>Figure 2: Delta and double delta vector
 * computation. </b>
 * <p/>
 * Referring to Figure 2, the delta is computed by subtracting the cepstrum that is two frames behind of the current
 * cepstrum from the cepstrum that is two frames ahead of the current cepstrum. The computation of the double delta is
 * similar. It is computed by subtracting the delta cepstrum one time frame behind from the delta cepstrum one time
 * frame ahead. Replacing delta cepstra with cepstra, this works out to a formula involving the cepstra that are one and
 * three behind and after the current cepstrum.
 */
public class DeltasFeatureExtractor extends AbstractFeatureExtractor {

    /**
     *
     * @param window
     */
    public DeltasFeatureExtractor( int window ) {
        super(window);
    }

    public DeltasFeatureExtractor( ) {
    }
    
    /**
     * Computes the next feature. Advances the pointers as well.
     *
     * @return the feature Data computed
     */
    @Override
    protected Data computeNextFeature() {

    	int jp1 = (currentPosition - 1 + cepstraBufferSize) % cepstraBufferSize;
        int jp2 = (currentPosition - 2 + cepstraBufferSize) % cepstraBufferSize;
        int jp3 = (currentPosition - 3 + cepstraBufferSize) % cepstraBufferSize;
        int jf1 = (currentPosition + 1) % cepstraBufferSize;
        int jf2 = (currentPosition + 2) % cepstraBufferSize;
        int jf3 = (currentPosition + 3) % cepstraBufferSize;
        
    	DoubleData currentCepstrum = cepstraBuffer[currentPosition];
        double[] mfc3f = cepstraBuffer[jf3].getValues();
        double[] mfc2f = cepstraBuffer[jf2].getValues();
        double[] mfc1f = cepstraBuffer[jf1].getValues();
        double[] current = currentCepstrum.getValues();
        double[] mfc1p = cepstraBuffer[jp1].getValues();
        double[] mfc2p = cepstraBuffer[jp2].getValues();
        double[] mfc3p = cepstraBuffer[jp3].getValues();
        float[] feature = new float[current.length * 3];

        currentPosition = (currentPosition + 1) % cepstraBufferSize;

        // CEP; copy all the cepstrum data
        int j = 0;
        for (double val : current) {
            feature[j++] = (float)val;
        }
        // System.arraycopy(current, 0, feature, 0, j);
        // DCEP: mfc[2] - mfc[-2]
        for (int k = 0; k < mfc2f.length; k++) {
            feature[j++] = (float) (mfc2f[k] - mfc2p[k]);
        }
        // D2CEP: (mfc[3] - mfc[-1]) - (mfc[1] - mfc[-3])
        for (int k = 0; k < mfc3f.length; k++) {
            feature[j++] = (float) ((mfc3f[k] - mfc1p[k]) - (mfc1f[k] - mfc3p[k]));
        }
        return (new FloatData(feature,
                currentCepstrum.getSampleRate(),
                currentCepstrum.getFirstSampleNumber()));
    }
}

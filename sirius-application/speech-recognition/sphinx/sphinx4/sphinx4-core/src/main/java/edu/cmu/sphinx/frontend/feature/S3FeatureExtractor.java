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
 * original cepstrum.
 * <p/>
 * <p/>
 * The format of the outputted feature is:
 * <p/>
 * 12 cepstra (c[1] through c[12]) <br>followed by delta cepstra (delta c[1] through delta c[12]) <br>followed by c[0],
 * delta c[0] <br>followed by delta delta c[0] through delta delta c[12] </p>
 */
public class S3FeatureExtractor extends AbstractFeatureExtractor {

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

        currentPosition = (currentPosition + 1)% cepstraBufferSize;

        // CEP; skip C[0]
        int j = 0;
        for (int k = 1; k < current.length; k++) {
            feature[j++] = (float) current[k];
        }

        // DCEP: mfc[2] - mfc[-2], skip DC[0]
        for (int k = 1; k < mfc2f.length; k++) {
            feature[j++] = (float) (mfc2f[k] - mfc2p[k]);
        }

        // POW: C0, DC0
        feature[j++] = (float) current[0];
        feature[j++] = (float) (mfc2f[0] - mfc2p[0]);

        // D2CEP: (mfc[3] - mfc[-1]) - (mfc[1] - mfc[-3])
        for (int k = 0; k < mfc3f.length; k++) {
            feature[j++] = (float)
                    ((mfc3f[k] - mfc1p[k]) - (mfc1f[k] - mfc3p[k]));
        }

        return (new FloatData(feature,
                currentCepstrum.getSampleRate(),
                currentCepstrum.getFirstSampleNumber()));
    }
}

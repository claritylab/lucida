/*
 * Copyright 2013 Carnegie Mellon University.  
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.frontend.feature;

import edu.cmu.sphinx.frontend.*;

public class KaldiDeltasFeatureExtractor extends AbstractFeatureExtractor {

    public KaldiDeltasFeatureExtractor( int window ) {
        super(window);
    }

    public KaldiDeltasFeatureExtractor( ) {
    }
    
    @Override
    protected Data computeNextFeature() {

    	int jp1 = (currentPosition - 1 + cepstraBufferSize) % cepstraBufferSize;
        int jp2 = (currentPosition - 2 + cepstraBufferSize) % cepstraBufferSize;
        int jp3 = (currentPosition - 3 + cepstraBufferSize) % cepstraBufferSize;
        int jp4 = (currentPosition - 4 + cepstraBufferSize) % cepstraBufferSize;
        int jf1 = (currentPosition + 1) % cepstraBufferSize;
        int jf2 = (currentPosition + 2) % cepstraBufferSize;
        int jf3 = (currentPosition + 3) % cepstraBufferSize;
        int jf4 = (currentPosition + 4) % cepstraBufferSize;
        
    	DoubleData currentCepstrum = cepstraBuffer[currentPosition];
        double[] mfc4f = cepstraBuffer[jf4].getValues();
        double[] mfc3f = cepstraBuffer[jf3].getValues();
        double[] mfc2f = cepstraBuffer[jf2].getValues();
        double[] mfc1f = cepstraBuffer[jf1].getValues();
        double[] current = currentCepstrum.getValues();
        double[] mfc1p = cepstraBuffer[jp1].getValues();
        double[] mfc2p = cepstraBuffer[jp2].getValues();
        double[] mfc3p = cepstraBuffer[jp3].getValues();
        double[] mfc4p = cepstraBuffer[jp4].getValues();
        float[] feature = new float[current.length * 3];

        currentPosition = (currentPosition + 1) % cepstraBufferSize;

        int j = 0;
        for (double val : current) {
            feature[j++] = (float)val;
        }
        for (int k = 0; k < mfc2f.length; k++) {
            feature[j++] = (float) (2 * mfc2f[k] + mfc1f[k] - mfc1p[k] - 2 * mfc2p[k]) / 10.0f;
        }
        
        for (int k = 0; k < mfc3f.length; k++) {
            feature[j++] = (float) ((4 * mfc4f[k] + 4 * mfc3f[k] + mfc2f[k] - 4 * mfc1f[k]) - 10 * current[k] +
        	    (4 * mfc4p[k] + 4 * mfc3p[k] + mfc2p[k] - 4 * mfc1p[k])) / 100.0f;
        }
        return (new FloatData(feature,
                currentCepstrum.getSampleRate(),
                currentCepstrum.getFirstSampleNumber()));
    }
}

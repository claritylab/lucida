/*
 * Copyright 2002-2009 Carnegie Mellon University.  
 * Copyright 2009 PC-NG Inc.
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
 * This component concatenate the cepstrum from the sequence of frames according to the window size.
 * It's not supposed to give high accuracy alone, but combined with LDA transform it can give the same
 * or even better results than conventional delta and delta-delta coefficients. The idea is that
 * delta-delta computation is also a matrix multiplication thus using automatically generated
 * with LDA/MLLT matrix we can gain better results.
 * The model for this feature extractor should be trained with SphinxTrain with 1s_c feature type and
 * with cepwin option enabled. Don't forget to set the window size accordingly.
 */
public class ConcatFeatureExtractor extends AbstractFeatureExtractor {

    /**
     *
     * @param window
     */
    public ConcatFeatureExtractor( int window ) {
        super(window);
    }

    public ConcatFeatureExtractor( ) {
    }

    /**
     * Computes the next feature. Advances the pointers as well.
     *
     * @return the feature Data computed
     */
    @Override
    protected Data computeNextFeature() {
        DoubleData currentCepstrum = cepstraBuffer[currentPosition];
        float[] feature = new float[(window * 2 + 1) * currentCepstrum.getValues().length];
        int j = 0;
        for (int k = -window; k <= window; k++) {
        	int position = (currentPosition + k + cepstraBufferSize) % cepstraBufferSize;
        	double[] buffer = cepstraBuffer[position].getValues();
            for (double val : buffer) {
                feature[j++] = (float)val;
            }
        }
        currentPosition = (currentPosition + 1) % cepstraBufferSize ;

        return (new FloatData(feature,
                currentCepstrum.getSampleRate(),
                currentCepstrum.getFirstSampleNumber()));
    }
}

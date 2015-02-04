/*
 * Copyright 2010 Carnegie Mellon University.  
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.frontend.endpoint;

import edu.cmu.sphinx.frontend.BaseDataProcessor;

/**
 * An abstract analyzer that signals about presense of speech in last processing frame.
 * This information is used in noise filtering components to estimate noise spectrum
 * for example.
 */
public abstract class AbstractVoiceActivityDetector extends BaseDataProcessor {

    /**
     * Returns the state of speech detected.
     *
     * @return if last processed data object was classified as speech.
     */
    public abstract boolean isSpeech();	
}

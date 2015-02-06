/*
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2002-2004 Sun Microsystems, Inc.  
 * Portions Copyright 2002-2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */


package edu.cmu.sphinx.frontend.endpoint;

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DoubleData;


/** A container for DoubleData class that indicates whether the contained DoubleData is speech or not. */
@SuppressWarnings("serial")
public class SpeechClassifiedData implements Data {

    private boolean isSpeech;
    private final DoubleData data;


    /**
     * Constructs a SpeechClassifiedData object.
     *
     * @param doubleData the DoubleData
     * @param isSpeech   indicates whether the DoubleData is speech
     */
    public SpeechClassifiedData(DoubleData doubleData, boolean isSpeech) {
        this.data = doubleData;
        this.isSpeech = isSpeech;
    }


    /**
     * Sets whether this SpeechClassifiedData is speech or not.
     *
     * @param isSpeech true if this is speech, false otherwise
     */
    public void setSpeech(boolean isSpeech) {
        this.isSpeech = isSpeech;
    }


    /**
     * Returns whether this is classified as speech.
     *
     * @return true if this is classified as speech, false otherwise
     */
    public boolean isSpeech() {
        return isSpeech;
    }


    /**
     * Returns the data values.
     *
     * @return the data values
     */
    public double[] getValues() {
        return data.getValues();
    }


    /**
     * Returns the sample rate of the data.
     *
     * @return the sample rate of the data
     */
    public int getSampleRate() {
        return data.getSampleRate();
    }


    /**
     * Returns the time in milliseconds at which the audio data is collected.
     *
     * @return the difference, in milliseconds, between the time the audio data is collected and midnight, January 1,
     *         1970
     */
    public long getCollectTime() {
        return data.getCollectTime();
    }


    /**
     * Returns the position of the first sample in the original data. The very first sample number is zero.
     *
     * @return the position of the first sample in the original data
     */
    public long getFirstSampleNumber() {
        return data.getFirstSampleNumber();
    }


    /**
     * Returns the DoubleData contained by this SpeechClassifiedData.
     *
     * @return the DoubleData contained by this SpeechClassifiedData
     */
    public DoubleData getDoubleData() {
        return data;
    }
    

    /**
     * @return a string that describes the data.
     */
    @Override
    public String toString() {
        return "SpeechClassifiedData containing " + data.toString() + " classified as " + (isSpeech ? "speech" : "non-speech");
    }
}

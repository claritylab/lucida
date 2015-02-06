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


package edu.cmu.sphinx.frontend;

/**
 * A signal that indicates the start of data.
 *
 * @see Data
 * @see DataProcessor
 * @see Signal
 */
@SuppressWarnings("serial")
public class DataStartSignal extends Signal {

    private final int sampleRate;
    /**
     * A constant that is attached to all DataStartSignal passing this component. This allows subsequent
     * <code>DataProcessor</code>s (like the <code>Scorer</code>) to adapt their processing behavior.
     */
    public static final String SPEECH_TAGGED_FEATURE_STREAM = "vadTaggedFeatureStream";


    /**
     * Constructs a DataStartSignal.
     *
     * @param sampleRate The sampling rate of the started data stream.
     */
    public DataStartSignal(int sampleRate) {
        this(sampleRate, System.currentTimeMillis());
    }


    /**
     * Constructs a DataStartSignal at the given time.
     *
     * @param sampleRate the sampling rate of the started data stream.
     * @param time       the time this DataStartSignal is created
     */
    public DataStartSignal(int sampleRate, long time) {
        this(sampleRate, time, false);
    }


    /**
     * Constructs a DataStartSignal at the given time.
     *
     * @param sampleRate  the sampling rate of the started data stream.
     * @param tagAsVadStream <code>true</code> if this feature stream will contain vad-signals
     */
    public DataStartSignal(int sampleRate, boolean tagAsVadStream) {
        this(sampleRate, System.currentTimeMillis(), tagAsVadStream);
    }


    /**
     * Constructs a DataStartSignal at the given time.
     *
     * @param sampleRate  the sampling rate of the started data stream.
     * @param time        the time this DataStartSignal is created
     * @param tagAsVadStream <code>true</code> if this feature stream will contain vad-signals
     */
    public DataStartSignal(int sampleRate, long time, boolean tagAsVadStream) {
        super(time);
        this.sampleRate = sampleRate;

        if (tagAsVadStream) {
            this.getProps().put(SPEECH_TAGGED_FEATURE_STREAM, tagAsVadStream);
        }
    }


    /**
     * Returns the string "DataStartSignal".
     *
     * @return the string "DataStartSignal"
     */
    @Override
    public String toString() {
        return "DataStartSignal: creation time: " + getTime();
    }


    /** @return the sampling rate of the started data stream. */
    public int getSampleRate() {
        return sampleRate;
    }



    public static void tagAsVadStream(DataStartSignal dsSignal) {
        dsSignal.getProps().put(DataStartSignal.SPEECH_TAGGED_FEATURE_STREAM, true);
    }


    public static void untagAsVadStream(DataStartSignal dsSignal) {
        dsSignal.getProps().remove(DataStartSignal.SPEECH_TAGGED_FEATURE_STREAM);
    }
}

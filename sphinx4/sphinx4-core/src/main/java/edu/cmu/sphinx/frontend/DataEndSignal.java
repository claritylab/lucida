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
 * A signal that indicates the end of data.
 *
 * @see Data
 * @see DataProcessor
 * @see Signal
 */
@SuppressWarnings("serial")
public class DataEndSignal extends Signal {

    private final long duration;


    /**
     * Constructs a DataEndSignal.
     *
     * @param duration the duration of the entire data stream in milliseconds
     */
    public DataEndSignal(long duration) {
        this(duration, System.currentTimeMillis());
    }


    /**
     * Constructs a DataEndSignal with the given creation time.
     *
     * @param duration the duration of the entire data stream in milliseconds
     * @param time     the creation time of the DataEndSignal
     */
    public DataEndSignal(long duration, long time) {
        super(time);
        this.duration = duration;
    }


    /**
     * Returns the duration of the entire data stream in milliseconds
     *
     * @return the duration of the entire data stream in milliseconds
     */
    public long getDuration() {
        return duration;
    }


    /**
     * Returns the string "DataEndSignal".
     *
     * @return the string "DataEndSignal"
     */
    @Override
    public String toString() {
        return ("DataEndSignal: creation time: " + getTime() + ", duration: " +
                getDuration() + "ms");
    }
}

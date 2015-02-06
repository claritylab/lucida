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


package edu.cmu.sphinx.frontend.endpoint;

import edu.cmu.sphinx.frontend.Signal;

/** A signal that indicates the end of speech. */
@SuppressWarnings("serial")
public class SpeechEndSignal extends Signal {

    /** Constructs a SpeechEndSignal. */
    public SpeechEndSignal() {
        this(System.currentTimeMillis());
    }


    /**
     * Constructs a SpeechEndSignal with the given creation time.
     *
     * @param time the creation time of the SpeechEndSignal
     */
    public SpeechEndSignal(long time) {
        super(time);
    }


    /**
     * Returns the string "SpeechEndSignal".
     *
     * @return the string "SpeechEndSignal"
     */
    @Override
    public String toString() {
        return "SpeechEndSignal";
    }
}

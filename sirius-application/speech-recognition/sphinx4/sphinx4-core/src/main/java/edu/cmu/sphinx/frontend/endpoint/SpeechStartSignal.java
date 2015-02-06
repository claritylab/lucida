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

/** A signal that indicates the start of speech. */
@SuppressWarnings("serial")
public class SpeechStartSignal extends Signal {

    /** Constructs a SpeechStartSignal. */
    public SpeechStartSignal() {
        this(System.currentTimeMillis());
    }


    /**
     * Constructs a SpeechStartSignal at the given time.
     *
     * @param time the time this SpeechStartSignal is created
     */
    public SpeechStartSignal(long time) {
        super(time);
    }


    /**
     * Returns the string "SpeechStartSignal".
     *
     * @return the string "SpeechStartSignal"
     */
    @Override
    public String toString() {
        return "SpeechStartSignal";
    }
}

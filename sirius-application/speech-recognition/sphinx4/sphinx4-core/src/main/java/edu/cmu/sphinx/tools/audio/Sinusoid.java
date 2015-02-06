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

package edu.cmu.sphinx.tools.audio;


/** A 16bit, linear SIGNED_PCM, little endian, sinusoid with a sample rate of 8kHz. */
public class Sinusoid extends AudioData {

    /**
     * Creates a 16bit, linear SIGNED_PCM, little endian, sinusoid with a sample rate of 8kHz.
     *
     * @param frequency cycles per second
     * @param phase     phase offset in radians
     * @param amplitude amplitude
     * @param duration  duration in seconds
     */
    public Sinusoid(double frequency,
                    double phase,
                    double amplitude,
                    double duration) {
        this.shorts = new short[(int) (8000 * duration)];
        double radiansPerSample = (frequency * 2.0 * Math.PI) / 8000.0;
        for (int i = 0; i < shorts.length; i++) {
            shorts[i] = (short) (
                    amplitude * Math.cos((radiansPerSample * i) + phase));
        }
    }
}

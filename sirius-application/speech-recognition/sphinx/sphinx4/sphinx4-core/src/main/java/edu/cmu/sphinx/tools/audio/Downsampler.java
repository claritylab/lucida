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

import java.util.Arrays;

/** Downsamples an audio clip. */
public class Downsampler {

    /**
     * Downsamples the given audio clip from the given input sample rate to the given output sample rate.
     *
     * @param inSamples the clip to down sample - one sample per element
     * @param srIn      the sample rate of the given clip
     * @param srOut     the sample to downsample to
     * @return an array of downsampled samples
     */
    public static short[] downsample(short[] inSamples,
                                     int srIn,
                                     int srOut) {

        /* [[[WDW - this was very back of the napkin for me.  The main
         * idea was to break a series of input samples into chunks
         * and have each sample in the chunk contribute to the average
         * value.  It's brute force, but I didn't have time to think
         * of anything less or more grand.]]]
         */
        short[] temp = new short[inSamples.length];
        int inSampleIndex = -1;
        int outSampleIndex = 0;
        int k = srOut;
        boolean done = false;
        while (!done) {
            int sum = 0;
            for (int i = 0; i < srIn; i++) {
                if (k == srOut) {
                    inSampleIndex++;
                    if (inSampleIndex >= inSamples.length) {
                        done = true;
                        break;
                    }
                    k = 0;
                }
                sum += inSamples[inSampleIndex];
                k++;
            }
            temp[outSampleIndex++] = (short) (sum / srIn);
        }
        return Arrays.copyOf(temp, outSampleIndex);
    }
}

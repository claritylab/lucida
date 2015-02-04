/*
 * Copyright 1999-2002 Carnegie Mellon University.
 * Portions Copyright 2002 Sun * Microsystems, Inc.
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 *
 * All Rights Reserved. Use is subject to license terms.
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */

package edu.cmu.sphinx.frontend.frequencywarp;

import static java.util.Arrays.copyOfRange;

/**
 * Defines a triangular mel-filter.
 *
 * The {@link MelFrequencyFilterBank2} creates mel-filters and filters spectrum
 * data using the method {@link #filterOutput(double[]) filterOutput}.
 *
 * A mel-filter is a triangular shaped bandpass filter. When a mel-filter is
 * constructed, the parameters <code>leftEdge</code>, <code>rightEdge</code>,
 * <code>centerFreq</code>, <code>initialFreq</code>, and
 * <code>deltaFreq</code> are given to the {@link MelFilter2 Constructor}. The
 * first three arguments to the constructor, i.e. <code>leftEdge</code>,
 * <code>rightEdge</code>, and <code>centerFreq</code>, specify the filter's
 * slopes. The total area under the filter is 1. The filter is shaped as a
 * triangle. Knowing the distance between the center frequency and each of the
 * edges, it is easy to compute the slopes of the two sides in the triangle -
 * the third side being the frequency axis. The last two arguments,
 * <code>initialFreq</code> and <code>deltaFreq</code>, identify the first
 * frequency bin that falls inside this filter and the spacing between
 * successive frequency bins. All frequencies here are considered in a linear
 * scale.
 *
 * Figure 1 below shows pictorially what the other parameters mean.
 *
 * <img src="doc-files/melfilter.jpg"> <br>
 * <center><b>Figure 1: A triangular mel-filter.</b></center>
 * 
 * @see MelFrequencyFilterBank2
 */
public class MelFilter2 {

    private final int offset;
    private final double[] weights;

    public MelFilter2(double center, double delta, double[] melPoints) {
        int lastIndex = 0;
        int firstIndex = melPoints.length;
        double left = center - delta;
        double right = center + delta;
        double [] heights = new double[melPoints.length];

        for (int i = 0; i < heights.length; ++i) {
            if (left < melPoints[i] && melPoints[i] <= center) {
                heights[i] = (melPoints[i] - left) / (center - left);
                firstIndex = Math.min(i, firstIndex);
                lastIndex = i;
            }

            if (center < melPoints[i] && melPoints[i] < right) {
                heights[i] = (right - melPoints[i]) / (right - center);
                lastIndex = i;
            }
        }

        offset = firstIndex;
        weights = copyOfRange(heights, firstIndex, lastIndex + 1);
    }

    public double apply(double[] powerSpectrum) {
        double result = 0;
        for (int i = 0; i < weights.length; ++i)
            result += weights[i] * powerSpectrum[offset + i];

        return result;
    }
}

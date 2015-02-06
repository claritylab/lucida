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


package edu.cmu.sphinx.frontend.frequencywarp;


/**
 * Defines a triangular mel-filter. The {@link edu.cmu.sphinx.frontend.frequencywarp.MelFrequencyFilterBank} creates
 * mel-filters and filters spectrum data using the method {@link #filterOutput(double[]) filterOutput}.
 * <p/>
 * A mel-filter is a triangular shaped bandpass filter.  When a mel-filter is constructed, the parameters
 * <code>leftEdge</code>, <code>rightEdge</code>, <code>centerFreq</code>, <code>initialFreq</code>, and
 * <code>deltaFreq</code> are given to the {@link MelFilter Constructor}. The first three arguments to the constructor,
 * i.e. <code>leftEdge</code>, <code>rightEdge</code>, and <code>centerFreq</code>, specify the filter's slopes. The
 * total area under the filter is 1. The filter is shaped as a triangle. Knowing the distance between the center
 * frequency and each of the edges, it is easy to compute the slopes of the two sides in the triangle - the third side
 * being the frequency axis. The last two arguments, <code>initialFreq</code> and <code>deltaFreq</code>, identify the
 * first frequency bin that falls inside this filter and the spacing between successive frequency bins. All frequencies
 * here are considered in a linear scale.
 * <p/>
 * Figure 1 below shows pictorially what the other parameters mean.
 * <p/>
 * <img src="doc-files/melfilter.jpg"> <br><center><b>Figure 1: A triangular mel-filter.</b></center>
 *
 * @see MelFrequencyFilterBank
 */
public class MelFilter {

    private double[] weight;

    private int initialFreqIndex;


    /**
     * Constructs a filter from the parameters.
     * <p/>
     * In the current implementation, the filter is a bandpass filter with a triangular shape.  We're given the left and
     * right edges and the center frequency, so we can determine the right and left slopes, which could be not only
     * asymmetric but completely different. We're also given the initial frequency, which may or may not coincide with
     * the left edge, and the frequency step.
     *
     * @param leftEdge    the filter's lowest passing frequency
     * @param centerFreq  the filter's center frequency
     * @param rightEdge   the filter's highest passing frequency
     * @param initialFreq the first frequency bin in the pass band
     * @param deltaFreq   the step in the frequency axis between frequency bins
     * @throws IllegalArgumentException
     */
    public MelFilter(double leftEdge,
                     double centerFreq,
                     double rightEdge,
                     double initialFreq,
                     double deltaFreq) throws IllegalArgumentException {
        double filterHeight;
        double leftSlope;
        double rightSlope;
        double currentFreq;
        int indexFilterWeight;
        int numberElementsWeightField;

        if (deltaFreq == 0) {
            throw new IllegalArgumentException("deltaFreq has zero value");
        }
        /**
         * Check if the left and right boundaries of the filter are
         * too close.
         */
        if ((Math.round(rightEdge - leftEdge) == 0)
                || (Math.round(centerFreq - leftEdge) == 0)
                || (Math.round(rightEdge - centerFreq) == 0)) {
            throw new IllegalArgumentException("Filter boundaries too close");
        }
        /**
         * Let's compute the number of elements we need in the
         * <code>weight</code> field by computing how many frequency
         * bins we can fit in the current frequency range.
         */
        numberElementsWeightField =
                (int) Math.round((rightEdge - leftEdge) / deltaFreq + 1);
        /**
         * Initialize the <code>weight</code> field.
         */
        if (numberElementsWeightField == 0) {
            throw new IllegalArgumentException("Number of elements in mel"
                    + " is zero.");
        }
        weight = new double[numberElementsWeightField];

        /**
         * Let's make the filter area equal to 1.
         */
        filterHeight = 2.0f / (rightEdge - leftEdge);

        /**
         * Now let's compute the slopes based on the height.
         */
        leftSlope = filterHeight / (centerFreq - leftEdge);
        rightSlope = filterHeight / (centerFreq - rightEdge);

        /**
         * Now let's compute the weight for each frequency bin.  We
         * initialize and update two variables in the <code>for</code>
         * line.
         */
        for (currentFreq = initialFreq, indexFilterWeight = 0;
             currentFreq <= rightEdge;
             currentFreq += deltaFreq, indexFilterWeight++) {
            /**
             * A straight line that contains point <b>(x0, y0)</b> and
             * has slope <b>m</b> is defined by:
             *
             * <b>y = y0 + m * (x - x0)</b>
             *
             * This is used for both "sides" of the triangular filter
             * below.
             */
            if (currentFreq < centerFreq) {
                weight[indexFilterWeight] = leftSlope
                        * (currentFreq - leftEdge);
            } else {
                weight[indexFilterWeight] = filterHeight + rightSlope
                        * (currentFreq - centerFreq);
            }
        }
        /**
         * Initializing frequency related fields.
         */
        this.initialFreqIndex = (int) Math.round
                (initialFreq / deltaFreq);
    }


    /**
     * Compute the output of a filter. We're given a power spectrum, to which we apply the appropriate weights.
     *
     * @param spectrum the input power spectrum to be filtered
     * @return the filtered value, in fact a weighted average of power in the frequency range of the filter pass band
     */
    public double filterOutput(double[] spectrum) {
        double output = 0.0f;
        int indexSpectrum;

        for (int i = 0; i < this.weight.length; i++) {
            indexSpectrum = this.initialFreqIndex + i;
            if (indexSpectrum < spectrum.length) {
                output += spectrum[indexSpectrum] * this.weight[i];
            }
        }
        return output;
    }
}

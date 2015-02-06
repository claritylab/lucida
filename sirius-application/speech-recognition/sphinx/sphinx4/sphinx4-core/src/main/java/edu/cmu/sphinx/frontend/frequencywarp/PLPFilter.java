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
 * Defines a filter used by the {@link PLPFrequencyFilterBank} class. The filter is defined by a function in the {@link
 * #PLPFilter Constructor}. A set of equally spaced frequencies in a linear scale is passed to the constructor, which
 * returns the weights for each of the frequency bins, such that the filter has the shape defined by this piecewise
 * function in the bark scale.
 *
 * @author <a href="mailto:rsingh@cs.cmu.edu">rsingh</a>
 * @version 1.0
 * @see PLPFrequencyFilterBank
 */
public class PLPFilter {

    private double[] filterCoefficients;
    private final int numDFTPoints;

    /** The center frequency of the filter in Hertz. */
    public final double centerFreqInHz;

    /** The center frequency of the filter in Bark. */
    public final double centerFreqInBark;


    /**
     * Constructs a PLP filter around a given center frequency.
     * <p/>
     * Defines a filter according to the following equation, defined piecewise (all frequencies in the equation are Bark
     * frequencies):
     * <p/>
     * <pre>
     * Filter(f) = 0 if f < -2.5 <br>
     *           = 10^(-(f+0.5)) if -2.5 <= f <= -0.5 <br>
     *           = 1  if -0.5 <= f <= 0.5 <br>
     *           = 10^(2.5(f-0.5)) if 0.5 <= f <= 1.3 <br>
     *           = 0 if f > 1.3 <br>
     * </pre>
     * The current implementation assumes that the calling routine passes in an array of frequencies, one for each of
     * the DFT points in the spectrum of the frame of speech to be filtered. This is used in conjunction with a
     * specified center frequency to determine the filter.
     *
     * @param DFTFrequenciesInHz is a double array containing the frequencies in Hertz corresponding to each of the DFT
     *                           points in the spectrum of the signal to be filtered.
     * @param centerFreqInHz     is the filter's center frequency
     * @throws IllegalArgumentException
     */


    public PLPFilter(double[] DFTFrequenciesInHz,
                     double centerFreqInHz) throws IllegalArgumentException {

        FrequencyWarper bark = new FrequencyWarper();

        numDFTPoints = DFTFrequenciesInHz.length;
        this.centerFreqInHz = centerFreqInHz;
        centerFreqInBark = bark.hertzToBark(centerFreqInHz);

        if (centerFreqInHz < DFTFrequenciesInHz[0] ||
                centerFreqInHz > DFTFrequenciesInHz[numDFTPoints - 1]) {
            throw new IllegalArgumentException
                    ("Center frequency for PLP filter out of range");
        }

        filterCoefficients = new double[numDFTPoints];

        for (int i = 0; i < numDFTPoints; i++) {
            double barkf;

            barkf = bark.hertzToBark(DFTFrequenciesInHz[i]) - centerFreqInBark;
            if (barkf < -2.5)
                filterCoefficients[i] = 0.0;
            else if (barkf <= -0.5)
                filterCoefficients[i] = Math.pow(10.0, barkf + 0.5);
            else if (barkf <= 0.5)
                filterCoefficients[i] = 1.0;
            else if (barkf <= 1.3)
                filterCoefficients[i] = Math.pow(10.0, -2.5 * (barkf - 0.5));
            else
                filterCoefficients[i] = 0.0;
        }
    }


    /**
     * Compute the PLP spectrum at the center frequency of this filter for a given power spectrum.
     *
     * @param spectrum the input power spectrum to be filtered
     * @return the PLP spectrum value
     * @throws IllegalArgumentException if the input spectrum is of a different length than the array of filter
     *                                  coefficients
     */
    public double filterOutput(double[] spectrum)
            throws IllegalArgumentException {

        if (spectrum.length != numDFTPoints) {
            throw new IllegalArgumentException
                    ("Mismatch in no. of DFT points " + spectrum.length +
                            " in spectrum and in filter " + numDFTPoints);
        }

        double output = 0.0;
        for (int i = 0; i < numDFTPoints; i++) {
            output += spectrum[i] * filterCoefficients[i];
        }
        return output;
    }
}

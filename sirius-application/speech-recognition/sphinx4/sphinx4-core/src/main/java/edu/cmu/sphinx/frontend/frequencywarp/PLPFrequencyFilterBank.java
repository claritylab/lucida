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

import edu.cmu.sphinx.frontend.BaseDataProcessor;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DataProcessingException;
import edu.cmu.sphinx.frontend.DoubleData;
import edu.cmu.sphinx.util.props.*;

/**
 * Filters an input power spectrum through a PLP filterbank. The filters in the filterbank are placed in the frequency
 * axis so as to mimic the critical band, representing different perceptual effect at different frequency bands. The
 * filter outputs are also scaled for equal loudness preemphasis. The filter shapes are defined by the {@link PLPFilter}
 * class. Like the {@link MelFrequencyFilterBank2}, this filter bank has characteristics defined by the {@link
 * #PROP_NUMBER_FILTERS number of filters}, the {@link #PROP_MIN_FREQ minimum frequency}, and the {@link #PROP_MAX_FREQ
 * maximum frequency}. Unlike the {@link MelFrequencyFilterBank2}, the minimum and maximum frequencies here refer to the
 * <b>center</b> frequencies of the filters located at the leftmost and rightmost positions, and not to the edges.
 * Therefore, this filter bank spans a frequency range that goes beyond the limits suggested by the minimum and maximum
 * frequencies.
 *
 * @author <a href="mailto:rsingh@cs.cmu.edu">rsingh</a>
 * @version 1.0
 * @see PLPFilter
 */
public class PLPFrequencyFilterBank extends BaseDataProcessor {

    /** The property for the number of filters in the filterbank. */
    @S4Integer(defaultValue = 32)
    public static final String PROP_NUMBER_FILTERS = "numberFilters";

    /** The property for the center frequency of the lowest filter in the filterbank. */
    @S4Double(defaultValue = 130.0)
    public static final String PROP_MIN_FREQ = "minimumFrequency";

    /** The property for the center frequency of the highest filter in the filterbank. */
    @S4Double(defaultValue = 3600.0)
    public static final String PROP_MAX_FREQ = "maximumFrequency";

    private int sampleRate;
    private int numberFftPoints;
    private int numberFilters;
    private double minFreq;
    private double maxFreq;
    private PLPFilter[] criticalBandFilter;
    private double[] equalLoudnessScaling;


    public PLPFrequencyFilterBank(double minFreq, double maxFreq, int numberFilters) {
        initLogger();
        this.minFreq = minFreq;
        this.maxFreq = maxFreq;
        this.numberFilters = numberFilters;
    }

    public PLPFrequencyFilterBank() {
    }
    
    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        minFreq = ps.getDouble(PROP_MIN_FREQ);
        maxFreq = ps.getDouble(PROP_MAX_FREQ);
        numberFilters = ps.getInt(PROP_NUMBER_FILTERS);
    }


    /** Initializes this PLPFrequencyFilterBank object */
    @Override
    public void initialize() {
        super.initialize();
    }


    /**
     * Build a PLP filterbank with the parameters given. The center frequencies of the PLP filters will be uniformly
     * spaced between the minimum and maximum analysis frequencies on the Bark scale. on the Bark scale.
     *
     * @throws IllegalArgumentException
     */
    private void buildCriticalBandFilterbank() throws IllegalArgumentException {
        double minBarkFreq;
        double maxBarkFreq;
        double deltaBarkFreq;
        double nyquistFreq;
        double centerFreq;
        int numberDFTPoints = (numberFftPoints >> 1) + 1;
        double[] DFTFrequencies;

        /* This is the same class of warper called by PLPFilter.java */
        FrequencyWarper bark = new FrequencyWarper();

        this.criticalBandFilter = new PLPFilter[numberFilters];

        if (numberFftPoints == 0) {
            throw new IllegalArgumentException("Number of FFT points is zero");
        }
        if (numberFilters < 1) {
            throw new IllegalArgumentException("Number of filters illegal: "
                    + numberFilters);
        }

        DFTFrequencies = new double[numberDFTPoints];
        nyquistFreq = sampleRate / 2;
        for (int i = 0; i < numberDFTPoints; i++) {
            DFTFrequencies[i] = i * nyquistFreq /
                    (numberDFTPoints - 1);
        }

        /**
         * Find center frequencies of filters in the Bark scale
         * translate to linear frequency and create PLP filters
         * with these center frequencies.
         *
         * Note that minFreq and maxFreq specify the CENTER FREQUENCIES
         * of the lowest and highest PLP filters
         */


        minBarkFreq = bark.hertzToBark(minFreq);
        maxBarkFreq = bark.hertzToBark(maxFreq);

        if (numberFilters < 1) {
            throw new IllegalArgumentException("Number of filters illegal: "
                    + numberFilters);
        }
        deltaBarkFreq = (maxBarkFreq - minBarkFreq) / (numberFilters + 1);

        for (int i = 0; i < numberFilters; i++) {
            centerFreq = bark.barkToHertz(minBarkFreq + i * deltaBarkFreq);
            criticalBandFilter[i] = new PLPFilter(DFTFrequencies, centerFreq);
        }
    }


    /**
     * This function return the equal loudness preemphasis factor at any frequency. The preemphasis function is given
     * by
     * <p/>
     * E(w) = f^4 / (f^2 + 1.6e5) ^ 2 * (f^2 + 1.44e6) / (f^2 + 9.61e6)
     * <p/>
     * This is more modern one from HTK, for some reason it's preferred over old variant, and 
     * it doesn't require conversion to radians
     * <p/>
     * E(w) = (w^2+56.8e6)*w^4/((w^2+6.3e6)^2(w^2+0.38e9)(w^6+9.58e26))
     * <p/>
     * where w is frequency in radians/second
     * @param freq
     */
    private double loudnessScalingFunction(double freq) {
        double fsq = freq * freq;
        double fsub = fsq / (fsq + 1.6e5);
        return fsub * fsub * ((fsq + 1.44e6) / (fsq + 9.61e6));
    }


    /** Create an array of equal loudness preemphasis scaling terms for all the filters */
    private void buildEqualLoudnessScalingFactors() {
        double centerFreq;

        equalLoudnessScaling = new double[numberFilters];
        for (int i = 0; i < numberFilters; i++) {
            centerFreq = criticalBandFilter[i].centerFreqInHz;
            equalLoudnessScaling[i] = loudnessScalingFunction(centerFreq);
        }
    }


    /**
     * Process data, creating the power spectrum from an input audio frame.
     *
     * @param input input power spectrum
     * @return PLP power spectrum
     * @throws java.lang.IllegalArgumentException
     *
     */
    private DoubleData process(DoubleData input) throws
            IllegalArgumentException {

        double[] in = input.getValues();

        if (criticalBandFilter == null ||
                sampleRate != input.getSampleRate()) {
            numberFftPoints = (in.length - 1) << 1;
            sampleRate = input.getSampleRate();
            buildCriticalBandFilterbank();
            buildEqualLoudnessScalingFactors();

        } else if (in.length != ((numberFftPoints >> 1) + 1)) {
            throw new IllegalArgumentException
                    ("Window size is incorrect: in.length == " + in.length +
                            ", numberFftPoints == " + ((numberFftPoints >> 1) + 1));
        }

        double[] outputPLPSpectralArray = new double[numberFilters];

        /**
         * Filter input power spectrum
         */
        for (int i = 0; i < numberFilters; i++) {
            // First compute critical band filter output
            outputPLPSpectralArray[i] = criticalBandFilter[i].filterOutput(in);
            // Then scale it for equal loudness preemphasis
            outputPLPSpectralArray[i] *= equalLoudnessScaling[i];
        }

        DoubleData output = new DoubleData
                (outputPLPSpectralArray, input.getSampleRate(),
                        input.getFirstSampleNumber());

        return output;
    }


    /**
     * Reads the next Data object, which is the power spectrum of an audio input frame. However, it can also be other
     * Data objects like a Signal, which is returned unmodified.
     *
     * @return the next available Data object, returns null if no Data object is available
     * @throws DataProcessingException if there is a data processing error
     */
    @Override
    public Data getData() throws DataProcessingException {

        Data input = getPredecessor().getData();

        getTimer().start();

        if (input != null) {
            if (input instanceof DoubleData) {
                input = process((DoubleData) input);
            }
        }

        getTimer().stop();

        return input;
    }
}

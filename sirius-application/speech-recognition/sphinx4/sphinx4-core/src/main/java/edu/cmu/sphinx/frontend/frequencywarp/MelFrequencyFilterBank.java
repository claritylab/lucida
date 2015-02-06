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
 * Filters an input power spectrum through a bank of number of mel-filters. The output is an array of filtered values,
 * typically called mel-spectrum, each corresponding to the result of filtering the input spectrum through an individual
 * filter. Therefore, the length of the output array is equal to the number of filters created.
 * <p/>
 * The triangular mel-filters in the filter bank are placed in the frequency axis so that each filter's center frequency
 * follows the mel scale, in such a way that the filter bank mimics the critical band, which represents different
 * perceptual effect at different frequency bands. Additionally, the edges are placed so that they coincide with the
 * center frequencies in adjacent filters. Pictorially, the filter bank looks like:
 * <p/>
 * <img src="doc-files/melfilterbank.jpg"> <br> <center><b>Figure 1: A Mel-filter bank. </b> </center>
 * <p/>
 * As you might notice in the above figure, the distance at the base from the center to the left edge is different from
 * the center to the right edge. Since the center frequencies follow the mel-frequency scale, which is a non-linear
 * scale that models the non-linear human hearing behavior, the mel filter bank corresponds to a warping of the
 * frequency axis. As can be inferred from the figure, filtering with the mel scale emphasizes the lower frequencies. A
 * common model for the relation between frequencies in mel and linear scales is as follows:
 * <p/>
 * <code>melFrequency = 2595 * log(1 + linearFrequency/700)</code>
 * <p/>
 * The constants that define the filterbank are the number of filters, the minimum frequency, and the maximum frequency.
 * The minimum and maximum frequencies determine the frequency range spanned by the filterbank. These frequencies depend
 * on the channel and the sampling frequency that you are using. For telephone speech, since the telephone channel
 * corresponds to a bandpass filter with cutoff frequencies of around 300Hz and 3700Hz, using limits wider than these
 * would waste bandwidth. For clean speech, the minimum frequency should be higher than about 100Hz, since there is no
 * speech information below it. Furthermore, by setting the minimum frequency above 50/60Hz, we get rid of the hum
 * resulting from the AC power, if present.
 * <p/>
 * The maximum frequency has to be lower than the Nyquist frequency, that is, half the sampling rate. Furthermore, there
 * is not much information above 6800Hz that can be used for improving separation between models. Particularly for very
 * noisy channels, maximum frequency of around 5000Hz may help cut off the noise.
 * <p/>
 * Typical values for the constants defining the filter bank are: <table width="80%" border="1"> <tr> <td><b>Sample rate
 * (Hz) </b></td> <td><b>16000 </b></td> <td><b>11025 </b></td> <td><b>8000 </b></td> </tr> <tr> <td>{@link
 * #PROP_NUMBER_FILTERS numberFilters}</td> <td>40</td> <td>36</td> <td>31</td> </tr> <tr> <td>{@link #PROP_MIN_FREQ
 * minimumFrequency}(Hz)</td> <td>130</td> <td>130</td> <td>200</td> </tr> <tr> <td>{@link #PROP_MAX_FREQ
 * maximumFrequency}(Hz)</td> <td>6800</td> <td>5400</td> <td>3500</td> </tr> </table>
 * <p/>
 * Davis and Mermelstein showed that Mel-frequency cepstral coefficients present robust characteristics that are good
 * for speech recognition. For details, see Davis and Mermelstein, <i>Comparison of Parametric Representations for
 * Monosyllable Word Recognition in Continuously Spoken Sentences, IEEE Transactions on Acoustic, Speech and Signal
 * Processing, 1980 </i>.
 *
 * @see MelFilter
 */
public class MelFrequencyFilterBank extends BaseDataProcessor {

    /** The property for the number of filters in the filterbank. */
    @S4Integer(defaultValue = 40)
    public static final String PROP_NUMBER_FILTERS = "numberFilters";

    /** The property for the minimum frequency covered by the filterbank. */
    @S4Double(defaultValue = 130.0)
    public static final String PROP_MIN_FREQ = "minimumFrequency";

    /** The property for the maximum frequency covered by the filterbank. */
    @S4Double(defaultValue = 6800.0)
    public static final String PROP_MAX_FREQ = "maximumFrequency";

    // ----------------------------------
    // Configuration data
    // ----------------------------------
    private int sampleRate;
    private int numberFftPoints;
    private int numberFilters;
    private double minFreq;
    private double maxFreq;
    private MelFilter[] filter;


    public MelFrequencyFilterBank(double minFreq, double maxFreq, int numberFilters) {
        initLogger();
        this.minFreq = minFreq;
        this.maxFreq = maxFreq;
        this.numberFilters = numberFilters;
    }

    public MelFrequencyFilterBank() {
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


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.frontend.DataProcessor#initialize(edu.cmu.sphinx.frontend.CommonConfig)
    */
    @Override
    public void initialize() {
        super.initialize();
    }


    /**
     * Compute mel frequency from linear frequency.
     * <p/>
     * Since we don't have <code>log10()</code>, we have to compute it using natural log: <b>log10(x) = ln(x) / ln(10)
     * </b>
     *
     * @param inputFreq the input frequency in linear scale
     * @return the frequency in a mel scale
     */
    private double linToMelFreq(double inputFreq) {
        return (2595.0 * (Math.log(1.0 + inputFreq / 700.0) / Math.log(10.0)));
    }


    /**
     * Compute linear frequency from mel frequency.
     *
     * @param inputFreq the input frequency in mel scale
     * @return the frequency in a linear scale
     */
    private double melToLinFreq(double inputFreq) {
        return (700.0 * (Math.pow(10.0, (inputFreq / 2595.0)) - 1.0));
    }


    /**
     * Sets the given frequency to the nearest frequency bin from the FFT. The FFT can be thought of as a sampling of
     * the actual spectrum of a signal. We use this function to find the sampling point of the spectrum that is closest
     * to the given frequency.
     *
     * @param inFreq   the input frequency
     * @param stepFreq the distance between frequency bins
     * @return the closest frequency bin
     * @throws IllegalArgumentException
     */
    private double setToNearestFrequencyBin(double inFreq, double stepFreq)
            throws IllegalArgumentException {
        if (stepFreq == 0) {
            throw new IllegalArgumentException("stepFreq is zero");
        }
        return stepFreq * Math.round(inFreq / stepFreq);
    }


    /**
     * Build a mel filterbank with the parameters given. Each filter will be shaped as a triangle. The triangles overlap
     * so that they cover the whole frequency range requested. The edges of a given triangle will be by default at the
     * center of the neighboring triangles.
     *
     * @param numberFftPoints number of points in the power spectrum
     * @param numberFilters   number of filters in the filterbank
     * @param minFreq         lowest frequency in the range of interest
     * @param maxFreq         highest frequency in the range of interest
     * @throws IllegalArgumentException
     */
    private void buildFilterbank(int numberFftPoints, int numberFilters,
                                 double minFreq, double maxFreq) throws IllegalArgumentException {
        double minFreqMel;
        double maxFreqMel;
        double deltaFreqMel;
        double[] leftEdge = new double[numberFilters];
        double[] centerFreq = new double[numberFilters];
        double[] rightEdge = new double[numberFilters];
        double nextEdgeMel;
        double nextEdge;
        double initialFreqBin;
        double deltaFreq;
        this.filter = new MelFilter[numberFilters];
        /**
         * In fact, the ratio should be between <code>sampleRate /
         * 2</code>
         * and <code>numberFftPoints / 2</code> since the number of points in
         * the power spectrum is half of the number of FFT points - the other
         * half would be symmetrical for a real sequence -, and these points
         * cover up to the Nyquist frequency, which is half of the sampling
         * rate. The two "divide by 2" get canceled out.
         */
        if (numberFftPoints == 0) {
            throw new IllegalArgumentException("Number of FFT points is zero");
        }
        deltaFreq = (double) sampleRate / numberFftPoints;
        /**
         * Initialize edges and center freq. These variables will be updated so
         * that the center frequency of a filter is the right edge of the
         * filter to its left, and the left edge of the filter to its right.
         */
        if (numberFilters < 1) {
            throw new IllegalArgumentException("Number of filters illegal: "
                    + numberFilters);
        }
        minFreqMel = linToMelFreq(minFreq);
        maxFreqMel = linToMelFreq(maxFreq);
        deltaFreqMel = (maxFreqMel - minFreqMel) / (numberFilters + 1);
        leftEdge[0] = setToNearestFrequencyBin(minFreq, deltaFreq);
        nextEdgeMel = minFreqMel;
        for (int i = 0; i < numberFilters; i++) {
            nextEdgeMel += deltaFreqMel;
            nextEdge = melToLinFreq(nextEdgeMel);
            centerFreq[i] = setToNearestFrequencyBin(nextEdge, deltaFreq);
            if (i > 0) {
                rightEdge[i - 1] = centerFreq[i];
            }
            if (i < numberFilters - 1) {
                leftEdge[i + 1] = centerFreq[i];
            }
        }
        nextEdgeMel = nextEdgeMel + deltaFreqMel;
        nextEdge = melToLinFreq(nextEdgeMel);
        rightEdge[numberFilters - 1] = setToNearestFrequencyBin(nextEdge,
                deltaFreq);
        for (int i = 0; i < numberFilters; i++) {
            initialFreqBin = setToNearestFrequencyBin(leftEdge[i], deltaFreq);
            if (initialFreqBin < leftEdge[i]) {
                initialFreqBin += deltaFreq;
            }
            //System.out.format("%d %f %f\n", i, leftEdge[i], rightEdge[i]);
            this.filter[i] = new MelFilter(leftEdge[i], centerFreq[i],
                    rightEdge[i], initialFreqBin, deltaFreq);
        }
    }


    /**
     * Process data, creating the power spectrum from an input audio frame.
     *
     * @param input input power spectrum
     * @return power spectrum
     * @throws java.lang.IllegalArgumentException
     *
     */
    private DoubleData process(DoubleData input)
            throws IllegalArgumentException {
        double[] in = input.getValues();

        if (filter == null || sampleRate != input.getSampleRate()) {
            numberFftPoints = (in.length - 1) << 1;
            sampleRate = input.getSampleRate();
            buildFilterbank(numberFftPoints, numberFilters, minFreq, maxFreq);
        } else if (in.length != ((numberFftPoints >> 1) + 1)) {
            throw new IllegalArgumentException(
                    "Window size is incorrect: in.length == " + in.length
                            + ", numberFftPoints == "
                            + ((numberFftPoints >> 1) + 1));
        }
        double[] output = new double[numberFilters];
        /**
         * Filter input power spectrum
         */
        for (int i = 0; i < numberFilters; i++) {
            output[i] = filter[i].filterOutput(in);
        }
        DoubleData outputMelSpectrum = new DoubleData(output,
                sampleRate, input.getFirstSampleNumber());
        return outputMelSpectrum;
    }


    /**
     * Reads the next Data object, which is the power spectrum of an audio input frame. Signals are returned
     * unmodified.
     *
     * @return the next available Data or Signal object, or returns null if no Data is available
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

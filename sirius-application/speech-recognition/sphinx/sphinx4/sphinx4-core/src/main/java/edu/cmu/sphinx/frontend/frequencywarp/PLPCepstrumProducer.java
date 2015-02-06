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
 * Computes the PLP cepstrum from a given PLP Spectrum. The power spectrum has the amplitude compressed by computing the
 * cubed root of the PLP spectrum.  This operation is an approximation to the power law of hearing and simulates the
 * non-linear relationship between sound intensity and perceived loudness.  Computationally, this operation is used to
 * reduce the spectral amplitude of the critical band to enable all-pole modeling with relatively low order AR filters.
 * The inverse discrete cosine transform (IDCT) is then applied to the autocorrelation coefficients. A linear prediction
 * filter is then estimated from the autocorrelation values, and the linear prediction cepstrum (LPC cepstrum) is
 * finally computed from the LP filter.
 *
 * @author <a href="mailto:rsingh@cs.cmu.edu">rsingh</a>
 * @version 1.0
 * @see LinearPredictor
 */
public class PLPCepstrumProducer extends BaseDataProcessor {

    /** The property for the number of filters in the filter bank. */
    @S4Integer(defaultValue = 32)
    public static final String PROP_NUMBER_FILTERS = "numberFilters";

    /** The property specifying the length of the cepstrum data. */
    @S4Integer(defaultValue = 13)
    public static final String PROP_CEPSTRUM_LENGTH
            = "cepstrumLength";

    /** The property specifying the LPC order. */
    @S4Integer(defaultValue = 14)
    public static final String PROP_LPC_ORDER = "lpcOrder";

    private int cepstrumSize;       // size of a Cepstrum
    private int LPCOrder;           // LPC Order to compute cepstrum
    private int numberPLPFilters;   // number of PLP filters
    private double[][] cosine;

    public PLPCepstrumProducer(int numberPLPFilters,int cepstrumSize,int LPCOrder) {
        initLogger();
        this.numberPLPFilters = numberPLPFilters;
        this.cepstrumSize = cepstrumSize;
        this.LPCOrder = LPCOrder;
    }

    public PLPCepstrumProducer() {
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        numberPLPFilters = ps.getInt(PROP_NUMBER_FILTERS);
        cepstrumSize = ps.getInt(PROP_CEPSTRUM_LENGTH);
        LPCOrder = ps.getInt(PROP_LPC_ORDER);
    }


    /** Constructs a PLPCepstrumProducer */
    @Override
    public void initialize() {
        super.initialize();
        computeCosine();
    }


    /** Compute the Cosine values for IDCT. */
    private void computeCosine() {
        cosine = new double[LPCOrder + 1][numberPLPFilters];

        double period = (double) 2 * numberPLPFilters;

        for (int i = 0; i <= LPCOrder; i++) {
            double frequency = 2 * Math.PI * i / period;

            for (int j = 0; j < numberPLPFilters; j++) {
                cosine[i][j] = Math.cos(frequency * (j + 0.5));
            }
        }
    }


    /**
     * Applies the intensity loudness power law. This operation is an approximation to the power law of hearing and
     * simulates the non-linear relationship between sound intensity and percieved loudness. Computationally, this
     * operation is used to reduce the spectral amplitude of the critical band to enable all-pole modeling with
     * relatively low order AR filters.
     * @param inspectrum
     */
    private double[] powerLawCompress(double[] inspectrum) {
        double[] compressedspectrum = new double[inspectrum.length];

        for (int i = 0; i < inspectrum.length; i++) {
            compressedspectrum[i] = Math.pow(inspectrum[i], 1.0 / 3.0);
        }
        return compressedspectrum;
    }


    /**
     * Returns the next Data object, which is the PLP cepstrum of the input frame. However, it can also be other Data
     * objects like a EndPointSignal.
     *
     * @return the next available Data object, returns null if no Data object is available
     * @throws DataProcessingException if there is an error reading the Data objects
     */
    @Override
    public Data getData() throws DataProcessingException {

        Data input = getPredecessor().getData();
        Data output = input;

        getTimer().start();

        if (input != null) {
            if (input instanceof DoubleData) {
                output = process((DoubleData) input);
            }
        }

        getTimer().stop();

        return output;
    }


    /**
     * Process data, creating the PLP cepstrum from an input audio frame.
     *
     * @param input a PLP Spectrum frame
     * @return a PLP Data frame
     * @throws IllegalArgumentException
     */
    private Data process(DoubleData input) throws IllegalArgumentException {

        double[] plpspectrum = input.getValues();

        if (plpspectrum.length != numberPLPFilters) {
            throw new IllegalArgumentException
                    ("PLPSpectrum size is incorrect: plpspectrum.length == " +
                            plpspectrum.length + ", numberPLPFilters == " +
                            numberPLPFilters);
        }

        // power law compress spectrum
        double[] compressedspectrum = powerLawCompress(plpspectrum);

        // compute autocorrelation values
        double[] autocor = applyCosine(compressedspectrum);

        LinearPredictor LPC = new LinearPredictor(LPCOrder);
        // Compute LPC Parameters
        LPC.getARFilter(autocor);
        // Compute LPC Cepstra
        double[] cepstrumDouble = LPC.getData(cepstrumSize);

        DoubleData cepstrum = new DoubleData
                (cepstrumDouble, input.getSampleRate(),
                        input.getFirstSampleNumber());

        return cepstrum;
    }


    /**
     * Compute the discrete Cosine transform for the given power spectrum
     *
     * @param plpspectrum the PLPSpectrum data
     * @return autocorrelation computed from PLP spectral values
     */
    private double[] applyCosine(double[] plpspectrum) {

        double[] autocor = new double[LPCOrder + 1];
        double period = numberPLPFilters;
        double beta = 0.5f;

        // apply the idct
        for (int i = 0; i <= LPCOrder; i++) {

            if (numberPLPFilters > 0) {
                double[] cosine_i = cosine[i];
                int j = 0;
                autocor[i] += (beta * plpspectrum[j] * cosine_i[j]);

                for (j = 1; j < numberPLPFilters; j++) {
                    autocor[i] += (plpspectrum[j] * cosine_i[j]);
                }
                autocor[i] /= period;
            }
        }

        return autocor;
    }
}

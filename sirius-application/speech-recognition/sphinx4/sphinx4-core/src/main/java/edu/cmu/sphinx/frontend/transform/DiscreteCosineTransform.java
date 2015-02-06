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
package edu.cmu.sphinx.frontend.transform;

import edu.cmu.sphinx.frontend.BaseDataProcessor;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DataProcessingException;
import edu.cmu.sphinx.frontend.DoubleData;
import edu.cmu.sphinx.util.props.*;

/**
 * Applies a logarithm and then a Discrete Cosine Transform (DCT) to the input data. The input data is normally the mel
 * spectrum. It has been proven that, for a sequence of real numbers, the discrete cosine transform is equivalent to the
 * discrete Fourier transform. Therefore, this class corresponds to the last stage of converting a signal to cepstra,
 * defined as the inverse Fourier transform of the logarithm of the Fourier transform of a signal. The property {@link
 * #PROP_CEPSTRUM_LENGTH}refers to the dimensionality of the coefficients that are actually returned, defaulting to
 * 13. When the input is mel-spectrum, the vector returned is the MFCC (Mel-Frequency
 * Cepstral Coefficient) vector, where the 0-th element is the energy value.
 */
public class DiscreteCosineTransform extends BaseDataProcessor {

    /** The property for the number of filters in the filterbank. */
    @S4Integer(defaultValue = 40)
    public static final String PROP_NUMBER_FILTERS = "numberFilters";

    /** The property for the size of the cepstrum */
    @S4Integer(defaultValue = 13)
    public static final String PROP_CEPSTRUM_LENGTH = "cepstrumLength";

    protected int cepstrumSize; // size of a Cepstrum
    protected int numberMelFilters; // number of mel-filters
    protected double[][] melcosine;


    public DiscreteCosineTransform( int numberMelFilters, int cepstrumSize ) {
        initLogger();
        this.numberMelFilters = numberMelFilters;
        this.cepstrumSize = cepstrumSize;
    }

    public DiscreteCosineTransform( ) {
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        numberMelFilters = ps.getInt(PROP_NUMBER_FILTERS);
        cepstrumSize = ps.getInt(PROP_CEPSTRUM_LENGTH);
    }


    @Override
    public void initialize() {
        super.initialize();
    }


    /**
     * Returns the next DoubleData object, which is the mel cepstrum of the input frame. Signals are returned
     * unmodified.
     *
     * @return the next available DoubleData melcepstrum, or Signal object, or null if no Data is available
     * @throws DataProcessingException if a data processing error occurred
     */
    @Override
    public Data getData() throws DataProcessingException {
        Data input = getPredecessor().getData(); // get the spectrum
        getTimer().start();
        if (input != null && input instanceof DoubleData) {
            input = process((DoubleData) input);
        }
        getTimer().stop();
        return input;
    }

    final static double LOG_FLOOR = 1e-4;
    
    /**
     * Process data, creating the mel cepstrum from an input spectrum frame.
     *
     * @param input a MelSpectrum frame
     * @return a mel Cepstrum frame
     * @throws IllegalArgumentException
     */
    private DoubleData process(DoubleData input)
            throws IllegalArgumentException {
        double[] melspectrum = input.getValues();

        if (melcosine == null) {
            numberMelFilters = melspectrum.length;
            computeMelCosine();

        } else if (melspectrum.length != numberMelFilters) {
            throw new IllegalArgumentException
                    ("MelSpectrum size is incorrect: melspectrum.length == " +
                            melspectrum.length + ", numberMelFilters == " +
                            numberMelFilters);
        }
        // first compute the log of the spectrum
        for (int i = 0; i < melspectrum.length; ++i) {
            melspectrum[i] = Math.log(melspectrum[i] + LOG_FLOOR);
        }

        double[] cepstrum;

        // create the cepstrum by apply the melcosine filter
        cepstrum = applyMelCosine(melspectrum);

        return new DoubleData(cepstrum, input.getSampleRate(),
                input.getFirstSampleNumber());
    }


    /** Compute the MelCosine filter bank. */
    protected void computeMelCosine() {
        melcosine = new double[cepstrumSize][numberMelFilters];
        double period = (double) 2 * numberMelFilters;
        for (int i = 0; i < cepstrumSize; i++) {
            double frequency = 2 * Math.PI * i / period;
            for (int j = 0; j < numberMelFilters; j++) {
                melcosine[i][j] = Math.cos(frequency * (j + 0.5));
            }
        }
    }


    /**
     * Apply the MelCosine filter to the given melspectrum.
     *
     * @param melspectrum the MelSpectrum data
     * @return MelCepstrum data produced by apply the MelCosine filter to the MelSpectrum data
     */
    protected double[] applyMelCosine(double[] melspectrum) {
        // create the cepstrum
        double[] cepstrum = new double[cepstrumSize];
        double period = numberMelFilters;
        double beta = 0.5;
        // apply the melcosine filter
        for (int i = 0; i < cepstrum.length; i++) {
            if (numberMelFilters > 0) {
                double[] melcosine_i = melcosine[i];
                int j = 0;
                cepstrum[i] += (beta * melspectrum[j] * melcosine_i[j]);
                for (j = 1; j < numberMelFilters; j++) {
                    cepstrum[i] += (melspectrum[j] * melcosine_i[j]);
                }
                cepstrum[i] /= period;
            }
        }
        
        return cepstrum;
    }
}

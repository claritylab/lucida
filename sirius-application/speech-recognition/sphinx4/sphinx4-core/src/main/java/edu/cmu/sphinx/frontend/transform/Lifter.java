/*
 * Copyright 2013 Carnegie Mellon University.
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
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Integer;

/**
 * Applies the Lifter to the input mel-cepstrum to 
 * smooth cepstrum values
 * 
 * @author Horia Cucu
 */
public class Lifter extends BaseDataProcessor {

    /** The property for the value of the lifterValue */
    @S4Integer(defaultValue = 22)
    public static final String PROP_LIFTER_VALUE = "lifterValue";
    protected int lifterValue;

    protected int cepstrumSize; // size of a Cepstrum
    protected double[] lifterWeights;

    public Lifter(int lifterValue) {
        initLogger();
        this.lifterValue = lifterValue;
    }

    public Lifter() {
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        lifterValue = ps.getInt(PROP_LIFTER_VALUE);
    }

    @Override
    public void initialize() {
        super.initialize();
    }

    /**
     * Returns the next DoubleData object, which is the lifted mel-cepstrum of
     * the input mel-cepstrum. Signals are returned unmodified.
     * 
     * @return the next available DoubleData lifted mel-cepstrum, or Signal
     *         object, or null if no Data is available
     * @throws DataProcessingException
     *             if a data processing error occurred
     */
    @Override
    public Data getData() throws DataProcessingException {
        Data data = getPredecessor().getData(); // get the cepstrum
        if (data != null && data instanceof DoubleData) {
            liftCepstrum((DoubleData) data);
        }
        return data;
    }

    /**
     * Lifts the input mel-cepstrum.
     * 
     * @param input
     *            a mel-cepstrum frame
     * @throws IllegalArgumentException
     */
    private void liftCepstrum(DoubleData input) throws IllegalArgumentException {
        double[] melCepstrum = input.getValues();

        if (lifterWeights == null) {
            cepstrumSize = melCepstrum.length;
            computeLifterWeights();
        } else if (melCepstrum.length != cepstrumSize) {
            throw new IllegalArgumentException(
                    "MelCepstrum size is incorrect: "
                            + "melcepstrum.length == " + melCepstrum.length
                            + ", cepstrumSize == " + cepstrumSize);
        }

        for (int i = 0; i < melCepstrum.length; i++) {
            melCepstrum[i] = melCepstrum[i] * lifterWeights[i];
        }
    }

    /**
     * Computes the Lifter weights.
     */
    private void computeLifterWeights() {
        lifterWeights = new double[cepstrumSize];
        for (int i = 0; i < cepstrumSize; i++) {
            lifterWeights[i] = 1 + lifterValue / 2
                    * Math.sin(i * Math.PI / lifterValue);
        }
    }
}

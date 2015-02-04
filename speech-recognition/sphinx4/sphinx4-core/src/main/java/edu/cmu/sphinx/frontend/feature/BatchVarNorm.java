/*
 * Copyright 2010 PC-NG Inc..  
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.frontend.feature;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.endpoint.*;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;

import java.util.*;


/**
 * Applies cepstral variance normalization (CVN), so that each coefficient
 * will have unit variance. You need to put this element after the means
 * normalizer in frontend pipeline.
 * <p/>
 * CVN is sited to improve the stability of the decoding with the additive
 * noise, so it might be useful in some situations.
 *
 * @see LiveCMN
 */
public class BatchVarNorm extends BaseDataProcessor {

    private double[] variances; // array of current sums
    private List<Data> cepstraList;
    private int numberDataCepstra;

    public BatchVarNorm() {
        initLogger();
    }

    /* (non-Javadoc)
     * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
     */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
    }


    /** Initializes this BatchCMN. */
    @Override
    public void initialize() {
        super.initialize();
        variances = null;
        cepstraList = new LinkedList<Data>();
    }


    /** Initializes the sums array and clears the cepstra list. */
    private void reset() {
        variances = null; // clears the sums array
        cepstraList.clear();
        numberDataCepstra = 0;
    }


    /**
     * Returns the next Data object, which is a normalized cepstrum. Signal objects are returned unmodified.
     *
     * @return the next available Data object, returns null if no Data object is available
     * @throws DataProcessingException if there is an error processing data
     */
    @Override
    public Data getData() throws DataProcessingException {

        Data output = null;

        if (!cepstraList.isEmpty()) {
            output = cepstraList.remove(0);
        } else {
            reset();
            // read the cepstra of the entire utterance, calculate
            // and apply variance normalization
            if (readUtterance() > 0) {
                normalizeList();
                output = cepstraList.remove(0); //getData();
            }
        }

        return output;
    }


    /**
     * Reads the cepstra of the entire Utterance into the cepstraList.
     *
     * @return the number cepstra (with Data) read
     * @throws DataProcessingException if an error occurred reading the Data
     */
    private int readUtterance() throws DataProcessingException {

        Data input = null;

        do {
            input = getPredecessor().getData();
            if (input != null) {
                if (input instanceof DoubleData) {
                    numberDataCepstra++;
                    double[] cepstrumData = ((DoubleData) input).getValues();
                    if (variances == null) {
                        variances = new double[cepstrumData.length];
                    } else {
                        if (variances.length != cepstrumData.length) {
                            throw new Error
                                    ("Inconsistent cepstrum lengths: sums: " +
                                            variances.length + ", cepstrum: " +
                                            cepstrumData.length);
                        }
                    }
                    // add the cepstrum data to the sums
                    for (int j = 0; j < cepstrumData.length; j++) {
                        variances[j] += cepstrumData[j] * cepstrumData[j];
                    }
                    cepstraList.add(input);

                } else if (input instanceof DataEndSignal || input instanceof SpeechEndSignal) {
                    cepstraList.add(input);
                    break;
                } else { // DataStartSignal or other Signal
                    cepstraList.add(input);
                }
            }
        } while (input != null);

        return numberDataCepstra;
    }


    /** Normalizes the list of Data. */
    private void normalizeList() {

        // calculate the variance first
        for (int i = 0; i < variances.length; i++) {
            variances[i] = Math.sqrt(numberDataCepstra / variances[i]);
        }

        for (Data data : cepstraList) {
            if (data instanceof DoubleData) {
                double[] cepstrum = ((DoubleData)data).getValues();
                for (int j = 0; j < cepstrum.length; j++) {
                    cepstrum[j] *= variances[j];
                }
            }
        }
    }
}

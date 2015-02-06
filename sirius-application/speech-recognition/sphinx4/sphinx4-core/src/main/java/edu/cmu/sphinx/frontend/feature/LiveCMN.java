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

package edu.cmu.sphinx.frontend.feature;

import java.util.LinkedList;
import java.util.List;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.endpoint.SpeechEndSignal;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Integer;

/**
 * Subtracts the mean of all the input so far from the Data objects.
 * 
 * Unlike the {@link BatchCMN}, it does not read in the entire stream of Data
 * objects before it calculates the mean. It estimates the mean from already
 * seen data and subtracts the mean from the Data objects on the fly. Therefore,
 * there is no delay introduced by LiveCMN in general. The only real issue is an
 * initial CMN estimation, for that some amount of frames are read initially
 * and cmn estimation is calculated from them.
 * <p/>
 * The properties that affect this processor are defined by the fields
 * {@link #PROP_INITIAL_CMN_WINDOW}, {@link #PROP_CMN_WINDOW}, and
 * {@link #PROP_CMN_SHIFT_WINDOW}. Please follow the link
 * "Constant Field Values" below to see the actual name of the Sphinx
 * properties.
 * <p/>
 * <p>
 * The mean of all the input cepstrum so far is not reestimated for each
 * cepstrum. This mean is recalculated after every
 * {@link #PROP_CMN_SHIFT_WINDOW} cepstra. This mean is estimated by dividing
 * the sum of all input cepstrum so far. After obtaining the mean, the sum is
 * exponentially decayed by multiplying it by the ratio:
 * 
 * <pre>
 * cmnWindow/(cmnWindow + number of frames since the last recalculation)
 * </pre>
 * <p/>
 * 
 * @see BatchCMN
 */
public class LiveCMN extends BaseDataProcessor {

    /** The property for the live CMN initial window size. */
    @S4Integer(defaultValue = 200)
    public static final String PROP_INITIAL_CMN_WINDOW = "initialCmnWindow";
    private int initialCmnWindow;
    
    /** The property for the live CMN window size. */
    @S4Integer(defaultValue = 100)
    public static final String PROP_CMN_WINDOW = "cmnWindow";
    private int cmnWindow;

    /**
     * The property for the CMN shifting window. The shifting window specifies
     * how many cepstrum after which we re-calculate the cepstral mean.
     */
    @S4Integer(defaultValue = 160)
    public static final String PROP_CMN_SHIFT_WINDOW = "shiftWindow";
    private int cmnShiftWindow; // # of Cepstrum to recalculate mean

    private double[] currentMean; // array of current means
    private double[] sum; // array of current sums
    private int numberFrame; // total number of input Cepstrum

    List<Data> initialList;

    public LiveCMN(double initialMean, int cmnWindow, int cmnShiftWindow, int initialCmnWindow) {
        initLogger();
        this.cmnWindow = cmnWindow;
        this.cmnShiftWindow = cmnShiftWindow;
        this.initialCmnWindow = initialCmnWindow;
    }

    public LiveCMN() {

    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        cmnWindow = ps.getInt(PROP_CMN_WINDOW);
        cmnShiftWindow = ps.getInt(PROP_CMN_SHIFT_WINDOW);
        initialCmnWindow = ps.getInt(PROP_INITIAL_CMN_WINDOW);
    }

    /** Initializes this LiveCMN. */
    @Override
    public void initialize() {
        super.initialize();
    }

    /**
     * Initializes the currentMean and sum arrays with the given cepstrum
     * length.
     * 
     * @param cepstrumLength
     *            the length of the cepstrum
     */
    private void initMeansSums() {
        int size = -1;
        for (Data data : initialList) {
            if (!(data instanceof DoubleData))
                continue;
            double[] cepstrum = ((DoubleData) data).getValues();
            // Initialize arrays if needed
            if (size < 0) {
                size = cepstrum.length;
                sum = new double[size];
                numberFrame = 0;
            }
            // Process
            for (int j = 0; j < size; j++) {
                sum[j] += cepstrum[j];
            }
            numberFrame++;
        }
        currentMean = new double[size];
        for (int j = 0; j < size; j++) {
            currentMean[j] = sum[j] / numberFrame;
        }
    }

    /**
     * Returns the next Data object, which is a normalized Data produced by this
     * class. Signals are returned unmodified.
     * 
     * @return the next available Data object, returns null if no Data object is
     *         available
     * @throws DataProcessingException
     *             if there is a data processing error
     */
    @Override
    public Data getData() throws DataProcessingException {

        Data input, output;
        getTimer().start();

        if (initialList == null) {
            initialList = new LinkedList<Data>();
            // Collect initial data for estimation
            while (initialList.size() < initialCmnWindow) {
                input = getPredecessor().getData();
                initialList.add(input);
                if (input instanceof SpeechEndSignal
                        || input instanceof DataEndSignal)
                    break;
            }
            initMeansSums();
            output = initialList.remove(0);
        } else if (!initialList.isEmpty()) {
            // Return the previously collected data
            output = initialList.remove(0);
        } else {
            // Process normal frame
            output = getPredecessor().getData();
        }

        normalize(output);
        getTimer().stop();
        return output;
    }

    /**
     * Normalizes the given Data with using the currentMean array. Updates the
     * sum array with the given Data.
     * 
     * @param cepstrumObject
     *            the Data object to normalize
     */
    private void normalize(Data data) {

        if (!(data instanceof DoubleData))
            return;

        double[] cepstrum = ((DoubleData) data).getValues();

        if (cepstrum.length != sum.length) {
            throw new Error("Data length (" + cepstrum.length
                    + ") not equal sum array length (" + sum.length + ')');
        }

        for (int j = 0; j < cepstrum.length; j++) {
            sum[j] += cepstrum[j];
            cepstrum[j] -= currentMean[j];
        }

        numberFrame++;

        if (numberFrame > cmnShiftWindow) {
            updateMeanSumBuffers();
        }
    }

    /**
     * Updates the currentMean buffer with the values in the sum buffer. Then
     * decay the sum buffer exponentially, i.e., divide the sum with
     * numberFrames.
     */
    private void updateMeanSumBuffers() {

        // update the currentMean buffer with the sum buffer
        double sf = 1.0 / numberFrame;

        System.arraycopy(sum, 0, currentMean, 0, sum.length);

        multiplyArray(currentMean, sf);

        // decay the sum buffer exponentially
        if (numberFrame >= cmnShiftWindow) {
            multiplyArray(sum, (sf * cmnWindow));
            numberFrame = cmnWindow;
        }
    }

    /**
     * Multiplies each element of the given array by the multiplier.
     * 
     * @param array
     *            the array to multiply
     * @param multiplier
     *            the amount to multiply by
     */
    private static void multiplyArray(double[] array, double multiplier) {
        for (int i = 0; i < array.length; i++) {
            array[i] *= multiplier;
        }
    }
}

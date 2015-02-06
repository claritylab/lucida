/*
 * Copyright 1999-2002 Carnegie Mellon University.  
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
 * Applies automatic gain control (CMN)
 */
public class BatchAGC extends BaseDataProcessor {

    private List<Data> cepstraList;
    private double agc;

    public BatchAGC() {
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
        cepstraList = new LinkedList<Data>();
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
	    agc = 0.0;
    	    cepstraList.clear();
            // read the cepstra of the entire utterance, calculate and substract gain
            if (readUtterance() > 0) {
                normalizeList();
                output = cepstraList.remove(0);
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
        int numFrames = 0;

        while (true) {
            input = getPredecessor().getData();
            if (input == null) {
		break;
	    } else if (input instanceof DataEndSignal || input instanceof SpeechEndSignal) {
                cepstraList.add(input);
                break;
	    } else if (input instanceof DoubleData) {
	        cepstraList.add(input);
		double c0 = ((DoubleData)input).getValues()[0];
		if (agc < c0)
		    agc = c0;
            } else { // DataStartSignal or other Signal
                cepstraList.add(input);
            }
            numFrames++;
        }

        return numFrames;
    }

    /** Normalizes the list of Data. */
    private void normalizeList() {
        for (Data data : cepstraList) {
            if (data instanceof DoubleData) {
                ((DoubleData)data).getValues()[0] -= agc;
            }
        }
    }
}

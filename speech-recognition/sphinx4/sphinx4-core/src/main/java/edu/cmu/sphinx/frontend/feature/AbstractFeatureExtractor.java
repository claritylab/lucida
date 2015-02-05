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

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.endpoint.*;
import edu.cmu.sphinx.util.props.*;

import java.util.*;

/**
 * Abstract base class for windowed feature extractors like DeltasFeatureExtractor, ConcatFeatureExtractor
 * or S3FeatureExtractor. The main purpose of this it to collect window size cepstra frames in a buffer
 * and let the extractor compute the feature frame with them.
 */
public abstract class AbstractFeatureExtractor extends BaseDataProcessor {

    /** The property for the window of the DeltasFeatureExtractor. */
    @S4Integer(defaultValue = 3)
    public static final String PROP_FEATURE_WINDOW = "windowSize";

    private int bufferPosition;
    private Signal pendingSignal;
    private LinkedList<Data> outputQueue;

    protected int cepstraBufferEdge;
    protected int window;
    protected int currentPosition;
    protected int cepstraBufferSize;
    protected DoubleData[] cepstraBuffer;

    /**
     * 
     * @param window
     */
    public AbstractFeatureExtractor( int window ) {
        initLogger();
        this.window = window;
    }

    public AbstractFeatureExtractor() {
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        window = ps.getInt(PROP_FEATURE_WINDOW);
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.frontend.DataProcessor#initialize(edu.cmu.sphinx.frontend.CommonConfig)
    */
    @Override
    public void initialize() {
        super.initialize();
        cepstraBufferSize = 256;
        cepstraBuffer = new DoubleData[cepstraBufferSize];
        cepstraBufferEdge = cepstraBufferSize - (window * 2 + 2);
        outputQueue = new LinkedList<Data>();
        reset();
    }


    /** Resets the DeltasFeatureExtractor to be ready to read the next segment of data. */
    private void reset() {
        bufferPosition = 0;
        currentPosition = 0;
    }


    /**
     * Returns the next Data object produced by this DeltasFeatureExtractor.
     *
     * @return the next available Data object, returns null if no Data is available
     * @throws DataProcessingException if there is a data processing error
     */
    @Override
    public Data getData() throws DataProcessingException {
        if (outputQueue.isEmpty()) {
            Data input = getNextData();
            if (input != null) {
                if (input instanceof DoubleData) {
                    addCepstrum((DoubleData) input);
                    computeFeatures(1);
                } else if (input instanceof DataStartSignal) {
                    pendingSignal = null;
                    outputQueue.add(input);
                    Data start = getNextData();
                    int n = processFirstCepstrum(start);
                    computeFeatures(n);
                    if (pendingSignal != null) {
                        outputQueue.add(pendingSignal);
                    }
                } else if (input instanceof DataEndSignal || input instanceof SpeechEndSignal) {
                    // when the DataEndSignal is right at the boundary
                    int n = replicateLastCepstrum();
                    computeFeatures(n);
                    outputQueue.add(input);
                }
            }
        }
        return outputQueue.isEmpty() ? null : outputQueue.removeFirst();
    }


    private Data getNextData() throws DataProcessingException {
        Data d = getPredecessor().getData();
        while (d != null && !(d instanceof DoubleData || d instanceof DataEndSignal || d instanceof DataStartSignal || d instanceof SpeechEndSignal)) {
            outputQueue.add(d);
            d = getPredecessor().getData();
        }

        return d;
    }


    /**
     * Replicate the given cepstrum Data object into the first window+1 number of frames in the cepstraBuffer. This is
     * the first cepstrum in the segment.
     *
     * @param cepstrum the Data to replicate
     * @return the number of Features that can be computed
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     */
    private int processFirstCepstrum(Data cepstrum)
            throws DataProcessingException {
        if (cepstrum instanceof DataEndSignal) {
            outputQueue.add(cepstrum);
            return 0;
        } else if (cepstrum instanceof DataStartSignal) {
            throw new Error("Too many UTTERANCE_START");
        } else {
            // At the start of an utterance, we replicate the first frame
            // into window+1 frames, and then read the next "window" number
            // of frames. This will allow us to compute the delta-
            // double-delta of the first frame.
            Arrays.fill(cepstraBuffer, 0, window + 1, cepstrum);
            bufferPosition = window + 1;
            bufferPosition %= cepstraBufferSize;
            currentPosition = window;
            currentPosition %= cepstraBufferSize;
            int numberFeatures = 1;
            pendingSignal = null;
            for (int i = 0; i < window; i++) {
                Data next = getNextData();
                if (next != null) {
                    if (next instanceof DoubleData) {
                        // just a cepstra
                        addCepstrum((DoubleData) next);
                    } else if (next instanceof DataEndSignal || next instanceof SpeechEndSignal) {
                        // end of segment cepstrum
                        pendingSignal = (Signal) next;
                        replicateLastCepstrum();
                        numberFeatures += i;
                        break;
                    } else if (next instanceof DataStartSignal) {
                        throw new Error("Too many UTTERANCE_START");
                    }
                }
            }
            return numberFeatures;
        }
    }


    /**
     * Adds the given DoubleData object to the cepstraBuffer.
     *
     * @param cepstrum the DoubleData object to add
     */
    private void addCepstrum(DoubleData cepstrum) {
        cepstraBuffer[bufferPosition++] = cepstrum;
        bufferPosition %= cepstraBufferSize;
    }


    /**
     * Replicate the last frame into the last window number of frames in the cepstraBuffer.
     *
     * @return the number of replicated Cepstrum
     */
    private int replicateLastCepstrum() {
        DoubleData last;
        if (bufferPosition > 0) {
            last = cepstraBuffer[bufferPosition - 1];
        } else if (bufferPosition == 0) {
            last = cepstraBuffer[cepstraBuffer.length - 1];
        } else {
            throw new Error("BufferPosition < 0");
        }
        for (int i = 0; i < window; i++) {
            addCepstrum(last);
        }
        return window;
    }


    /**
     * Converts the Cepstrum data in the cepstraBuffer into a FeatureFrame.
     *
     * @param totalFeatures the number of Features that will be produced
     */
    private void computeFeatures(int totalFeatures) {
        getTimer().start();
        if (totalFeatures == 1) {
            computeFeature();
        } else {
            // create the Features
            for (int i = 0; i < totalFeatures; i++) {
                computeFeature();
            }
        }
        getTimer().stop();
    }


    /** Computes the next Feature. */
    private void computeFeature() {
        Data feature = computeNextFeature();
        outputQueue.add(feature);
    }


    /**
     * Computes the next feature. Advances the pointers as well.
     *
     * @return the feature Data computed
     */
    protected abstract Data computeNextFeature();
}

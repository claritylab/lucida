/*
 *
 * Copyright 1999-2004 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.decoder;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.endpoint.SpeechEndSignal;
import edu.cmu.sphinx.frontend.endpoint.SpeechStartSignal;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.decoder.search.SearchManager;

import java.util.List;


/**
 * A decoder which does not use the common pull-principle of S4 but processes only one single frame on each call of
 * <code>decode()</code>. When using this decoder, make sure that the <code>AcousticScorer</code> used by the
 * <code>SearchManager</code> can access some buffered <code>Data</code>s.
 */
public class FrameDecoder extends AbstractDecoder implements DataProcessor {

    private DataProcessor predecessor;

    private boolean isRecognizing;
    private Result result;

    public FrameDecoder( SearchManager searchManager, boolean fireNonFinalResults, boolean autoAllocate, List<ResultListener> listeners) {
        super(searchManager, fireNonFinalResults, autoAllocate, listeners);
    }    
    
    public FrameDecoder() {
    }

    /**
     * Decode a single frame.
     *
     * @param referenceText the reference text (or null)
     * @return a result
     */
    @Override
    public Result decode(String referenceText) {
        return searchManager.recognize(1);
    }

    public Data getData() throws DataProcessingException {
        Data d = getPredecessor().getData();

        if (isRecognizing && (d instanceof FloatData || d instanceof DoubleData || d instanceof SpeechEndSignal)) {
            result = decode(null);

            if (result != null) {
                fireResultListeners(result);
                result = null;
            }
        }

        // we also trigger recogntion on a DataEndSignal to allow threaded scorers to shut down correctly
        if (d instanceof DataEndSignal) {
            searchManager.stopRecognition();
        }

        if (d instanceof SpeechStartSignal) {
            searchManager.startRecognition();
            isRecognizing = true;
            result = null;
        }

        if (d instanceof SpeechEndSignal) {
            searchManager.stopRecognition();

            //fire results which were not yet final
            if (result != null)
                fireResultListeners(result);

            isRecognizing = false;
        }

        return d;
    }


    public DataProcessor getPredecessor() {
        return predecessor;
    }


    public void setPredecessor(DataProcessor predecessor) {
        this.predecessor = predecessor;
    }


    public void initialize() {
    }

}

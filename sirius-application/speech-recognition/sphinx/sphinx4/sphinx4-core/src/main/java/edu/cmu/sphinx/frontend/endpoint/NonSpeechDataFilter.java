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


package edu.cmu.sphinx.frontend.endpoint;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;


/**
 * Given a sequence of Data, filters out the non-speech regions. The sequence of Data should have the speech and
 * non-speech regions marked out by the SpeechStartSignal and SpeechEndSignal, using the {@link SpeechMarker
 * SpeechMarker}.
 * <p/>
 * Only one speech region</b> <p>In the first case, the data stream has only one speech region: <p><img
 * src="doc-files/one-region.gif"> <br><i>Figure 1: A data stream with only one speech region</i>. <p>After filtering,
 * the non-speech regions are removed, and becomes: <p><img src="doc-files/one-region-filtered.gif"> <br><i>Figure 2: A
 * data stream with only on speech region after filtering.</i>
 */
public class NonSpeechDataFilter extends BaseDataProcessor {

    private boolean inSpeech;

    public NonSpeechDataFilter() {
        initLogger();
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
    }


    /** Initializes this data processor */
    @Override
    public void initialize() {
        super.initialize();

        this.inSpeech = false;
    }


    @Override
    public Data getData() throws DataProcessingException {
        Data data = readData();

        while (data != null
                && !(data instanceof DataEndSignal) && !(data instanceof DataStartSignal)
                && !(data instanceof SpeechEndSignal) && !inSpeech) {
            data = readData();
        }

        return data;
    }


    private Data readData() {
        Data data = getPredecessor().getData();

        if (data instanceof SpeechStartSignal)
            inSpeech = true;

        if (data instanceof SpeechEndSignal)
            inSpeech = false;

        return data;
    }
}

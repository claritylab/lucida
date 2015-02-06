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

package edu.cmu.sphinx.trainer;

import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.S4Integer;
import edu.cmu.sphinx.util.props.S4String;


/** Provides mechanisms for reading a control file (or a pair control file plus transcription file). */
public interface ControlFile extends Configurable {

    /** Simple control file containing audio file names only. */
    @S4String(defaultValue = "an4_train.fileids")
    String PROP_AUDIO_FILE = "audioFile";

    /** Transcription file containing transcriptions, simple or full. */
    @S4String(defaultValue = "an4_train.transcription")
    String PROP_TRANSCRIPT_FILE = "transcriptFile";

    /** The property for which batch partition to process. */
    @S4Integer(defaultValue = 1)
    public final static String PROP_WHICH_BATCH = "whichBatch";

    /** The property for the total number of batch partitions. */
    @S4Integer(defaultValue = 1)
    public final static String PROP_TOTAL_BATCHES = "totalBatches";


    /** Gets an iterator for utterances. */
    public void startUtteranceIterator();


    /** Returns whether there is a next utterance. */
    public boolean hasMoreUtterances();


    /** Returns next utterance. */
    public Utterance nextUtterance();

}

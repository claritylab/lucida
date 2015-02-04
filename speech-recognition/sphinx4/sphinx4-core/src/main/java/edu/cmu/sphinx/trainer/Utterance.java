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

import edu.cmu.sphinx.linguist.dictionary.Dictionary;

/** Provides mechanisms for accessing an utterance. */
public interface Utterance {

    /**
     * Add transcript with dictionary and exact flag.
     *
     * @param transcript    the transcript
     * @param dictionary    the default dictionary name
     * @param isExact       the default flag
     * @param wordSeparator the word separator characters
     */
    public void add(String transcript, Dictionary dictionary,
                    boolean isExact, String wordSeparator);


    /** Gets the transcript iterator. */
    public void startTranscriptIterator();


    /** Returns whether there is a next transcript. */
    public boolean hasMoreTranscripts();


    /** Returns next transcript. */
    public Transcript nextTranscript();

}

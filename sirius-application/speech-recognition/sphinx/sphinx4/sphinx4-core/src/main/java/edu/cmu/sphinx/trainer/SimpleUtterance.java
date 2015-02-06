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

import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.logging.Logger;

/** Provides mechanism for handling a simple utterance. */
public class SimpleUtterance implements Utterance {

    private String utteranceID;
    private Collection<SimpleTranscript> transcriptSet;
    private Iterator<SimpleTranscript> transcriptIterator;

    /*
     * The logger for this class
     */
    private static Logger logger =
            Logger.getLogger("edu.cmu.sphinx.trainer.SimpleUtterance");


    /** Constructor for class SimpleUtterance. */
    public SimpleUtterance() {
        transcriptSet = new LinkedList<SimpleTranscript>();
    }


    /**
     * Constructor for class SimpleUtterance.
     *
     * @param utteranceID the utterance ID, usually a file name.
     */
    public SimpleUtterance(String utteranceID) {
        logger.info("Utterance ID: " + utteranceID);
        this.utteranceID = utteranceID;
        this.transcriptSet = new LinkedList<SimpleTranscript>();
    }


    /**
     * Add transcript with dictionary and exact flag.
     *
     * @param transcript    the transcript
     * @param dictionary    the default dictionary name
     * @param isExact       the default flag
     * @param wordSeparator the word separator characters
     */
    public void add(String transcript, Dictionary dictionary,
                    boolean isExact, String wordSeparator) {
        logger.info("Transcript: " + transcript);
        transcriptSet.add(new SimpleTranscript(transcript, dictionary,
                isExact, wordSeparator));
    }


    /**
     * Starts the transcript iterator.
     *
     * @return the transcript iterator.
     */
    public void startTranscriptIterator() {
        transcriptIterator = transcriptSet.iterator();
    }


    /**
     * Returns whether there is a next transcript.
     *
     * @return true if there are more transcrips.
     */
    public boolean hasMoreTranscripts() {
        return transcriptIterator.hasNext();
    }


    /**
     * Gets next transcript.
     *
     * @return the next Trasncript.
     */
    public Transcript nextTranscript() {
        return transcriptIterator.next();
    }


    /**
     * Returns a string representation of this utterance.
     *
     * @return the string representation.
     */
    @Override
    public String toString() {
        return utteranceID;
    }
}

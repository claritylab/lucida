/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 
 * Portions Copyright 2010 LIUM, University of Le Mans, France
 -> Yannick Esteve, Anthony Rousseau
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.linguist.language.ngram.large;

/** Represents a word ID (Nth word of a N-gram), and a N-gram probability ID. */
class NGramProbability {

    private final int which;
    private final int wordID;
    private final int probabilityID;
    private final int backoffID;
    private final int firstNPlus1GramEntry;

    /**
     * Constructs a NGramProbability
     * 
     * @param which
     *            which follower of the first word is this NGram
     * @param wordID
     *            the ID of the Nth word in a NGram
     * @param probabilityID
     *            the index into the probability array
     * @param backoffID
     *            the index into the backoff probability array
     * @param firstNPlus1GramEntry
     *            the first N+1Gram entry
     */
    public NGramProbability(int which, int wordID, int probabilityID,
            int backoffID, int firstNPlus1GramEntry) {
        this.which = which;
        this.wordID = wordID;
        this.probabilityID = probabilityID;
        this.backoffID = backoffID;
        this.firstNPlus1GramEntry = firstNPlus1GramEntry;
    }

    /**
     * Returns which follower of the first word is this NGram
     * 
     * @return which follower of the first word is this NGram
     */
    public int getWhichFollower() {
        return which;
    }

    /**
     * Returns the Nth word ID of this NGram
     * 
     * @return the Nth word ID
     */
    public int getWordID() {
        return wordID;
    }

    /**
     * Returns the NGram probability ID.
     * 
     * @return the NGram probability ID
     */
    public int getProbabilityID() {
        return probabilityID;
    }

    /**
     * Returns the backoff weight ID.
     * 
     * @return the backoff weight ID
     */
    public int getBackoffID() {
        return backoffID;
    }

    /**
     * Returns the index of the first N+1Gram entry.
     * 
     * @return the index of the first N+1Gram entry
     */
    public int getFirstNPlus1GramEntry() {
        return firstNPlus1GramEntry;
    }
}

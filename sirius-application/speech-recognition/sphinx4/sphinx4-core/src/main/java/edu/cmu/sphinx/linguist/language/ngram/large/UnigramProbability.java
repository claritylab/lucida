/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * Portions Copyright 2010 LIUM, University of Le Mans, France
  -> Yannick Esteve, Anthony Rousseau

 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.linguist.language.ngram.large;


/** Represents a probability, a backoff probability, and the location of the first bigram entry. */
class UnigramProbability {

    private final int wordID;
    private float logProbability;
    private float logBackoff;
    private final int firstBigramEntry;


    /**
     * Constructs a UnigramProbability
     *
     * @param wordID           the id of the word
     * @param logProbability   the probability
     * @param logBackoff       the backoff probability
     * @param firstBigramEntry the first bigram entry
     */
    public UnigramProbability(int wordID, float logProbability,
                              float logBackoff, int firstBigramEntry) {
        this.wordID = wordID;
        this.logProbability = logProbability;
        this.logBackoff = logBackoff;
        this.firstBigramEntry = firstBigramEntry;
    }


    /**
     * Returns a string representation of this object
     *
     * @return the string form of this object
     */
    @Override
    public String toString() {
        return "Prob: " + logProbability + ' ' + logBackoff;
    }


    /**
     * Returns the word ID of this unigram
     *
     * @return the word ID of this unigram
     */
    public int getWordID() {
        return wordID;
    }


    /**
     * Returns the log probability of this unigram.
     *
     * @return the log probability of this unigram
     */
    public float getLogProbability() {
        return logProbability;
    }


    /**
     * Returns the log backoff weight of this unigram
     *
     * @return the log backoff weight of this unigram
     */
    public float getLogBackoff() {
        return logBackoff;
    }


    /**
     * Returns the index of the first bigram entry of this unigram.
     *
     * @return the index of the first bigram entry of this unigram
     */
    public int getFirstBigramEntry() {
        return firstBigramEntry;
    }


    /**
     * Sets the log probability of this unigram
     *
     * @param logProbability the new log probability of this unigram
     */
    public void setLogProbability(float logProbability) {
        this.logProbability = logProbability;
    }


    /**
     * Sets the log backoff weight.
     *
     * @param logBackoff the new log backoff weight
     */
    public void setLogBackoff(float logBackoff) {
        this.logBackoff = logBackoff;
    }
}




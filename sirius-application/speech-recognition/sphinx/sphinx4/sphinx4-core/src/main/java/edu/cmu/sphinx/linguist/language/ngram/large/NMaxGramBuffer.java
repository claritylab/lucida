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


/**
 * Implements a buffer that contains NGrams of model's MAX order. 
 * It assumes that the first two bytes of each n-gram entry is the 
 * ID of the n-gram.
 */

class NMaxGramBuffer extends NGramBuffer {

    /**
     * Constructs a NMaxGramBuffer object with the given byte[].
     *
     * @param buffer       the byte[] with NGrams
     * @param numberNGrams the number of N-gram
     * @param bigEndian	   the buffer's endianness
     * @param is32bits     whether the buffer is 16 or 32 bits
     * @param n	           the buffer's order
     * @param firstCurrentNGramEntry the first Current NGram Entry
    */
    public NMaxGramBuffer(byte[] buffer, int numberNGrams, boolean bigEndian, boolean is32bits, int n, int firstCurrentNGramEntry) {
        super(buffer, numberNGrams, bigEndian, is32bits, n, firstCurrentNGramEntry);
    }


    /**
     * Returns the NGramProbability of the nth follower.
     *
     * @param nthFollower which follower
     * @return the NGramProbability of the nth follower
     */
    @Override
    public int getProbabilityID(int nthFollower) {
    	int nthPosition = 0;
    	
    	nthPosition = nthFollower * LargeNGramModel.BYTES_PER_NMAXGRAM * ((is32bits()) ? 4 : 2);
    	setPosition(nthPosition + ((is32bits()) ? 4 : 2)); // to skip the word ID
    	
        return readBytesAsInt();
    }
    
    
    /**
     * Finds the NGram probabilities for the given nth word in a NGram.
     *
     * @param nthWordID the ID of the nth word
     * @return the NGramProbability of the given nth word
     */
    @Override
    public NGramProbability findNGram(int nthWordID) {

        int mid, start = 0, end = getNumberNGrams();
        NGramProbability ngram = null;

        while ((end - start) > 0) {
            mid = (start + end) / 2;
            int midWordID = getWordID(mid);
            if (midWordID < nthWordID) {
                start = mid + 1;
            } else if (midWordID > nthWordID) {
                end = mid;
            } else {
                ngram = getNGramProbability(mid);
                break;
            }
        }

        return ngram;
    }
    

    /**
     * Returns the NGramProbability of the nth follower.
     *
     * @param nthFollower which follower
     * @return the NGramProbability of the nth follower
     */
    @Override
    public NGramProbability getNGramProbability(int nthFollower) {
    	int nthPosition = 0, wordID = 0, probID = 0, backoffID = 0, firstNGram = 0;

    	nthPosition = nthFollower * LargeNGramModel.BYTES_PER_NMAXGRAM * ((is32bits()) ? 4 : 2);

        setPosition(nthPosition);
        
        wordID = readBytesAsInt();
        probID = readBytesAsInt();
            
        return (new NGramProbability(nthFollower, wordID, probID, backoffID, firstNGram));
    }
}

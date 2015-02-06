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
 * Implements a buffer that contains NGrams. It assumes that the first two bytes of each n-gram entry is the ID of the
 * n-gram.
 */

class NGramBuffer {

    private final byte[] buffer;
    private final int numberNGrams;
    private int position;
    private final boolean bigEndian;
    private final boolean is32bits;
    private final int n;
    private boolean used;
    private int firstNGramEntry;

    /**
     * Constructs a NGramBuffer object with the given byte[].
     *
     * @param buffer       the byte[] with NGrams
     * @param numberNGrams the number of N-gram
     * @param bigEndian	   the buffer's endianness
     * @param is32bits     whether the buffer is 16 or 32 bits
     * @param n	           the buffer's order
     * @param firstNGramEntry  the first NGram Entry
     */
    public NGramBuffer(byte[] buffer, int numberNGrams, boolean bigEndian, boolean is32bits, int n, int firstNGramEntry) {
        this.buffer = buffer;
        this.numberNGrams = numberNGrams;
        this.bigEndian = bigEndian;
        this.is32bits = is32bits;
        this.position = 0;
        this.n = n;
	this.firstNGramEntry = firstNGramEntry;
    }


    /**
     * Returns the byte[] of n-grams.
     *
     * @return the byte[] of n-grams
     */
    public byte[] getBuffer() {
        return buffer;
    }

    /**
     * Returns the firstNGramEntry
     * @return the firstNGramEntry of the buffer
     */

    public int getFirstNGramEntry() {
        return firstNGramEntry;
    }


    /**
     * Returns the size of the buffer in bytes.
     *
     * @return the size of the buffer in bytes
     */
    public int getSize() {
        return buffer.length;
    }


    /**
     * Returns the number of n-grams in this buffer.
     *
     * @return the number of n-grams in this buffer
     */
    public int getNumberNGrams() {
        return numberNGrams;
    }


    /**
     * Returns the position of the buffer.
     *
     * @return the position of the buffer
     */
    protected int getPosition() {
        return position;
    }


    protected int getN() {
    	return n;
    }
    
    
    /**
     * Sets the position of the buffer.
     *
     * @param position new buffer position
     */
    protected void setPosition(int position) {
        this.position = position;
    }


    /**
     * Returns the word ID of the nth follower, assuming that the ID is the first two bytes of the NGram entry.
     *
     * @param nthFollower starts from 0 to (numberFollowers - 1).
     * @return the word ID
     */
    public final int getWordID(int nthFollower) {
        int nthPosition = nthFollower * (buffer.length / numberNGrams);
        setPosition(nthPosition);
        return readBytesAsInt();
    }


    /**
     * Returns true if the NGramBuffer is big-endian.
     *
     * @return true if the NGramBuffer is big-endian, false if little-endian
     */
    public final boolean isBigEndian() {
        return bigEndian;
    }


    /**
     * Returns true if the NGramBuffer is 32 bits.
     *
     * @return true if the NGramBuffer is 32 bits, false if 16 bits
     */
    public final boolean is32bits() {
    	return is32bits;
    }
    
    /**
     * Reads the next two bytes from the buffer's current position as an integer.
     *
     * @return the next two bytes as an integer
     */
    public final int readBytesAsInt() {
    	if (is32bits) {
            if (bigEndian) {
                int value = (0x000000ff & buffer[position++]);
                value <<= 8;
                value |= (0x000000ff & buffer[position++]);
                value <<= 8;
                value |= (0x000000ff & buffer[position++]);
                value <<= 8;
                value |= (0x000000ff & buffer[position++]);
                return value;
            } else {
                int value = (0x000000ff & buffer[position+3]);
                value <<= 8;
                value |= (0x000000ff & buffer[position+2]);
                value <<= 8;
                value |= (0x000000ff & buffer[position+1]);
                value <<= 8;
                value |= (0x000000ff & buffer[position]);
                position += 4;
                return value;
            }
    	}
    	else {
            if (bigEndian) {
                int value = (0x000000ff & buffer[position++]);
                value <<= 8;
                value |= (0x000000ff & buffer[position++]);
                return value;
            } else {
                int value = (0x000000ff & buffer[position + 1]);
                value <<= 8;
                value |= (0x000000ff & buffer[position]);
                position += 2;
                return value;
            }
    	}
    }


    /**
     * Returns true if this buffer was used in the last utterance.
     *
     * @return true if this buffer was used in the last utterance
     */
    public boolean getUsed() {
        return used;
    }


    /**
     * Sets whether this buffer was used in the last utterance
     *
     * @param used true if this buffer was used in the last utterance, false otherwise
     */
    public void setUsed(boolean used) {
        this.used = used;
    }
    
    
    /**
     * Finds the NGram probability ID for the given nth word in a NGram.
     *
     * @param nthWordID the ID of the nth word
     * @return the NGram Probability ID of the given nth word
     */
    public int findProbabilityID(int nthWordID) {
        int mid, start = 0, end = getNumberNGrams();

        int nGram = -1;

        while ((end - start) > 0) {
            mid = (start + end) / 2;
            int midWordID = getWordID(mid);
            if (midWordID < nthWordID) {
                start = mid + 1;
            } else if (midWordID > nthWordID) {
                end = mid;
            } else {
                nGram = getProbabilityID(mid);
                break;
            }
        }
        return nGram;
    }


    /**
     * Returns the NGramProbability of the nth follower.
     *
     * @param nthFollower which follower
     * @return the NGramProbability of the nth follower
     */
    public int getProbabilityID(int nthFollower) {
    	int nthPosition = 0;
    	
    	nthPosition = nthFollower * LargeNGramModel.BYTES_PER_NGRAM * ((is32bits) ? 4 : 2);
    	setPosition(nthPosition + ((is32bits) ? 4 : 2)); // to skip the word ID
    	
        return readBytesAsInt();
    }
    
    
    /**
     * Finds the NGram probabilities for the given nth word in a NGram.
     *
     * @param nthWordID the ID of the nth word
     * @return the NGramProbability of the given nth word
     */
    public NGramProbability findNGram(int nthWordID) {

        int mid, start = 0, end = getNumberNGrams() - 1;
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
     * Finds the NGram index for the given nth word in a NGram
     * 
     * @param nthWordID the ID of the nth word
     * @return the NGramIndex of the given nth word
     */
    public int findNGramIndex(int nthWordID) {

        int mid = -1, start = 0, end = getNumberNGrams() - 1;

        while ((end - start) > 0) {
            mid = (start + end) / 2;
            int midWordID = getWordID(mid);
            if (midWordID < nthWordID) {
                start = mid + 1;
            } else if (midWordID > nthWordID) {
                end = mid;
            } else {
                break;
            }
        }

        return mid;
    }
    

    /**
     * Returns the NGramProbability of the nth follower.
     *
     * @param nthFollower which follower
     * @return the NGramProbability of the nth follower
     */
    public NGramProbability getNGramProbability(int nthFollower) {
    	int nthPosition = 0, wordID = 0, probID = 0, backoffID = 0, firstNGram = 0;
    	
    	nthPosition = nthFollower * LargeNGramModel.BYTES_PER_NGRAM * ((is32bits) ? 4 : 2);
        
        setPosition(nthPosition);
        
        wordID = readBytesAsInt();
        probID = readBytesAsInt();
        backoffID = readBytesAsInt();
        firstNGram = readBytesAsInt();
            
        return (new NGramProbability(nthFollower, wordID, probID, backoffID, firstNGram));
    }
}

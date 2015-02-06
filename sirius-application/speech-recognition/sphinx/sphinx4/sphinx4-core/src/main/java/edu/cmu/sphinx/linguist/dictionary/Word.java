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

package edu.cmu.sphinx.linguist.dictionary;


/** Represents a word, its spelling and its pronunciation. */
public class Word {

    /** The Word representing the unknown word. */
    public static final Word UNKNOWN;


    static {
        Pronunciation[] pros = {Pronunciation.UNKNOWN};
        UNKNOWN = new Word("<unk>", pros, false);
        Pronunciation.UNKNOWN.setWord(UNKNOWN);
    }


    private final String spelling;               // the spelling of the word
    private final Pronunciation[] pronunciations; // pronunciations of this word
    private final boolean isFiller;


    /**
     * Creates a Word
     *
     * @param spelling       the spelling of this word
     * @param pronunciations the pronunciations of this word
     * @param isFiller       true if the word is a filler word
     */
    public Word(String spelling, Pronunciation[] pronunciations,
                boolean isFiller) {
        this.spelling = spelling;
        this.pronunciations = pronunciations;
        this.isFiller = isFiller;
    }


    /**
     * Returns the spelling of the word.
     *
     * @return the spelling of the word
     */
    public String getSpelling() {
        return spelling;
    }


    /**
     * Determines if this is a filler word
     *
     * @return <code>true</code> if this word is a filler word, otherwise it returns <code>false</code>
     */
    public boolean isFiller() {
        return isFiller;
    }


    /**
     * Returns true if this word is an end of sentence word
     *
     * @return true if the word matches Dictionary.SENTENCE_END_SPELLING
     */
    public boolean isSentenceEndWord() {
        return Dictionary.SENTENCE_END_SPELLING.equals(this.spelling);
    }


    /**
     * Returns true if this word is a start of sentence word
     *
     * @return true if the word matches Dictionary.SENTENCE_START_SPELLING
     */
    public boolean isSentenceStartWord() {
        return Dictionary.SENTENCE_START_SPELLING.equals(this.spelling);
    }


    /**
     * Retrieves the pronunciations of this word
     *
     * @param wordClassification the classification of the word (typically part of speech classification) or null if all
     *                           word classifications are acceptable. The word classification must be one of the set
     *                           returned by <code>Dictionary.getPossibleWordClassifications</code>
     * @return the pronunciations of this word
     */
    public Pronunciation[] getPronunciations
            (WordClassification wordClassification) {
        return pronunciations;
    }


    /**
     * Retrieves the pronunciations of this word
     *
     * @return the pronunciations of this word
     */
    public Pronunciation[] getPronunciations() {
        return pronunciations;
    }


    /**
     * Get the highest probability pronunciation for a word
     *
     * @return the highest probability pronunciation
     */
    public Pronunciation getMostLikelyPronunciation() {
        float bestScore = Float.NEGATIVE_INFINITY;
        Pronunciation best = null;
        for (Pronunciation pronunciation : pronunciations) {
            if (pronunciation.getProbability() > bestScore) {
                bestScore = pronunciation.getProbability();
                best = pronunciation;
            }
        }
        return best;
    }


    @Override
    public int hashCode() {
        return spelling.hashCode();
    }


    @Override
    public boolean equals(Object obj) {
        return obj instanceof Word && spelling.equals(((Word) obj).spelling);

    }


    /**
     * Returns a string representation of this word, which is the spelling
     *
     * @return the spelling of this word
     */
    @Override
    public String toString() {
        return spelling;
    }
}


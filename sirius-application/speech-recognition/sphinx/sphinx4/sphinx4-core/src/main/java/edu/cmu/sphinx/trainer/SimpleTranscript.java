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

import java.util.StringTokenizer;

/** Provides mechanisms for accessing a transcription. */
public class SimpleTranscript implements Transcript {

    private String transcript;              // the transcript
    private Dictionary dictionary;   // the dictionary
    boolean isExact;                        // is exact transcription?
    private boolean wasInitialized; // Has this object been initialized?
    private StringTokenizer words;          // string tokenizer for current transcription.
    private String wordSeparator;           // word separators


    /**
     * Constructor for the SimpleTranscript.
     *
     * @param transcript this transcript's text
     */
    public SimpleTranscript(String transcript) {
        if (!wasInitialized) {
            initialize(null, false);
        }
        this.transcript = transcript;
    }


    /**
     * Constructor for the SimpleTranscript.
     *
     * @param dictionary this transcript's dictionary
     * @param isExact    whether the transcription is exact
     */
    public SimpleTranscript(Dictionary dictionary, boolean isExact) {
        initialize(dictionary, isExact);
    }


    /**
     * Constructor for the SimpleTranscript.
     *
     * @param transcript    this transcript's text
     * @param dictionary    this transcript's dictionary
     * @param isExact       whether the transcription is exact
     * @param wordSeparator string containing the word separator characters.
     */
    public SimpleTranscript(String transcript, Dictionary dictionary,
                            boolean isExact, String wordSeparator) {
        this.transcript = transcript;
        this.dictionary = dictionary;
        this.isExact = isExact;
        this.wordSeparator = wordSeparator;
    }


    /**
     * Constructor for the SimpleTranscript.
     *
     * @param transcript this transcript's text
     * @param dictionary this transcript's dictionary
     * @param isExact    whether the transcription is exact
     */
    public SimpleTranscript(String transcript, Dictionary dictionary,
                            boolean isExact) {
        this.transcript = transcript;
        this.dictionary = dictionary;
        this.isExact = isExact;
        this.wordSeparator = " \t\n\r\f"; // the white spaces
    }


    /**
     * Initializes the SimpleTranscript with dictionary and exact flag.
     *
     * @param dictionary this transcript's dictionary
     * @param isExact    whether the transcription is exact
     */
    public void initialize(Dictionary dictionary, boolean isExact) {
        this.dictionary = dictionary;
        this.isExact = isExact;
        wasInitialized = true;
    }


    /**
     * Gets the transcription.
     *
     * @return current transcription string.
     */
    public String getTranscriptText() {
        return transcript;
    }


    /**
     * Gets the transcript's dictionary.
     *
     * @return current dictionary.
     */
    public Dictionary getDictionary() {
        return dictionary;
    }


    /**
     * Returns whether the transcript is exact.
     *
     * @return true is transcription is exact (has been forced aligned)
     */
    public boolean isExact() {
        return isExact;
    }


    /**
     * Get the number of words in the transcription.
     *
     * @return number of words in the transcription.
     */
    public int numberOfWords() {
        return words.countTokens();
    }


    /** Start the iterator for the words in the transcription. */
    public void startWordIterator() {
        words = new StringTokenizer(transcript, wordSeparator);
    }


    /**
     * Return whether there are more words.
     *
     * @return whether there are more words.
     */
    public boolean hasMoreWords() {
        return words.hasMoreTokens();
    }


    /**
     * Returns the next word.
     *
     * @return next word in the transcription.
     */
    public String nextWord() {
        return words.nextToken();
    }


    /**
     * Returns a string representation of this transcript.
     *
     * @return the string representation
     */
    @Override
    public String toString() {
        String result = "";

        result = "Dict: " + dictionary + " : transcript ";
        if (isExact) {
            result += "IS exact: ";
        } else {
            result += "is NOT exact: ";
        }
        result += transcript;
        return result;
    }
}

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

package edu.cmu.sphinx.linguist.flat;

import edu.cmu.sphinx.linguist.WordSearchState;
import edu.cmu.sphinx.linguist.dictionary.Pronunciation;


/** Represents a pronunciation in an SentenceHMMS */
@SuppressWarnings("serial")
public class PronunciationState extends SentenceHMMState implements        WordSearchState {

    private final Pronunciation pronunciation;

    /**
     * Creates a PronunciationState
     *
     * @param parent the parent word of the current pronunciation
     * @param which  the pronunciation of interest
     */
    public PronunciationState(WordState parent, int which) {
        super("P", parent, which);

        pronunciation = parent.getWord().getPronunciations(null)[which];
    }


    /**
     * Creates a PronunciationState
     *
     * @param name  the name of the pronunciation associated with this state
     * @param p     the pronunciation
     * @param which the index for the pronunciation
     */
    public PronunciationState(String name, Pronunciation p, int which) {
        super(name, null, which);
        pronunciation = p;
    }


    /**
     * Gets the pronunciation associated with this state
     *
     * @return the pronunciation
     */
    public Pronunciation getPronunciation() {
        return pronunciation;
    }


    /**
     * Retrieves a short label describing the type of this state. Typically, subclasses of SentenceHMMState will
     * implement this method and return a short (5 chars or less) label
     *
     * @return the short label.
     */
    @Override
    public String getTypeLabel() {
        return "Pron";
    }


    /**
     * Returns the state order for this state type
     *
     * @return the state order
     */
    @Override
    public int getOrder() {
        return 4;
    }


    /**
     * Returns true if this PronunciationState indicates the start of a word. Returns false if this PronunciationState
     * indicates the end of a word.
     *
     * @return true if this PronunciationState indicates the start of a word, false if this PronunciationState indicates
     *         the end of a word
     */
    @Override
    public boolean isWordStart() {
        return true;
    }
}



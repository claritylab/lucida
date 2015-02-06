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

package edu.cmu.sphinx.linguist;

import edu.cmu.sphinx.linguist.dictionary.Pronunciation;

/** Represents a single word state in a language search space */
public interface WordSearchState extends SearchState {

    /**
     * Gets the word (as a pronunciation)
     *
     * @return the word
     */
    Pronunciation getPronunciation();


    /**
     * Returns true if this WordSearchState indicates the start of a word. Returns false if this WordSearchState
     * indicates the end of a word.
     *
     * @return true if this WordSearchState indicates the start of a word, false if this WordSearchState indicates the
     *         end of a word
     */
    public boolean isWordStart();
}

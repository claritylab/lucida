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

/** Represents a single state in a language search space */
public interface SearchState {

    /**
     * Gets a successor to this search state
     *
     * @return the set of successors
     */
    SearchStateArc[] getSuccessors();


    /**
     * Determines if this is an emitting state
     *
     * @return <code>true</code> if the state is an emitting state
     */
    boolean isEmitting();


    /**
     * Determines if this is a final state
     *
     * @return <code>true</code> if the state is a final state
     */
    boolean isFinal();


    /**
     * Returns a pretty version of the string representation for this object
     *
     * @return a pretty string
     */
    String toPrettyString();


    /**
     * Returns a unique signature for this state
     *
     * @return the signature for the state
     */
    String getSignature();


    /**
     * Gets the word history for this state
     *
     * @return the word history
     */
    WordSequence getWordHistory();


    /**
     * Returns the lex tree state
     *
     * @return the lex tree state
     */
    Object getLexState();


    /**
     * Returns the order of this particular state
     *
     * @return the state order for this state
     */
    int getOrder();
}

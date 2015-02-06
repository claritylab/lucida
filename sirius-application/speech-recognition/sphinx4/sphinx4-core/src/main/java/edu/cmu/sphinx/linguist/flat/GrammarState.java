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


import edu.cmu.sphinx.linguist.language.grammar.GrammarNode;

/** Represents a non-emitting sentence hmm state */
@SuppressWarnings("serial")
public class GrammarState extends SentenceHMMState {

    private final GrammarNode grammarNode;


    /**
     * Creates a GrammarState
     *
     * @param node the GrammarNode associated with this state
     */
    public GrammarState(GrammarNode node) {
        super("G", null, node.getID());
        this.grammarNode = node;
        setFinalState(grammarNode.isFinalNode());
    }


    /**
     * Gets the grammar node associated with this state
     *
     * @return the grammar node
     */
    public GrammarNode getGrammarNode() {
        return grammarNode;
    }


    /**
     * Retrieves a short label describing the type of this state. Typically, subclasses of SentenceHMMState will
     * implement this method and return a short (5 chars or less) label
     *
     * @return the short label.
     */
    @Override
    public String getTypeLabel() {
        return "Gram";
    }


    /**
     * Returns the state order for this state type
     *
     * @return the state order
     */
    @Override
    public int getOrder() {
        return 3;
    }
}



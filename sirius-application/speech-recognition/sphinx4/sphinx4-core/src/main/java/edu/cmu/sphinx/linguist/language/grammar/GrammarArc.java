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

package edu.cmu.sphinx.linguist.language.grammar;


/**
 * Represents a single transition out of a grammar node. The grammar represented is a stochastic grammar, each
 * transition has a probability associated with it. The probabilities are relative and are not necessarily constrained
 * to total 1.0.
 * <p/>
 * Note that all probabilities are maintained in the LogMath log base
 */
public class GrammarArc {

    private GrammarNode grammarNode;
    private float logProbability;

    /**
     * Create a grammar arc
     *
     * @param grammarNode    the node that this arc points to
     * @param logProbability the log probability of following this arc
     */
    public GrammarArc(GrammarNode grammarNode, float logProbability) {
        assert grammarNode != null;
        this.grammarNode = grammarNode;
        this.logProbability = logProbability;
    }


    /**
     * Retrieves the destination node for this transition
     *
     * @return the destination node
     */
    public GrammarNode getGrammarNode() {
        return grammarNode;
    }


    /**
     * Retrieves the probability for this transition
     *
     * @return the log probability for this transition
     */
    public float getProbability() {
        return logProbability;
    }
}


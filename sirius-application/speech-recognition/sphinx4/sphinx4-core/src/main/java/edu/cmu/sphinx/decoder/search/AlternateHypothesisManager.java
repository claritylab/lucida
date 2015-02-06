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

package edu.cmu.sphinx.decoder.search;


import edu.cmu.sphinx.decoder.scorer.Scoreable;

import java.util.*;

/**
 * Manager for pruned hypothesis
 * 
 * @author Joe Woelfel
 */
public class AlternateHypothesisManager {

    private final Map<Token, List<Token>> viterbiLoserMap = new HashMap<Token, List<Token>>();
    private final int maxEdges;


    /**
     * Creates an alternate hypotheses manager
     *
     * @param maxEdges the maximum edges allowed
     */
    public AlternateHypothesisManager(int maxEdges) {
        this.maxEdges = maxEdges;
    }


    /**
     * Collects adds alternate predecessors for a token that would have lost because of viterbi.
     *
     * @param token       - a token that has an alternate lower scoring predecessor that still might be of interest
     * @param predecessor - a predecessor that scores lower than token.getPredecessor().
     */

    public void addAlternatePredecessor(Token token, Token predecessor) {
        assert predecessor != token.getPredecessor();
        List<Token> list = viterbiLoserMap.get(token);
        if (list == null) {
            list = new ArrayList<Token>();
            viterbiLoserMap.put(token, list);
        }
        list.add(predecessor);
    }


    /**
     * Returns a list of alternate predecessors for a token.
     *
     * @param token - a token that may have alternate lower scoring predecessor that still might be of interest
     * @return A list of predecessors that scores lower than token.getPredecessor().
     */
    public List<Token> getAlternatePredecessors(Token token) {
        return viterbiLoserMap.get(token);
    }


    /** Purge all but max number of alternate preceding token hypotheses. */
    public void purge() {

        int max = maxEdges - 1;

        for (Map.Entry<Token, List<Token>> entry : viterbiLoserMap.entrySet()) {
            List<Token> list = entry.getValue();
            Collections.sort(list, Scoreable.COMPARATOR);
            List<Token> newList = list.subList(0, list.size() > max ? max : list.size());
            viterbiLoserMap.put(entry.getKey(), newList);
        }
    }


    /**
     * Change the successor from one token to another
     *
     * @param newSuccessor the new successor token
     * @param oldSuccessor the old successor token
     */
    public void changeSuccessor(Token newSuccessor, Token oldSuccessor) {
        List<Token> list = viterbiLoserMap.get(oldSuccessor);
        viterbiLoserMap.put(newSuccessor, list);
        viterbiLoserMap.remove(oldSuccessor);
    }


	public boolean hasAlternatePredecessors(Token token) {
		return viterbiLoserMap.containsKey(token);
	}
}


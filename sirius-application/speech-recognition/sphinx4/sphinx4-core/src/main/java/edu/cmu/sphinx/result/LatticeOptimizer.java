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
package edu.cmu.sphinx.result;

import java.util.List;
import java.util.ArrayList;

/**
 * Class used to collapse all equivalent paths in a Lattice.  Results in a Lattices that is deterministic (no Node has
 * Edges to two or more equivalent Nodes), and minimal (no Node has Edge from two or more equivalent Nodes).
 */

public class LatticeOptimizer {

    protected final Lattice lattice;


    /**
     * Create a new Lattice optimizer
     *
     * @param lattice
     */
    public LatticeOptimizer(Lattice lattice) {
        this.lattice = lattice;
    }


    /**
     * Code for optimizing Lattices.  An optimal lattice has all the same paths as the original, but with fewer nodes
     * and edges
     * <p/>
     * Note that these methods are all in Lattice so that it is easy to change the definition of "equivalent" nodes and
     * edges.  For example, an equivalent node might have the same word, but start or end at a different time.
     * <p/>
     * To experiment with other definitions of equivalent, just create a superclass of Lattice.
     */
    public void optimize() {
        //System.err.println("***");
        //lattice.dumpAllPaths();
        //System.err.println("***");

        optimizeForward();

        //System.err.println("***");
        //lattice.dumpAllPaths();
        //System.err.println("***");

        optimizeBackward();

        //System.err.println("***");
        //lattice.dumpAllPaths();
        //System.err.println("***");

    }


    /**
     * Make the Lattice deterministic, so that no node has multiple outgoing edges to equivalent nodes.
     * <p/>
     * Given two edges from the same node to two equivalent nodes, replace with one edge to one node with outgoing edges
     * that are a union of the outgoing edges of the old two nodes.
     * <p/>
     * A --> B --> C \--> B' --> Y
     * <p/>
     * where B and B' are equivalent.
     * <p/>
     * is replaced with
     * <p/>
     * A --> B" --> C \--> Y
     * <p/>
     * where B" is the merge of B and B'
     * <p/>
     * Note that equivalent nodes must have the same incomming edges. For example
     * <p/>
     * A --> B \ \ X --> B'
     * <p/>
     * B and B' would not be equivalent because the incomming edges are different
     */
    protected void optimizeForward() {
        //System.err.println("*** Optimizing forward ***");

        boolean moreChanges = true;
        while (moreChanges) {
            moreChanges = false;
            // search for a node that can be optimized
            // note that we use getCopyOfNodes to avoid concurrent changes to nodes
            for (Node n : lattice.getCopyOfNodes()) {
                // we are iterating down a list of node before optimization
                // previous iterations may have removed nodes from the list
                // therefore we have to check that the node stiff exists
                if (lattice.hasNode(n)) {
                    moreChanges |= optimizeNodeForward(n);
                }
            }
        }
    }


    /**
     * Look for 2 "to" edges to equivalent nodes.  Replace the edges with one edge to one node that is a merge of the
     * equivalent nodes
     * <p/>
     * nodes are equivalent if they have equivalent from edges, and the same label
     * <p/>
     * merged nodes have a union of "from" and "to" edges
     *
     * @param n
     * @return true if Node n required an optimize forward
     */
    protected boolean optimizeNodeForward(Node n) {
        assert lattice.hasNode(n);

        List<Edge> leavingEdges = new ArrayList<Edge>(n.getLeavingEdges());
        for (int j = 0; j < leavingEdges.size(); j++) {
            Edge e = leavingEdges.get(j);
            for (int k = j + 1; k < leavingEdges.size(); k++) {
                Edge e2 = leavingEdges.get(k);

                /*
                 * If these are not the same edge, and they point to
                 * equivalent nodes, we have a hit, return true
                 */
                assert e != e2;
                if (equivalentNodesForward(e.getToNode(), e2.getToNode())) {
                    mergeNodesAndEdgesForward(n, e, e2);
                    return true;
                }
            }
        }
        /*
         * return false if we did not get a hit
         */
        return false;
    }


    /**
     * nodes are equivalent forward if they have "from" edges from the same nodes, and have equivalent labels (Token,
     * start/end times)
     *
     * @param n1
     * @param n2
     * @return true if n1 and n2 are "equivalent forwards"
     */
    protected boolean equivalentNodesForward(Node n1, Node n2) {

        assert lattice.hasNode(n1);
        assert lattice.hasNode(n2);

        // do the labels match?
        if (!equivalentNodeLabels(n1, n2)) return false;

        // if they have different number of "from" edges they are not equivalent
        // or if there is a "from" edge with no match then the nodes are not
        // equivalent
        return n1.hasEquivalentEnteringEdges(n2);
    }


    /**
     * given edges e1 and e2 from node n to nodes n1 and n2
     * <p/>
     * merge e1 and e2, that is, merge the scores of e1 and e2 create n' that is a merge of n1 and n2 add n' add edge e'
     * from n to n'
     * <p/>
     * remove n1 and n2 and all associated edges
     *
     * @param n
     * @param e1
     * @param e2
     */
    protected void mergeNodesAndEdgesForward(Node n, Edge e1, Edge e2) {
        assert lattice.hasNode(n);
        assert lattice.hasEdge(e1);
        assert lattice.hasEdge(e2);

        assert e1.getFromNode() == n;
        assert e2.getFromNode() == n;

        Node n1 = e1.getToNode();
        Node n2 = e2.getToNode();

        assert n1.hasEquivalentEnteringEdges(n1);
        assert n1.getWord().equals(n2.getWord());

        // merge the scores of e1 and e2 into e1
        e1.setAcousticScore(mergeAcousticScores
                (e1.getAcousticScore(), e2.getAcousticScore()));
        e1.setLMScore(mergeLanguageScores(e1.getLMScore(),
                e2.getLMScore()));

        // add n2's edges to n1
        for (Edge e : n2.getLeavingEdges()) {
            e2 = n1.getEdgeToNode(e.getToNode());
            if (e2 == null) {
                lattice.addEdge(n1, e.getToNode(),
                        e.getAcousticScore(), e.getLMScore());
            } else {
                // if we got here then n1 and n2 had edges to the same node
                // choose the edge with best score
                e2.setAcousticScore
                        (mergeAcousticScores
                                (e.getAcousticScore(), e2.getAcousticScore()));
                e2.setLMScore(mergeLanguageScores(e.getLMScore(),
                        e2.getLMScore()));
            }
        }

        // remove n2 and all associated edges
        lattice.removeNodeAndEdges(n2);
    }


    /**
     * Minimize the Lattice deterministic, so that no node has multiple incoming edges from equivalent nodes.
     * <p/>
     * Given two edges from equivalent nodes to a single nodes, replace with one edge from one node with incoming edges
     * that are a union of the incoming edges of the old two nodes.
     * <p/>
     * A --> B --> C X --> B' --/
     * <p/>
     * where B and B' are equivalent.
     * <p/>
     * is replaced with
     * <p/>
     * A --> B" --> C X --/
     * <p/>
     * where B" is the merge of B and B'
     * <p/>
     * Note that equivalent nodes must have the same outgoing edges. For example
     * <p/>
     * A --> X \ \ \ A' --> B
     * <p/>
     * A and A' would not be equivalent because the outgoing edges are different
     */
    protected void optimizeBackward() {
        //System.err.println("*** Optimizing backward ***");

        boolean moreChanges = true;
        while (moreChanges) {
            moreChanges = false;
            // search for a node that can be optimized
            // note that we use getCopyOfNodes to avoid concurrent changes to nodes
            for (Node n : lattice.getCopyOfNodes()) {
                // we are iterating down a list of node before optimization
                // previous iterations may have removed nodes from the list
                // therefore we have to check that the node stiff exists
                if (lattice.hasNode(n)) {
                    moreChanges |= optimizeNodeBackward(n);
                }
            }
        }
    }


    /**
     * Look for 2 entering edges from equivalent nodes.  Replace the edges with one edge to one new node that is a merge
     * of the equivalent nodes Nodes are equivalent if they have equivalent to edges, and the same label. Merged nodes
     * have a union of entering and leaving edges
     *
     * @param n
     * @return true if Node n required optimizing backwards
     */
    protected boolean optimizeNodeBackward(Node n) {
        List<Edge> enteringEdges = new ArrayList<Edge>(n.getEnteringEdges());
        for (int j = 0; j < enteringEdges.size(); j++) {
            Edge e = enteringEdges.get(j);
            for (int k = j + 1; k < n.getEnteringEdges().size(); k++) {
                Edge e2 = enteringEdges.get(k);

                /*
                 * If these are not the same edge, and they point to
                 * equivalent nodes, we have a hit, return true
                 */
                assert e != e2;
                if (equivalentNodesBackward(e.getFromNode(),
                        e2.getFromNode())) {
                    mergeNodesAndEdgesBackward(n, e, e2);
                    return true;
                }
            }
        }
        /*
         * return false if we did not get a hit
         */
        return false;
    }


    /**
     * nodes are equivalent backward if they have "to" edges to the same nodes, and have equivalent labels (Token,
     * start/end times)
     *
     * @param n1
     * @param n2
     * @return true if n1 and n2 are "equivalent backwards"
     */
    protected boolean equivalentNodesBackward(Node n1, Node n2) {

        assert lattice.hasNode(n1);
        assert lattice.hasNode(n2);

        // do the labels match?
        if (!equivalentNodeLabels(n1, n2)) return false;

        // if they have different number of "to" edges they are not equivalent
        // or if there is a "to" edge with no match then the nodes are not equiv
        return n1.hasEquivalentLeavingEdges(n2);
    }


    /**
     * Is the contents of these Node equivalent?
     *
     * @param n1
     * @param n2
     * @return true if n1 and n2 have "equivalent labels"
     */
    protected boolean equivalentNodeLabels(Node n1, Node n2) {
        return (n1.getWord().equals(n2.getWord()) &&
                (n1.getBeginTime() == n2.getBeginTime() &&
                        n1.getEndTime() == n2.getEndTime()));
    }


    /**
     * given edges e1 and e2 to node n from nodes n1 and n2
     * <p/>
     * merge e1 and e2, that is, merge the scores of e1 and e2 create n' that is a merge of n1 and n2 add n' add edge e'
     * from n' to n
     * <p/>
     * remove n1 and n2 and all associated edges
     *
     * @param n
     * @param e1
     * @param e2
     */
    protected void mergeNodesAndEdgesBackward(Node n, Edge e1, Edge e2) {
        assert lattice.hasNode(n);
        assert lattice.hasEdge(e1);
        assert lattice.hasEdge(e2);

        assert e1.getToNode() == n;
        assert e2.getToNode() == n;

        Node n1 = e1.getFromNode();
        Node n2 = e2.getFromNode();

        assert n1.hasEquivalentLeavingEdges(n2);
        assert n1.getWord().equals(n2.getWord());

        // merge the scores of e1 and e2 into e1
        e1.setAcousticScore(mergeAcousticScores(e1.getAcousticScore(),
                e2.getAcousticScore()));
        e1.setLMScore(mergeLanguageScores(e1.getLMScore(),
                e2.getLMScore()));

        // add n2's "from" edges to n1
        for (Edge e : n2.getEnteringEdges()) {
            e2 = n1.getEdgeFromNode(e.getFromNode());
            if (e2 == null) {
                lattice.addEdge(e.getFromNode(), n1,
                        e.getAcousticScore(), e.getLMScore());
            } else {
                // if we got here then n1 and n2 had edges from the same node
                // choose the edge with best score
                e2.setAcousticScore
                        (mergeAcousticScores(e.getAcousticScore(),
                                e2.getAcousticScore()));
                e2.setLMScore(mergeLanguageScores(e.getLMScore(),
                        e2.getLMScore()));
            }
        }

        // remove n2 and all associated edges
        lattice.removeNodeAndEdges(n2);
    }


    /** Remove all Nodes that have no Edges to them (but not <s>) */
    protected void removeHangingNodes() {
        for (Node n : lattice.getCopyOfNodes()) {
            if (lattice.hasNode(n)) {
                if (n == lattice.getInitialNode()) {

                } else if (n == lattice.getTerminalNode()) {

                } else {
                    if (n.getLeavingEdges().isEmpty()
                            || n.getEnteringEdges().isEmpty()) {
                        lattice.removeNodeAndEdges(n);
                        removeHangingNodes();
                        return;
                    }
                }
            }
        }
    }


    /**
     * Provides a single method to merge acoustic scores, so that changes to how acoustic score are merged can be made
     * at one point only.
     *
     * @param score1 the first acoustic score
     * @param score2 the second acoustic score
     * @return the merged acoustic score
     */
    private double mergeAcousticScores(double score1, double score2) {
        // return lattice.getLogMath().addAsLinear(score1, score2);
        return Math.max(score1, score2);
    }


    /**
     * Provides a single method to merge language scores, so that changes to how language score are merged can be made
     * at one point only.
     *
     * @param score1 the first language score
     * @param score2 the second language score
     * @return the merged language score
     */
    private double mergeLanguageScores(double score1, double score2) {
        // return lattice.getLogMath().addAsLinear(score1, score2);
        return Math.max(score1, score2);
    }


    /**
     * Self test for LatticeOptimizer
     *
     * @param args
     */
    public static void main(String[] args) {
        Lattice lattice = new Lattice(args[0]);

        LatticeOptimizer optimizer = new LatticeOptimizer(lattice);

        optimizer.optimize();

        lattice.dump(args[1]);
    }
}

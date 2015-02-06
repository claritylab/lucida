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

import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.util.LogMath;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.*;

/**
 * <p/>
 * A node is part of Lattices, representing the theory that a word was spoken over a given period of time. A node also
 * has a set of entering and leaving {@link edu.cmu.sphinx.result.Edge edges}, connecting it to other nodes. One can get
 * and set the beginning and end frames of the word via the getBeginTime and getEndTime methods. When setting these
 * times, the beginning time must be earlier or equal to the end time, otherwise an error will be thrown. </p>
 * <p/>
 * <p/>
 * The posterior probability of any word in a word lattice is the probability that the node representing that word
 * occurs on any path through the lattice. It is usually computed as the ratio of the total likelihood scores of all
 * paths through the lattice that pass through the node, to the total likelihood score of all paths through the lattice.
 * Path scores are usually computed using the acoustic likelihoods of the nodes, although language scores can also be
 * incorporated. The posterior probabilities of an entire lattice is usually computed efficiently using the
 * Forward-Backward Algorithm. Refer to the {@link edu.cmu.sphinx.result.Lattice#computeNodePosteriors
 * computeNodePosteriors} method in the Lattice class for details. </p>
 */
public class Node {

    // used to generate unique IDs for new Nodes.
    private static int nodeCount;

    private String id;
    private Word word;
    // TODO: use TimeFrame
    private int beginTime = -1;
    private int endTime = -1;
    private List<Edge> enteringEdges;
    private List<Edge> leavingEdges;
    private double forwardScore;
    private double backwardScore;
    private double posterior;
    private Node bestPredecessor;
    private double viterbiScore;
    private Set<Node> descendants;


    {
        enteringEdges = new ArrayList<Edge>();
        leavingEdges = new ArrayList<Edge>();
        nodeCount++;
    }


    /**
     * Create a new Node
     *
     * @param word      the word of this node
     * @param beginTime the start time of the word
     * @param endTime   the end time of the word
     */
    protected Node(Word word, int beginTime, int endTime) {
        this(getNextNodeId(), word, beginTime, endTime);
    }


    /**
     * Create a new Node with given ID. Used when creating a Lattice from a .LAT file
     *
     * @param id
     * @param word
     * @param beginTime
     * @param endTime
     */
    protected Node(String id, Word word, int beginTime, int endTime) {
        this.id = id;
        this.word = word;
        this.beginTime = beginTime;
        this.endTime = endTime;
        if (endTime != -1) {
            if (beginTime > endTime) {
                throw new Error("Begin time (" + beginTime +
                        ") later than end time (" + endTime + ')');
            }
        }
        this.forwardScore = LogMath.LOG_ZERO;
        this.backwardScore = LogMath.LOG_ZERO;
        this.posterior = LogMath.LOG_ZERO;
    }


    /**
     * Get a unique ID for a new Node. Used when creating a Lattice from a .LAT file
     *
     * @return the unique ID for a new node
     */
    protected static String getNextNodeId() {
        return Integer.toString(nodeCount);
    }


    /**
     * Test if a node has an Edge to a Node
     *
     * @param n
     * @return unique Node ID
     */
    protected boolean hasEdgeToNode(Node n) {
        return getEdgeToNode(n) != null;
    }


    /**
     * given a node find the edge to that node
     *
     * @param n the node of interest
     * @return the edge to that node or <code> null</code>  if no edge could be found.
     */
    public Edge getEdgeToNode(Node n) {
        for (Edge e : leavingEdges) {
            if (e.getToNode() == n) {
                return e;
            }
        }
        return null;
    }


    /**
     * Test is a Node has an Edge from a Node
     *
     * @param n
     * @return true if this node has an Edge from n
     */
    protected boolean hasEdgeFromNode(Node n) {
        return getEdgeFromNode(n) != null;
    }


    /**
     * given a node find the edge from that node
     *
     * @param n the node of interest
     * @return the edge from that node or <code> null</code>  if no edge could be found.
     */
    public Edge getEdgeFromNode(Node n) {
        for (Edge e : enteringEdges) {
            if (e.getFromNode() == n) {
                return e;
            }
        }
        return null;
    }


    /**
     * Test if a Node has all Edges from the same Nodes and another Node.
     *
     * @param n
     * @return true if this Node has Edges from the same Nodes as n
     */
    protected boolean hasEquivalentEnteringEdges(Node n) {
        if (enteringEdges.size() != n.getEnteringEdges().size()) {
            return false;
        }
        for (Edge e : enteringEdges) {
            Node fromNode = e.getFromNode();
            if (!n.hasEdgeFromNode(fromNode)) {
                return false;
            }
        }
        return true;
    }


    /**
     * Test if a Node has all Edges to the same Nodes and another Node.
     *
     * @param n the node of interest
     * @return true if this Node has all Edges to the sames Nodes as n
     */
    public boolean hasEquivalentLeavingEdges(Node n) {
        if (leavingEdges.size() != n.getLeavingEdges().size()) {
            return false;
        }
        for (Edge e : leavingEdges) {
            Node toNode = e.getToNode();
            if (!n.hasEdgeToNode(toNode)) {
                return false;
            }
        }
        return true;
    }


    /**
     * Get the Edges to this Node
     *
     * @return Edges to this Node
     */
    public Collection<Edge> getEnteringEdges() {
        return enteringEdges;
    }


    /**
     * Get the Edges from this Node
     *
     * @return Edges from this Node
     */
    public Collection<Edge> getLeavingEdges() {
        return leavingEdges;
    }

    /**
     * Returns a copy of the Edges to this Node, so that the underlying data structure will not be modified.
     *
     * @return a copy of the edges to this node
     */
    public Collection<Edge> getCopyOfEnteringEdges() {
        return new ArrayList<Edge>(enteringEdges);
    }

    /**
     * Returns a copy of the Edges from this Node, so that the underlying data structure will not be modified.
     *
     * @return a copy of the edges from this node
     */
    public Collection<Edge> getCopyOfLeavingEdges() {
        return new ArrayList<Edge>(leavingEdges);
    }

    /**
     * Add an Edge from this Node
     *
     * @param e
     */
    protected void addEnteringEdge(Edge e) {
        enteringEdges.add(e);
    }


    /**
     * Add an Edge to this Node
     *
     * @param e
     */
    protected void addLeavingEdge(Edge e) {
        leavingEdges.add(e);
    }


    /**
     * Remove an Edge from this Node
     *
     * @param e
     */
    protected void removeEnteringEdge(Edge e) {
        enteringEdges.remove(e);
    }


    /**
     * Remove an Edge to this Node
     *
     * @param e the edge to remove
     */
    public void removeLeavingEdge(Edge e) {
        leavingEdges.remove(e);
    }


    /**
     * Get the ID associated with this Node
     *
     * @return the ID
     */
    public String getId() {
        return id;
    }


    /**
     * Get the word associated with this Node
     *
     * @return the word
     */
    public Word getWord() {
        return word;
    }


    /**
     * Get the frame number when the word began
     *
     * @return the begin frame number, or -1 if the frame number is unknown
     */
    public int getBeginTime() {
        if (beginTime == -1) {
            calculateBeginTime();
        }
        return beginTime;
    }


    /**
     * Sets the frame number when the word began. The begin time must be not be later than the time returned by the
     * getEndTime() method, otherwise an error will be thrown.
     *
     * @param beginTime the frame number when the word began
     */
    public void setBeginTime(int beginTime) {
        if (beginTime > getEndTime()) {
            throw new Error("Attempting to set a begin time (" + beginTime +
                    ") that is later than the end time (" +
                    getEndTime() + ").");
        }
        this.beginTime = beginTime;
    }


    /**
     * Get the frame number when the word ends
     *
     * @return the end time, or -1 if the frame number if is unknown
     */
    public int getEndTime() {
        return endTime;
    }


    /**
     * Sets the frame number when the words ended. The end time must not be earlier than the time returned by the
     * getEndTime() method, otherwise an error will be thrown.
     *
     * @param endTime the frame number when the word ended
     */
    public void setEndTime(int endTime) {
        if (getBeginTime() > endTime) {
            throw new Error("Attempting to set an end time (" + endTime +
                    ") that is earlier than the start time (" +
                    getBeginTime() + ").");
        }
        this.endTime = endTime;
    }


    /**
     * Returns a description of this Node that contains the word, the start time, and the end time.
     *
     * @return a description of this Node
     */
    @Override
    public String toString() {
        return ("Node(" + word.getSpelling() + ',' + getBeginTime() + '|' +
                getEndTime() + ')');
    }


    /**
     * Internal routine when dumping Lattices as AiSee files
     *
     * @param f
     * @throws IOException
     */
    void dumpAISee(FileWriter f) throws IOException {
        String posterior = String.valueOf(getPosterior());
        if (getPosterior() == LogMath.LOG_ZERO) {
            posterior = "log zero";
        }
        f.write("node: { title: \"" + id + "\" label: \""
                + getWord() + '[' + getBeginTime() + ',' + getEndTime() +
                " p:" + posterior + "]\" }\n");
    }

    /**
     * Internal routine when dumping Lattices as Graphviz files
     * 
     * @param f
     * @throws IOException
     */
    public void dumpDot(FileWriter f) throws IOException {
        String posterior = String.valueOf(getPosterior());
        if (getPosterior() == LogMath.LOG_ZERO) {
            posterior = "log zero";
        }
        String label = getWord().toString() + '[' + getBeginTime() + ',' + getEndTime() + " p:" + posterior + ']';
        f.write("\tnode" + id + " [ label=\"" + label + "\" ]\n");
    }

    /**
     * Internal routine used when dumping Lattices as .LAT files
     *
     * @param f
     * @throws IOException
     */
    void dump(PrintWriter f) throws IOException {
        f.println("node: " + id + ' ' + word.getSpelling() +
                //" a:" + getForwardProb() + " b:" + getBackwardProb()
                " p:" + getPosterior());
    }


    /**
     * Internal routine used when loading Lattices from .LAT files
     *
     * @param lattice
     * @param tokens
     */
    static void load(Lattice lattice, StringTokenizer tokens) {

        String id = tokens.nextToken();
        String label = tokens.nextToken();

        lattice.addNode(id, label, 0, 0);
    }


    /**
     * Returns the backward score, which is calculated during the computation of the posterior score for this node.
     *
     * @return Returns the backwardScore.
     */
    public double getBackwardScore() {
        return backwardScore;
    }


    /**
     * Sets the backward score for this node.
     *
     * @param backwardScore The backwardScore to set.
     */
    public void setBackwardScore(double backwardScore) {
        this.backwardScore = backwardScore;
    }


    /**
     * Returns the forward score, which is calculated during the computation of the posterior score for this node.
     *
     * @return Returns the forwardScore.
     */
    public double getForwardScore() {
        return forwardScore;
    }


    /**
     * Sets the backward score for this node.
     *
     * @param forwardScore The forwardScore to set.
     */
    public void setForwardScore(double forwardScore) {
        this.forwardScore = forwardScore;
    }


    /**
     * Returns the posterior probability of this node. Refer to the javadocs for this class for a description of
     * posterior probabilities.
     *
     * @return Returns the posterior probability of this node.
     */
    public double getPosterior() {
        return posterior;
    }


    /**
     * Sets the posterior probability of this node. Refer to the javadocs for this class for a description of posterior
     * probabilities.
     *
     * @param posterior The node posterior probability to set.
     */
    public void setPosterior(double posterior) {
        this.posterior = posterior;
    }


    /** @see java.lang.Object#hashCode() */
    @Override
    public int hashCode() {
        return id.hashCode();
    }


    /**
     * Assumes ids are unique node identifiers
     *
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals(Object obj) {
        return obj instanceof Node && id.equals(((Node) obj).getId());
    }


    /**
     * Calculates the begin time of this node, in the event that the begin time was not specified. The begin time is the
     * latest of the end times of its predecessor nodes.
     */
    private void calculateBeginTime() {
        beginTime = 0;
        for (Edge edge : enteringEdges) {
            if (edge.getFromNode().getEndTime() > beginTime) {
                beginTime = edge.getFromNode().getEndTime();
            }
        }
    }


    /**
     * Get the nodes at the other ends of outgoing edges of this node.
     *
     * @return a list of child nodes
     */
    public List<Node> getChildNodes() {
        LinkedList<Node> childNodes = new LinkedList<Node>();
        for (Edge edge : leavingEdges) {
            childNodes.add(edge.getToNode());
        }
        return childNodes;
    }


    protected void cacheDescendants() {
        descendants = new HashSet<Node>();
        cacheDescendantsHelper(this);
    }


    protected void cacheDescendantsHelper(Node n) {
        for (Node child : n.getChildNodes()) {
            if (descendants.contains(child)) {
                continue;
            }
            descendants.add(child);
            cacheDescendantsHelper(child);
        }
    }


    protected boolean isAncestorHelper(List<Node> children, Node node, Set<Node> seenNodes) {
        for (Node n : children) {
            if (seenNodes.contains(n)) {
                continue;
            }
            seenNodes.add(n);
            if (n.equals(node)) {
                return true;
            }
            if (isAncestorHelper(n.getChildNodes(), node, seenNodes)) {
                return true;
            }
        }
        return false;
    }


    /**
     * Check whether this node is an ancestor of another node.
     *
     * @param node the Node to check
     * @return whether this node is an ancestor of the passed in node.
     */
    public boolean isAncestorOf(Node node) {
        if (descendants != null) {
            return descendants.contains(node);
        }
        if (this.equals(node)) {
            return true; // node is its own ancestor
        }
        Set<Node> seenNodes = new HashSet<Node>();
        seenNodes.add(this);
        return isAncestorHelper(this.getChildNodes(), node, seenNodes);
    }


    /**
     * Check whether this node has an ancestral relationship with another node (i.e. either this node is an ancestor of
     * the other node, or vice versa)
     *
     * @param node the Node to check for a relationship
     * @return whether a relationship exists
     */
    public boolean hasAncestralRelationship(Node node) {
        return this.isAncestorOf(node) || node.isAncestorOf(this);
    }


    /**
     * Returns true if the given node is equivalent to this node. Two nodes are equivalent only if they have the same
     * word, the same number of entering and leaving edges, and that their begin and end times are the same.
     *
     * @param other the Node we're comparing to
     * @return true if the Node is equivalent; false otherwise
     */
    public boolean isEquivalent(Node other) {
        return
                ((word.getSpelling().equals(other.getWord().getSpelling()) &&
                        (getEnteringEdges().size() == other.getEnteringEdges().size() &&
                                getLeavingEdges().size() == other.getLeavingEdges().size())) &&
                        (getBeginTime() == other.getBeginTime() &&
                                endTime == other.getEndTime()));
    }


    /**
     * Returns a leaving edge that is equivalent to the given edge. Two edges are eqivalent if Edge.isEquivalent()
     * returns true.
     *
     * @param edge the Edge to compare the leaving edges of this node against
     * @return an equivalent edge, if any; or null if no equivalent edge
     */
    public Edge findEquivalentLeavingEdge(Edge edge) {
        for (Edge e : leavingEdges) {
            if (e.isEquivalent(edge)) {
                return e;
            }
        }
        return null;
    }


    /**
     * Returns the best predecessor for this node.
     *
     * @return Returns the bestPredecessor.
     */
    public Node getBestPredecessor() {
        return bestPredecessor;
    }


    /**
     * Sets the best predecessor of this node.
     *
     * @param bestPredecessor The bestPredecessor to set.
     */
    public void setBestPredecessor(Node bestPredecessor) {
        this.bestPredecessor = bestPredecessor;
    }


    /**
     * Returns the Viterbi score for this node. The Viterbi score is usually computed during the speech recognition
     * process.
     *
     * @return Returns the viterbiScore.
     */
    public double getViterbiScore() {
        return viterbiScore;
    }


    /**
     * Sets the Viterbi score for this node. The Viterbi score is usually computed during the speech recognition
     * process.
     *
     * @param viterbiScore The viterbiScore to set.
     */
    public void setViterbiScore(double viterbiScore) {
        this.viterbiScore = viterbiScore;
    }

}

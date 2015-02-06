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

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/** Defines the basic Node for any graph A generic graph Node must have a list of outgoing edges and an identifier. */
public class Node {

    // Do we really need nodeId and object? Maybe we can use object as
    // the id when we assign a string to it.
    /** The identifier for this Node */
    private String nodeId;

    /** Object contained in this mode. Typically, an HMM state, a senone. */
    private Object object;

    /** The type of node, such as a dummy node or node represented by a specific type of symbol */
    private NodeType nodeType;

    /** The list of incoming edges to this node. */
    private List<Edge> incomingEdges;
    private Iterator<Edge> incomingEdgeIterator;

    /** The list of outgoing edges from this node */
    private List<Edge> outgoingEdges;
    private Iterator<Edge> outgoingEdgeIterator;


    /**
     * Constructor for node when a type and symbol are given.
     *
     * @param nodeType   the type of node.
     * @param nodeSymbol the symbol for this type.
     */
    Node(NodeType nodeType, String nodeSymbol) {
        incomingEdges = new ArrayList<Edge>();
        outgoingEdges = new ArrayList<Edge>();
        this.nodeId = nodeSymbol;
        this.nodeType = nodeType;
        this.object = null;
    }


    /**
     * Constructor for node when a type only is given.
     *
     * @param nodeType the type of node.
     */
    Node(NodeType nodeType) {
        this(nodeType, null);
    }


    /**
     * Assign an object to this node.
     *
     * @param object the object to assign
     */
    public void setObject(Object object) {
        this.object = object;
    }


    /**
     * Retrieves the object associated with this node.
     *
     * @return the object
     */
    public Object getObject() {
        return object;
    }


    /**
     * Method to add an incoming edge. Note that we do not check if the destination node of the incoming edge is
     * identical to this node
     *
     * @param edge the incoming edge
     */
    public void addIncomingEdge(Edge edge) {
        incomingEdges.add(edge);
    }


    /** Start iterator for incoming edges. */
    public void startIncomingEdgeIterator() {
        incomingEdgeIterator = incomingEdges.iterator();
    }


    /**
     * Whether there are more incoming edges.
     *
     * @return if true, there are more incoming edges
     */
    public boolean hasMoreIncomingEdges() {
        return incomingEdgeIterator.hasNext();
    }


    /**
     * Returns the next incoming edge to this node.
     *
     * @return the next edge incoming edge
     */
    public Edge nextIncomingEdge() {
        return incomingEdgeIterator.next();
    }


    /**
     * Returns the size of the incoming edges list.
     *
     * @return the number of incoming edges
     */
    public int incomingEdgesSize() {
        return incomingEdges.size();
    }


    /**
     * Method to add an outgoing edge. Note that we do not check if the source node of the outgoing edge is identical to
     * this node
     *
     * @param edge the outgoing edge
     */
    public void addOutgoingEdge(Edge edge) {
        outgoingEdges.add(edge);
    }


    /** Start iterator for outgoing edges. */
    public void startOutgoingEdgeIterator() {
        outgoingEdgeIterator = outgoingEdges.iterator();
    }


    /**
     * Whether there are more outgoing edges.
     *
     * @return if true, there are more outgoing edges
     */
    public boolean hasMoreOutgoingEdges() {
        return outgoingEdgeIterator.hasNext();
    }


    /**
     * Returns the next outgoing edge from this node.
     *
     * @return the next outgoing edge
     */
    public Edge nextOutgoingEdge() {
        return outgoingEdgeIterator.next();
    }


    /**
     * Returns the size of the outgoing edges list.
     *
     * @return the number of outgoing edges
     */
    public int outgoingEdgesSize() {
        return outgoingEdges.size();
    }


    /**
     * Method to check the type of a node.
     *
     * @return if true, this node is of the type specified
     */
    public boolean isType(String type) {
        return (type.equals(this.nodeType.toString()));
    }


    /**
     * Returns type of a node.
     *
     * @return returns the type of this node
     */
    public NodeType getType() {
        return nodeType;
    }


    /**
     * Returns the ID of a node. Typically, a string representing a word or a phoneme.
     *
     * @return this node's ID
     */
    public String getID() {
        return nodeId;
    }


    /**
     * Validade node. Checks if all nodes have at least one incoming and one outgoing edge.
     *
     * @return if true, node passed validation
     */
    public boolean validate() {
        boolean passed = true;

        if (isType("WORD") || isType("PHONE")) {
            if (nodeId == null) {
                System.out.println("Content null in a WORD node.");
                passed = false;
            }
        }
        if ((incomingEdgesSize() == 0) && (outgoingEdgesSize() == 0)) {
            System.out.println("Node not connected anywhere.");
            passed = false;
        }
        return passed;
    }


    /** Prints out this node. */
    public void print() {
        System.out.print("ID: " + nodeId);
        System.out.print(" Type: " + nodeType + " | ");
        for (startIncomingEdgeIterator();
             hasMoreIncomingEdges();) {
            System.out.print(nextIncomingEdge() + " ");
        }
        System.out.print(" | ");
        for (startOutgoingEdgeIterator();
             hasMoreOutgoingEdges();) {
            System.out.print(nextOutgoingEdge() + " ");
        }
        System.out.println();
    }
}

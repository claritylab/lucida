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

/** Defines the basic Edge for any graph A generic graph edge must have a destination Node and an identifier. */

public class Edge {

    /** The identifier for this edge */
    public String id;

    /** The source node for this edge */
    public Node sourceNode;

    /** The destination node for this edge */
    public Node destinationNode;


    /*
    * Default Constructor
    */
    Edge(Node source, Node destination, String id) {
        this.sourceNode = source;
        this.destinationNode = destination;
        this.id = id;
    }


    /*
    * Constructor given no id.
    */
    Edge(Node source, Node destination) {
        this(source, destination, null);
    }


    /**
     * Sets the destination node for a given edge.
     *
     * @param node the destination node for this edge
     * @see #getDestination
     */
    public void setDestination(Node node) {
        this.destinationNode = node;
    }


    /**
     * Sets source node for a given edge.
     *
     * @param node the source node for this edge
     * @see #getSource
     */
    public void setSource(Node node) {
        this.sourceNode = node;
    }


    /**
     * Gets the destination node for a given edge.
     *
     * @return the destination node
     * @see #setDestination
     */
    public Node getDestination() {
        return destinationNode;
    }


    /**
     * Gets source node for a given edge.
     *
     * @return the source node
     * @see #setSource
     */
    public Node getSource() {
        return sourceNode;
    }


    /**
     * Validate this edge. Checks if source and destination are non-null.
     *
     * @return if true, edge passed validation
     */
    public boolean validate() {
        return ((sourceNode != null) && (destinationNode != null));
    }


    /** Prints out this edge. */
    public void print() {
        System.out.print("ID: " + id);
        System.out.print(" | " + sourceNode);
        System.out.println(" | " + destinationNode);
    }
}

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

/** Implementation of a graph */

public class Graph {

    private ArrayList<Edge> edges; // The list of edges.
    private ArrayList<Node> nodes; // The list of nodes.
    private Iterator<Edge> edgeIterator; // The iterator for the list of edges.
    private Iterator<Node> nodeIterator; // The iterator for the list of nodes.

    /** The initial node in the graph. This has no incoming edges. */
    private Node initialNode;

    /*
     * The final node in the graph. This has no outgoing edges.
     */
    private Node finalNode;


    /** Constructor for class. Creates lists of edges and nodes. */
    public Graph() {
        edges = new ArrayList<Edge>();
        nodes = new ArrayList<Node>();
    }


    /** Set the initial node */
    public void setInitialNode(Node node) throws IllegalArgumentException {
        if (isNodeInGraph(node)) {
            initialNode = node;
        } else {
            throw new IllegalArgumentException("Initial node not in graph");
        }
    }


    /** Set the final node */
    public void setFinalNode(Node node) throws IllegalArgumentException {
        if (isNodeInGraph(node)) {
            finalNode = node;
        } else {
            throw new IllegalArgumentException("Final node not in graph");
        }
    }


    /** Get the initial node */
    public Node getInitialNode() {
        return initialNode;
    }


    /** Get the final node */
    public Node getFinalNode() {
        return finalNode;
    }


    /**
     * Get this graph's size. The size of a graph is the number of nodes in the graph.
     *
     * @return the size of the graph
     */
    public int size() {
        return nodes.size();
    }


    /**
     * Get node at the specified position in the list. The order is the same in which the nodes were entered.
     *
     * @param index index of item to retun
     * @return the item
     */
    public Node getNode(int index) {
        return nodes.get(index);
    }


    /**
     * Gets an array containing the nodes in this graph.
     *
     * @return an array of nodes
     */
    public Node[] nodeToArray() {
        return nodes.toArray(new Node[nodes.size()]);
    }


    /**
     * Gets the index of a particular node in the graph.
     *
     * @param node the node
     * @return the index in this graph, or -1 if not found
     */
    public int indexOf(Node node) {
        return nodes.indexOf(node);
    }


    /**
     * Returns whether the given node is the initial node in this graph.
     *
     * @param node the node we want to compare
     * @return if true, the node is the initial node
     */
    public boolean isInitialNode(Node node) {
        return node == initialNode;
    }


    /**
     * Returns whether the given node is the final node in this graph.
     *
     * @param node the node we want to compare
     * @return if true, the node is the final node
     */
    public boolean isFinalNode(Node node) {
        return node == finalNode;
    }


    /**
     * Link two nodes. If the source or destination nodes are not in the graph, they are added to it. No check is
     * performed to ensure that the nodes are linked to other nodes in the graph.
     */
    public Edge linkNodes(Node sourceNode, Node destinationNode) {
        Edge newLink = new Edge(sourceNode, destinationNode);

        sourceNode.addOutgoingEdge(newLink);
        destinationNode.addIncomingEdge(newLink);

        if (!isNodeInGraph(sourceNode)) {
            addNode(sourceNode);
        }

        if (!isNodeInGraph(destinationNode)) {
            addNode(destinationNode);
        }

        addEdge(newLink);

        return newLink;
    }


    /** Add node to list of nodes. */
    public void addNode(Node node) {
        nodes.add(node);
    }


    /** Add edge to list of nodes. */
    public void addEdge(Edge edge) {
        edges.add(edge);
    }


    /** Check if a node is in the graph. */
    public boolean isNodeInGraph(Node node) {
        return nodes.contains(node);
    }


    /** Check if an edge is in the graph. */
    public boolean isEdgeInGraph(Node edge) {
        return edges.contains(edge);
    }


    /** Start iterator for nodes. */
    public void startNodeIterator() {
        nodeIterator = nodes.iterator();
    }


    /** Whether there are more nodes. */
    public boolean hasMoreNodes() {
        return nodeIterator.hasNext();
    }


    /** Returns next node. */
    public Node nextNode() {
        return nodeIterator.next();
    }


    /** Start iterator for edges. */
    public void startEdgeIterator() {
        edgeIterator = edges.iterator();
    }


    /** Whether there are more edges. */
    public boolean hasMoreEdges() {
        return edgeIterator.hasNext();
    }


    /** Returns next edge. */
    public Edge nextEdge() {
        return edgeIterator.next();
    }


    /**
     * Copy a graph to the current graph object.
     *
     * @param graph the graph to copy from
     */
    public void copyGraph(Graph graph) {
        // Make sure the current graph is empty
        assert ((nodes.isEmpty()) && (edges.isEmpty()));
        for (graph.startNodeIterator();
             graph.hasMoreNodes();) {
            addNode(graph.nextNode());
        }
        for (graph.startEdgeIterator();
             graph.hasMoreEdges();) {
            addEdge(graph.nextEdge());
        }
        setInitialNode(graph.getInitialNode());
        setFinalNode(graph.getFinalNode());
    }


    /**
     * Insert a graph in the current graph, replacing a particular node.
     *
     * @param graph the graph to insert
     * @param node  the node that this graph will replace
     */
    public void insertGraph(Graph graph, Node node) {
        // Make sure the node belongs to the graph
        assert isNodeInGraph(node) : "Node not in graph";
        assert graph != null : "Graph not defined";
        assert ((!isFinalNode(node)) && (!isInitialNode(node)));
        int nodePosition = nodes.indexOf(node);
        nodes.remove(nodePosition);
        int index;
        for (graph.startNodeIterator(), index = nodePosition;
             graph.hasMoreNodes(); index++) {
            nodes.add(index, graph.nextNode());
        }
        for (graph.startEdgeIterator();
             graph.hasMoreEdges();) {
            addEdge(graph.nextEdge());
        }
        Node initialNode = graph.getInitialNode();
        for (node.startIncomingEdgeIterator();
             node.hasMoreIncomingEdges();) {
            Edge edge = node.nextIncomingEdge();
            edge.setDestination(initialNode);
            initialNode.addIncomingEdge(edge);
        }
        Node finalNode = graph.getFinalNode();
        for (node.startOutgoingEdgeIterator();
             node.hasMoreOutgoingEdges();) {
            Edge edge = node.nextOutgoingEdge();
            edge.setSource(finalNode);
            finalNode.addOutgoingEdge(edge);
        }
    }


    /**
     * Validate the graph. It checks out basics about the graph, such as whether all nodes have at least one incoming
     * and outgoing edge, except for the initial and final.
     *
     * @return if true, graph validation passed
     */
    public boolean validate() {
        boolean passed = true;
        for (startNodeIterator();
             hasMoreNodes();) {
            Node node = nextNode();
            passed &= node.validate();
            int incoming = node.incomingEdgesSize();
            int outgoing = node.outgoingEdgesSize();
            if (incoming < 1) {
                if (!isInitialNode(node)) {
                    System.out.println("No incoming edge: " + node);
                    passed = false;
                }
            }
            for (node.startIncomingEdgeIterator();
                 node.hasMoreIncomingEdges();) {
                passed &= edges.contains(node.nextIncomingEdge());
            }
            if (outgoing < 1) {
                if (!isFinalNode(node)) {
                    System.out.println("No outgoing edge: " + node);
                    passed = false;
                }
            }
            for (node.startOutgoingEdgeIterator();
                 node.hasMoreOutgoingEdges();) {
                passed &= edges.contains(node.nextOutgoingEdge());
            }
        }
        for (startEdgeIterator();
             hasMoreEdges();) {
            Edge edge = nextEdge();
            passed &= edge.validate();
            passed &= nodes.contains(edge.getSource());
            passed &= nodes.contains(edge.getDestination());
        }
        return passed;
    }


    /** Prints out the graph. For debugging purposes. */
    public void printGraph() {
        for (startNodeIterator();
             hasMoreNodes();) {
            Node node = nextNode();
            if (isInitialNode(node)) {
                System.out.println("Initial Node");
            }
            if (isFinalNode(node)) {
                System.out.println("Final Node");
            }
            System.out.println(node);
            node.print();
        }
        for (startEdgeIterator();
             hasMoreEdges();) {
            Edge edge = nextEdge();
            System.out.println(edge);
            edge.print();
        }
    }
}

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

/** Indicates node types such as beginning, end, containing word etc. */
public class NodeType {

    private String name;

    /** NodeType to indicate dummy node. */
    public static final NodeType DUMMY = new NodeType("DUMMY");

    /** NodeType to indicate node containing silence with loopback. */
    public static final NodeType SILENCE_WITH_LOOPBACK =
            new NodeType("SILENCE_WITH_LOOPBACK");

    /** NodeType to indicate the end of a speech utterance. */
    public static final NodeType UTTERANCE_END = new NodeType("UTTERANCE_END");

    /** NodeType to indicate the start of am utterance. */
    public static final NodeType UTTERANCE_BEGIN =
            new NodeType("UTTERANCE_BEGIN");

    /** NodeType to indicate the node contains a word. */
    public static final NodeType WORD = new NodeType("WORD");

    /** NodeType to indicate the node contains a word. */
    public static final NodeType PHONE = new NodeType("PHONE");

    /** NodeType to indicate the node contains a word. */
    public static final NodeType STATE = new NodeType("STATE");


    /** Constructs a NodeType with the given name. */
    protected NodeType(String name) {
        this.name = name;
    }


    /**
     * Returns true if the given NodeType is equal to this NodeType.
     *
     * @param nodeType the NodeType to compare
     * @return true if they are the same, false otherwise
     */
    public boolean equals(NodeType nodeType) {
        if (nodeType != null) {
            return toString().equals(nodeType.toString());
        } else {
            return false;
        }
    }


    /**
     * Returns the name of this NodeType.
     *
     * @return the name of this NodeType.
     */
    @Override
    public String toString() {
        return name;
    }
}

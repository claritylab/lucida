/*
 * 
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.linguist;

/** Represents a search graph */
public interface SearchGraph {

    /**
     * Retrieves initial search state
     *
     * @return the set of initial search state
     */
    SearchState getInitialState();


    /**
     * Returns the number of different state types maintained in the search graph
     *
     * @return the number of different state types
     */
    int getNumStateOrder();

}

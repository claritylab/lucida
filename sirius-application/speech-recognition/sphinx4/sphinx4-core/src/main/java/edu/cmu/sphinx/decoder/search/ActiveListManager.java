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

import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.S4Double;
import edu.cmu.sphinx.util.props.S4Integer;

import java.util.Iterator;

/** An active list is maintained as a sorted list */
public interface ActiveListManager extends Configurable {

    /** The property that specifies the absolute word beam width */
    @S4Integer(defaultValue = 2000)
    public final static String PROP_ABSOLUTE_WORD_BEAM_WIDTH =
            "absoluteWordBeamWidth";

    /** The property that specifies the relative word beam width */
    @S4Double(defaultValue = 0.0)
    public final static String PROP_RELATIVE_WORD_BEAM_WIDTH =
            "relativeWordBeamWidth";

    /**
     * Adds the given token to the list
     *
     * @param token the token to add
     */
    public void add(Token token);


    /**
     * Replaces an old token with a new token
     *
     * @param oldToken the token to replace (or null in which case, replace works like add).
     * @param newToken the new token to be placed in the list.
     */
    public void replace(Token oldToken, Token newToken);


    /**
     * Returns an Iterator of all the non-emitting ActiveLists. The iteration order is the same as the search state
     * order.
     *
     * @return an Iterator of non-emitting ActiveLists
     */
    public Iterator<ActiveList> getNonEmittingListIterator();


    /**
     * Returns the emitting ActiveList from the manager
     *
     * @return the emitting ActiveList
     */
    public ActiveList getEmittingList();

    
    /**
     * Clears emitting list in manager
     */
	public void clearEmittingList();

	
    /** Dumps out debug info for the active list manager */
    public void dump();


    /**
     * Sets the total number of state types to be managed
     *
     * @param numStateOrder the total number of state types
     */
    public void setNumStateOrder(int numStateOrder);

}


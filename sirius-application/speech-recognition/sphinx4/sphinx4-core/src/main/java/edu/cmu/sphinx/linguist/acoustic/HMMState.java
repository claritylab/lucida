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

package edu.cmu.sphinx.linguist.acoustic;

import edu.cmu.sphinx.frontend.Data;

/** Represents a single state in an HMM */
public interface HMMState {

    /**
     * Gets the HMM associated with this state
     *
     * @return the HMM
     */
    public HMM getHMM();


    /**
     * Gets the state
     *
     * @return the state
     */
    public int getState();


    /**
     * Gets the score for this HMM state
     *
     * @param data the data to be scored
     * @return the acoustic score for this state.
     */
    public float getScore(Data data);


    /**
     * Determines if this HMMState is an emitting state
     *
     * @return true if the state is an emitting state
     */
    public boolean isEmitting();


    /**
     * Retrieves the state of successor states for this state
     *
     * @return the set of successor state arcs
     */
    public HMMStateArc[] getSuccessors();


    /**
     * Determines if this state is an exit state of the HMM
     *
     * @return true if the state is an exit state
     */
    public boolean isExitState();
}


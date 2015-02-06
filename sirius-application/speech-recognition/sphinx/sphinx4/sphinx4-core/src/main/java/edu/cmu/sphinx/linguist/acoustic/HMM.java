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


/**
 * Represents a hidden-markov-model. An HMM consists of a unit (context dependent or independent), a transition matrix
 * from state to state, and a sequence of senones associated with each state. This representation of an HMM is a
 * specialized left-to-right markov model. No backward transitions are allowed.
 */

public interface HMM {

    /**
     * Gets the  unit associated with this HMM
     *
     * @return the unit associated with this HMM
     */
    public Unit getUnit();


    /**
     * Gets the  base unit associated with this HMM
     *
     * @return the unit associated with this HMM
     */
    public Unit getBaseUnit();


    /**
     * Retrieves the hmm state
     *
     * @param which the state of interest
     */
    public HMMState getState(int which);


    /**
     * Returns the order of the HMM
     *
     * @return the order of the HMM
     */
    public int getOrder();


    /**
     * Retrieves the position of this HMM.
     *
     * @return the position for this HMM
     */
    public HMMPosition getPosition();


    /**
     * Gets the initial states (with probabilities) for this HMM
     *
     * @return the set of arcs that transition to the initial states for this HMM
     */
    public HMMState getInitialState();
}


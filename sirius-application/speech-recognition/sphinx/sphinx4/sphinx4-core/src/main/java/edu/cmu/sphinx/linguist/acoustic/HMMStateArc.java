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
 * Represents a transition to single state in an HMM
 * <p/>
 * All probabilities are maintained in linear base
 */
public class HMMStateArc {

    private final HMMState hmmState;
    private final float probability;


    /**
     * Constructs an HMMStateArc
     *
     * @param hmmState    destination state for this arc
     * @param probability the probability for this transition
     */
    public HMMStateArc(HMMState hmmState, float probability) {
        this.hmmState = hmmState;
        this.probability = probability;
    }


    /**
     * Gets the HMM associated with this state
     *
     * @return the HMM
     */
    public HMMState getHMMState() {
        return hmmState;
    }


    /**
     * Gets log transition probability
     *
     * @return the probability in the LogMath log domain
     */
    public float getLogProbability() {
        return probability;
    }


    /** returns a string representation of the arc */
    @Override
    public String toString() {
        return "HSA " + hmmState + " prob " + probability;
    }
}


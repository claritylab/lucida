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

package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import edu.cmu.sphinx.linguist.acoustic.HMM;
import edu.cmu.sphinx.linguist.acoustic.HMMPosition;
import edu.cmu.sphinx.linguist.acoustic.HMMState;
import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.util.Utilities;

/**
 * Represents a hidden-markov-model. An HMM consists of a unit (context dependent or independent), a transition matrix
 * from state to state, and a sequence of senones associated with each state. This representation of an HMM is a
 * specialized left-to-right markov model. No backward transitions are allowed.
 */
public class SenoneHMM implements HMM {

    private final Unit unit;
    private final Unit baseUnit;
    private final SenoneSequence senoneSequence;
    private final float[][] transitionMatrix;
    private final HMMPosition position;
    private static int objectCount;
    private final HMMState[] hmmStates;


    /**
     * Constructs an HMM
     *
     * @param unit             the unit for this HMM
     * @param senoneSequence   the sequence of senones for this HMM
     * @param transitionMatrix the state transition matrix
     * @param position         the position associated with this HMM
     */
    public SenoneHMM(Unit unit, SenoneSequence senoneSequence,
                     float[][] transitionMatrix, HMMPosition position) {
        this.unit = unit;
        this.senoneSequence = senoneSequence;
        this.transitionMatrix = transitionMatrix;
        this.position = position;
        Utilities.objectTracker("HMM", objectCount++);

        hmmStates = new HMMState[transitionMatrix.length];
        for (int i = 0; i < hmmStates.length; i++) {
            hmmStates[i] = new SenoneHMMState(this, i);
        }
        // baseUnit = Unit.getUnit(unit.getName());
        baseUnit = unit.getBaseUnit();
    }


    /**
     * Gets the  unit associated with this HMM
     *
     * @return the unit associated with this HMM
     */
    public Unit getUnit() {
        return unit;
    }


    /**
     * Gets the  base unit associated with this HMM
     *
     * @return the unit associated with this HMM
     */
    public Unit getBaseUnit() {
        return baseUnit;
    }


    /**
     * Retrieves the hmm state
     *
     * @param which the state of interest
     */
    public HMMState getState(int which) {
        return hmmStates[which];
    }


    /**
     * Returns the order of the HMM
     *
     * @return the order of the HMM
     */
    // [[[NOTE: this method is probably not explicitly needed since
    // getSenoneSequence.getSenones().length will provide the same
    // value, but this is certainly more convenient and easier to
    // understand
    public int getOrder() {
        return getSenoneSequence().getSenones().length;
    }


    /**
     * Returns the SenoneSequence associated with this HMM
     *
     * @return the sequence of senones associated with this HMM. The length of the sequence is N, where N is the order
     *         of the HMM. Note that senone sequences may be shared among HMMs.
     */
    // [[ NOTE: the senone sequence may in fact be a sequence of
    // composite senones
    public SenoneSequence getSenoneSequence() {
        return senoneSequence;
    }


    /**
     * Determines if this HMM is a composite HMM
     *
     * @return true if this is a composite hmm
     */
    public boolean isComposite() {
        Senone[] senones = getSenoneSequence().getSenones();
        for (Senone senone : senones) {
            if (senone instanceof CompositeSenone) {
                return true;
            }
        }
        return false;
    }


    /**
     * Returns the transition matrix that determines the state transition probabilities for the matrix. Each entry in
     * the transition matrix defines the probability of transitioning from one state to the next. For example, the
     * probability of transitioning from state 1 to state 2 can be determined by accessing transition matrix
     * element[1][2].
     *
     * @return the transition matrix (in log domain) of size NxN where N is the order of the HMM
     */
    public float[][] getTransitionMatrix() {
        return transitionMatrix;
    }


    /**
     * Returns the transition probability between two states.
     *
     * @param stateFrom the index of the state this transition goes from
     * @param stateTo   the index of the state this transition goes to
     * @return the transition probability (in log domain)
     */
    public float getTransitionProbability(int stateFrom, int stateTo) {
        return transitionMatrix[stateFrom][stateTo];
    }


    /**
     * Retrieves the position of this HMM. Possible
     *
     * @return the position for this HMM
     */
    public HMMPosition getPosition() {
        return position;
    }


    /**
     * Determines if this HMM represents a filler unit. A filler unit is speech that is not meaningful such as a cough,
     * 'um' , 'er', or silence.
     *
     * @return true if the HMM  represents a filler unit
     */
    public boolean isFiller() {
        return unit.isFiller();
    }


    /**
     * Determines if this HMM corresponds to a context dependent unit
     *
     * @return true if the HMM is context dependent
     */
    public boolean isContextDependent() {
        return unit.isContextDependent();
    }


    /**
     * Gets the initial states (with probabilities) for this HMM
     *
     * @return the set of arcs that transition to the initial states for this HMM
     */
    public HMMState getInitialState() {
        return getState(0);
    }


    /**
     * Returns the string representation of this object
     *
     * @return the string representation
     */
    @Override
    public String toString() {
        String name = isComposite() ? "HMM@" : "HMM";
        return name + '(' + unit + "):" + position;
    }


    @Override
    public int hashCode() {
        return getSenoneSequence().hashCode();
    }


    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        } else if (o instanceof SenoneHMM) {
            SenoneHMM other = (SenoneHMM) o;
		return getSenoneSequence().equals(other.getSenoneSequence());
        }
        return false;
    }
}


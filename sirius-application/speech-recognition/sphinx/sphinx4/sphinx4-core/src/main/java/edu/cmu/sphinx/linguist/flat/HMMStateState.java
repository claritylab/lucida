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

package edu.cmu.sphinx.linguist.flat;

import edu.cmu.sphinx.decoder.scorer.ScoreProvider;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.linguist.HMMSearchState;
import edu.cmu.sphinx.linguist.acoustic.HMMState;

import java.io.Serializable;

/** Represents a hmmState in an SentenceHMMS */

@SuppressWarnings("serial")
public class HMMStateState extends SentenceHMMState
        implements Serializable, HMMSearchState, ScoreProvider {

    private HMMState hmmState;
    private boolean isEmitting;


    /**
     * Creates a HMMStateState
     *
     * @param parent   the parent of this state
     * @param hmmState the hmmState associated with this state
     */
    public HMMStateState(SentenceHMMState parent, HMMState hmmState) {
        super("S", parent, hmmState.getState());
        this.hmmState = hmmState;
        this.isEmitting = hmmState.isEmitting();
    }


    /**
     * Gets the hmmState associated with this state
     *
     * @return the hmmState
     */
    public HMMState getHMMState() {
        return hmmState;
    }


    /**
     * Determines if this state is an emitting state
     *
     * @return true if the state is an emitting state
     */
    @Override
    public boolean isEmitting() {
        return isEmitting;
    }


    /**
     * Retrieves a short label describing the type of this state. Typically, subclasses of SentenceHMMState will
     * implement this method and return a short (5 chars or less) label
     *
     * @return the short label.
     */
    @Override
    public String getTypeLabel() {
        return "HMM";
    }

    
    /**
     * Calculate the acoustic score for this state
     *
     * @return the acoustic score for this state
     */
    public float getScore(Data feature) {
        return hmmState.getScore(feature);
    }


    /**
     * Returns the state order for this state type
     *
     * @return the state order
     */
    @Override
    public int getOrder() {
        return isEmitting ? 6 : 0;
    }
}



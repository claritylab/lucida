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

import edu.cmu.sphinx.linguist.acoustic.HMMState;


/** Represents a hmmState in an SentenceHMMS */
@SuppressWarnings("serial")
public class NonEmittingHMMState extends HMMStateState {

    /**
     * Creates a NonEmittingHMMState
     *
     * @param parent   the parent of this state
     * @param hmmState the hmmState associated with this state
     */
    public NonEmittingHMMState(SentenceHMMState parent, HMMState hmmState) {
        super(parent, hmmState);
    }
}



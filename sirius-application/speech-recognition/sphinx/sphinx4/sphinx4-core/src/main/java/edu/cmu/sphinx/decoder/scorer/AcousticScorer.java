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

package edu.cmu.sphinx.decoder.scorer;

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.util.props.Configurable;

import java.util.List;

/** Provides a mechanism for scoring a set of HMM states */
public interface AcousticScorer extends Configurable {

    /** Allocates resources for this scorer */
    public void allocate();


    /** Deallocates resources for this scorer */
    public void deallocate();


    /** starts the scorer */
    public void startRecognition();


    /** stops the scorer */
    public void stopRecognition();

    /**
     * Scores the given set of states
     *
     * @param scorableList a list containing Scoreable objects to be scored
     * @return the best scoring scoreable, or null if there are no more frames to score
     */
    public Data calculateScores(List<? extends Scoreable> scorableList);

}



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

package edu.cmu.sphinx.result;

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.linguist.acoustic.Unit;

/**
 * Contains statistics about a frame.
 * <p/>
 * Note that all scores are maintained in LogMath log base
 */
public abstract class FrameStatistics {

    /**
     * Gets the frame number
     *
     * @return the frame number
     */
    public abstract int getFrameNumber();


    /**
     * Gets the feature associated with this frame
     *
     * @return the feature associated with the frame or null if the feature is not available
     */
    public abstract Data getData();


    /**
     * Gets the best score for this frame
     *
     * @return the best score for this frame in the LogMath log domain
     */
    public abstract float getBestScore();


    /**
     * Gets the unit that had the best score for this frame
     *
     * @return the unit with the best score
     */
    public abstract Unit getBestUnit();


    /**
     * Gets the best scoring hmm state for this frame
     *
     * @return the best scoring state
     */
    public abstract int getBestState();
}



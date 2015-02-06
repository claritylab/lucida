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


import edu.cmu.sphinx.frontend.Data;

import java.io.Serializable;

/** Represents a set of acoustic data that can be scored against a feature */
public interface Senone extends Serializable {

    /**
     * Calculates the score for this senone based upon the given feature.
     *
     * @param feature the feature vector to score this senone against
     * @return the score for this senone in LogMath log base
     */
    public float getScore(Data feature);


    /**
     * Calculates the component scores for the mixture components in this senone based upon the given feature.
     *
     * @param feature the feature vector to score this senone against
     * @return the scores for this senone in LogMath log base
     */
    public float[] calculateComponentScore(Data feature);


    /**
     * Gets the ID for this senone
     *
     * @return the senone id
     */
    public long getID();


    /**
     * Dumps a senone
     *
     * @param msg an annotation for the dump
     */
    public void dump(String msg);

}

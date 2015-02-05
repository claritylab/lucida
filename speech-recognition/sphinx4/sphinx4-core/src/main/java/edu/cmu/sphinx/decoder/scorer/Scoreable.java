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

import java.util.Comparator;

/** Represents an entity that can be scored against a data */
public interface Scoreable extends Data {

    /**
     * A {@code Scoreable} comparator that is used to order scoreables according to their score,
     * in descending order.
     *
     * <p>Note: since a higher score results in a lower natural order,
     * statements such as {@code Collections.min(list, Scoreable.COMPARATOR)}
     * actually return the Scoreable with the <b>highest</b> score,
     * in contrast to the natural meaning of the word "min".   
     */
    Comparator<Scoreable> COMPARATOR = new Comparator<Scoreable>() {
        public int compare(Scoreable t1, Scoreable t2) {
            if (t1.getScore() > t2.getScore()) {
                return -1;
            } else if (t1.getScore() == t2.getScore()) {
                return 0;
            } else {
                return 1;
            }
        }
    };

    /**
     * Calculates a score against the given data. The score can be retrieved with get score
     *
     * @param data     the data to be scored
     * @return the score for the data
     */
    public float calculateScore(Data data);


    /**
     * Retrieves a previously calculated (and possibly normalized) score
     *
     * @return the score
     */
    public float getScore();


    /**
     * Normalizes a previously calculated score
     *
     * @param maxScore
     * @return the normalized score
     */
    public float normalizeScore(float maxScore);


    /**
     * Returns the frame number that this Scoreable should be scored against.
     *
     * @return the frame number that this Scoreable should be scored against.
     */
    public int getFrameNumber();
}

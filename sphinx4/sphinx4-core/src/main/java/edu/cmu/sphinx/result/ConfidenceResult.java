/*
 * Copyright 1999-2004 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.result;

/**
 * <p/>
 * Shows the confidence information about a Result. </p>
 */
public interface ConfidenceResult extends Iterable<ConfusionSet> {

    /**
     * Returns the best hypothesis of the result.
     *
     * @return the best hypothesis of the result
     */
    public Path getBestHypothesis();

    /**
     * Get the number of word slots contained in this result
     *
     * @return length of the result
     */
    public int size();

    /**
     * Get the nth confusion set in this result
     *
     * @param i the index of the confusion set to get
     * @return the requested confusion set
     */
    public ConfusionSet getConfusionSet(int i);
}

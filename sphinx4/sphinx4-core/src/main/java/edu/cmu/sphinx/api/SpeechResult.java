/*
 * Copyright 2013 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */

package edu.cmu.sphinx.api;

import java.util.List;
import java.util.Collection;

import edu.cmu.sphinx.result.Lattice;
import edu.cmu.sphinx.result.LatticeOptimizer;
import edu.cmu.sphinx.result.Nbest;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.result.WordResult;


/**
 * High-level wrapper for {@link Result} instance.
 */
public final class SpeechResult {

    private final Result result;
    private final Lattice lattice;

    /**
     * Constructs recognition result based on {@link Result} object.
     *
     * @param scorer confidence scorer
     * @param result recognition result returned by {@link Recognizer}
     */
    public SpeechResult(Result result) {
        this.result = result;
        lattice = new Lattice(result);
        new LatticeOptimizer(lattice).optimize();
    }

    /**
     * Returns {@link List} of words of the recognition result.
     * Within the list words are ordered by time frame.
     *
     * @return words that form the result
     */
    public List<WordResult> getWords() {
        return result.getWords();
    }

    /**
     * Returns string representaion of the result.
     */
    public String getHypothesis() {
	return result.getBestResultNoFiller();
    }

    /**
     * Return N best hypothesis.
     *
     * @param  n number of hypothesis to return
     * @return   {@link Collection} of several best hypothesis
     */
    public Collection<String> getNbest(int n) {
        return new Nbest(lattice).getNbest(n);
    }

    /**
     * Returns lattice for the recognition result.
     *
     * @return lattice object
     */
    public Lattice getLattice() {
        return lattice;
    }
}

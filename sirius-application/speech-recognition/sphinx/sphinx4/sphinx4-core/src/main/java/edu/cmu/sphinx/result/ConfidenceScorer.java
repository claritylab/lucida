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

import edu.cmu.sphinx.util.props.Configurable;

/**
 * <p/>
 * Computes confidences for a Result. Typically, one is interested in the confidence of the best path of a result, as
 * well as the confidence of each word in the best path of a result. To obtain this information, one should do the
 * following: </p>
 * <p/>
 * <pre>
 * <p/>
 * ConfidenceScorer scorer = (ConfidenceScorer) ... // obtain scorer from configuration manager
 * <p/>
 * Result result = recognizer.recognize();
 * ConfidenceResult confidenceResult = scorer.score(result);
 * <p/>
 * // confidence for best path
 * Path bestPath = confidenceResult.getBestHypothesis();
 * double pathConfidence = bestPath.getConfidence();
 * <p/>
 * // confidence for each word in best path
 * WordResult[] words = bestPath.getWords();
 * for (int i = 0; i < words.length; i++) {
 *     WordResult wordResult = (WordResult) words[i];
 *     double wordConfidence = wordResult.getConfidence();
 * }
 * <p/>
 * </pre>
 * <p/>
 * <p/>
 * Note that different ConfidenceScorers have different definitions for the 'best path', and therefore their
 * <code>getBestHypothesis</code> methods will return different things. The {@link
 * edu.cmu.sphinx.result.MAPConfidenceScorer} returns the highest scoring path. On the other hand, the {@link
 * edu.cmu.sphinx.result.SausageMaker} returns the path where all the words have the highest confidence in their
 * corresponding time slot. </p>
 */
public interface ConfidenceScorer extends Configurable {

    /**
     * Computes confidences for a Result and returns a ConfidenceResult, a compact representation of all the hypothesis
     * contained in the result together with their per-word and per-path confidences.
     *
     * @param result the result to compute confidences for
     * @return a confidence result
     */
    public ConfidenceResult score(Result result);
}

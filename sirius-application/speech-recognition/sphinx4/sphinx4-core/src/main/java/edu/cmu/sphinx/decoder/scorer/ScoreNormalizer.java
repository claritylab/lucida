package edu.cmu.sphinx.decoder.scorer;

import edu.cmu.sphinx.util.props.Configurable;

import java.util.List;

/**
 * Describes all API-elements that are necessary  to normalize token-scores after these have been computed by an
 * AcousticScorer.
 *
 * @author Holger Brandl
 * @see edu.cmu.sphinx.decoder.scorer.AcousticScorer
 * @see edu.cmu.sphinx.decoder.search.Token
 */
public interface ScoreNormalizer extends Configurable {

    /**
     * Normalizes the scores of a set of Tokens.
     *
     * @param scoreableList The set of scores to be normalized
     * @param bestToken     The best scoring Token of the above mentioned list. Although not strictly necessary it's
     *                      included because of convenience reasons and to reduce computational overhead.
     * @return The best token after the all <code>Token</code>s have been normalized. In most cases normalization won't
     *         change the order but to keep the API open for any kind of approach it seemed reasonable to include this.
     */
    Scoreable normalize(List<? extends Scoreable> scoreableList, Scoreable bestToken);
}

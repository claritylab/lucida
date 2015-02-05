package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import edu.cmu.sphinx.frontend.Data;

/**
 * Implements a Senone that contains a cache of the last scored data.
 * <p>
 * Subclasses should implement the abstract {@link #calculateScore} method,
 * which is called by the {@link #getScore} method to calculate the score
 * for each cache miss.
 * <p>
 * Note: this implementation is thread-safe and can be safely used
 * across different threads without external synchronization.
 *
 * @author Yaniv Kunda
 */
@SuppressWarnings("serial")
public abstract class ScoreCachingSenone implements Senone {

    private class ScoreCache {
        private final Data feature;
        private final float score;

        public ScoreCache(Data feature, float score) {
            this.feature = feature;
            this.score = score;
        }
    }

    private volatile ScoreCache scoreCache = new ScoreCache(null, 0.0f);

    /**
     * Gets the cached score for this senone based upon the given feature.
     * If the score was not cached, it is calculated using {@link #calculateScore},
     * cached, and then returned.  
     */
    public float getScore(Data feature) {
        ScoreCache cached = scoreCache;
        if (feature != cached.feature) {
            cached = new ScoreCache(feature, calculateScore(feature));
            scoreCache = cached;
        }
        return cached.score;
    }

    /**
     * Calculates the score for this senone based upon the given feature.
     *
     * @param feature the feature vector to score this senone against
     * @return the score for this senone in LogMath log base
     */
    protected abstract float calculateScore(Data feature);

}

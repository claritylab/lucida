/*
* Copyright 1999-2013 Carnegie Mellon University.
* Portions Copyright 2002 Sun Microsystems, Inc.
* Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
* All Rights Reserved.  Use is subject to license terms.
*
* See the file "license.terms" for information on usage and
* redistribution of this file, and for a DISCLAIMER OF ALL
* WARRANTIES.
*/

package edu.cmu.sphinx.result;

import edu.cmu.sphinx.linguist.dictionary.Pronunciation;
import edu.cmu.sphinx.linguist.dictionary.Word;

import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.TimeFrame;

/**
 * Represents a word in a recognition result.
 *
 * This is designed specifically for obtaining confidence scores.
 * All scores are maintained in LogMath log base.
 */
public class WordResult {

    private final Word word;
    private final TimeFrame timeFrame;

    private final double score;
    private final double confidence;

    /**
     * Construct a word result from a string and a confidence score.
     *
     * @param w          the word
     * @param confidence the confidence for this word
     */
    public WordResult(String w, double confidence) {
        Pronunciation[] pros = {Pronunciation.UNKNOWN};
        word = new Word(w, pros, false);
        timeFrame = TimeFrame.NULL;
        this.confidence = confidence;
        this.score = LogMath.LOG_ZERO;
    }

    /**
     * Construct a word result with full information.
     *
     * @param w          the word object to store
     * @param timeFrame  time frame
     * @param ef         word end time
     * @param score      score of the word
     * @param confidence confidence (posterior) of the word
     */
    public WordResult(Word w, TimeFrame timeFrame,
                      double score, double confidence)
    {
        this.word = w;
        this.timeFrame = timeFrame;
        this.score = score;
        this.confidence = confidence;
    }

    /**
     * Construct a WordResult using a Node object and a confidence (posterior).
     *
     * This does not use the posterior stored in the Node object, just its
     * word, start and end.
     *
     * TODO: score is currently set to zero
     *
     * @param node       the node to extract information from
     * @param confidence the confidence (posterior) to assign
     */
    public WordResult(Node node, double confidence) {
        this(node.getWord(),
             new TimeFrame(node.getBeginTime(), node.getEndTime()),
             LogMath.LOG_ZERO, confidence);
    }

    /**
     * Gets the total score for this word.
     *
     * @return the score for the word (in LogMath log base)
     */
    public double getScore() {
        return score;
    }

    /**
     * Returns a log confidence score for this WordResult.
     *
     * Use the getLogMath().logToLinear() method to convert the log confidence
     * score to linear. The linear value should be between 0.0 and 1.0
     * (inclusive) for this word.
     *
     * @return a log confidence score which linear value is in [0, 1]
     */
    public double getConfidence() {
        // TODO: can confidence really be greater than 1?
        return Math.min(confidence, LogMath.LOG_ONE);
    }

    /**
     * Gets the pronunciation for this word.
     *
     * @return the pronunciation for the word
     */
    public Pronunciation getPronunciation() {
        return word.getMostLikelyPronunciation();
    }

    /**
     * Gets time frame for the word
     */
    public TimeFrame getTimeFrame() {
        return timeFrame;
    }

    /**
     * Does this word result represent a filler token?
     *
     * @return true if this is a filler
     */
    public boolean isFiller() {
        return word.isFiller() || word.toString().equals("<skip>");
    }

    @Override
    public String toString() {
        return String.format("{%s, %f, [%d, %d]}",
                             word, confidence,
                             timeFrame.getStart(), timeFrame.getEnd());
    }
}


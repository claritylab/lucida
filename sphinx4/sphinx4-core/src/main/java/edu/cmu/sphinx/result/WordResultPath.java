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
 *
 * Created on Aug 31, 2004
 */

package edu.cmu.sphinx.result;

import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.util.LogMath;

import java.util.List;
import java.util.ArrayList;

/**
 * An implementation of a result Path that computes scores and confidences on the fly.
 *
 * @author P. Gorniak
 */
public class WordResultPath implements Path {

    private final List<WordResult> path = new ArrayList<WordResult>();


    /**
     * Constructs a WordResultPath with the given list of WordResults and LogMath.
     *
     * @param wordResults the list of WordResults
     */
    WordResultPath(List<WordResult> wordResults) {
        path.addAll(wordResults);
    }


    /** Constructs an empty WordResultPath. */
    WordResultPath() {
    }


    /** @see edu.cmu.sphinx.result.Path#getScore() */
    public double getScore() {
        double score = LogMath.LOG_ONE;
        for (WordResult wr : path) {
            score += wr.getScore();
        }
        return score;
    }


    /** @see edu.cmu.sphinx.result.Path#getConfidence() */
    public double getConfidence() {
        double confidence = LogMath.LOG_ONE;
        for (WordResult wr : path) {
            confidence += wr.getConfidence();
        }
        return confidence;
    }

    /** @see edu.cmu.sphinx.result.Path#getWords() */
    public WordResult[] getWords() {
        return path.toArray(new WordResult[path.size()]);
    }


    /** @see edu.cmu.sphinx.result.Path#getTranscription() */
    public String getTranscription() {
        StringBuilder sb = new StringBuilder();
        for (WordResult wr : path)
            sb.append(wr).append(' ');
        return sb.toString().trim();
    }

    /** @see edu.cmu.sphinx.result.Path#getTranscriptionNoFiller() */
    public String getTranscriptionNoFiller() {
        StringBuilder sb = new StringBuilder();
        for (WordResult wordResult : path) {
            Word word = wordResult.getPronunciation().getWord();
            if (!word.isFiller() && !word.getSpelling().equals("<unk>")) {
                sb.append(word.getSpelling()).append(' ');
            }
        }
        return sb.toString().trim();
    }

    public void add(WordResult wr) {
        path.add(wr);
    }

}

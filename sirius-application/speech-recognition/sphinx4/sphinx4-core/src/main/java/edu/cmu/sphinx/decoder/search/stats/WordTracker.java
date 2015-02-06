package edu.cmu.sphinx.decoder.search.stats;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.linguist.WordSearchState;
import edu.cmu.sphinx.linguist.WordSequence;
import edu.cmu.sphinx.linguist.dictionary.Word;

/** A class that keeps track of word histories */

public class WordTracker {

    final Map<WordSequence, WordStats> statMap;
    final int frameNumber;
    int stateCount;
    int maxWordHistories;


    /**
     * Creates a word tracker for the given frame number
     *
     * @param frameNumber the frame number
     */
    public WordTracker(int frameNumber) {
        statMap = new HashMap<WordSequence, WordStats>();
        this.frameNumber = frameNumber;
    }


    /**
     * Adds a word history for the given token to the word tracker
     *
     * @param t the token to add
     */
    public void add(Token t) {
        stateCount++;
        WordSequence ws = getWordSequence(t);
        WordStats stats = statMap.get(ws);
        if (stats == null) {
            stats = new WordStats(ws);
            statMap.put(ws, stats);
        }
        stats.update(t);
    }


    /** Dumps the word histories in the tracker */
    public void dump() {
        dumpSummary();
        List<WordStats> stats = new ArrayList<WordStats>(statMap.values());
        Collections.sort(stats, WordStats.COMPARATOR);
        for (WordStats stat : stats) {
            System.out.println("   " + stat);
        }
    }


    /** Dumps summary information in the tracker */
    void dumpSummary() {
        System.out.println("Frame: " + frameNumber + " states: " + stateCount
                + " histories " + statMap.size());
    }


    /**
     * Given a token, gets the word sequence represented by the token
     *
     * @param token the token of interest
     * @return the word sequence for the token
     */
    private WordSequence getWordSequence(Token token) {
        List<Word> wordList = new LinkedList<Word>();

        while (token != null) {
            if (token.isWord()) {
                WordSearchState wordState = (WordSearchState) token
                        .getSearchState();
                Word word = wordState.getPronunciation().getWord();
                wordList.add(0, word);
            }
            token = token.getPredecessor();
        }
        return new WordSequence(wordList);
    }
    
    /** Keeps track of statistics for a particular word sequence */

    static class WordStats {

        public final static Comparator<WordStats> COMPARATOR = new Comparator<WordStats>() {
            public int compare(WordStats ws1, WordStats ws2) {
                if (ws1.maxScore > ws2.maxScore) {
                    return -1;
                } else if (ws1.maxScore == ws2.maxScore) {
                    return 0;
                } else {
                    return 1;
                }
            }
        };

        private int size;
        private float maxScore;
        private float minScore;
        private final WordSequence ws;

        /**
         * Creates a word statistics for the given sequence
         *
         * @param ws the word sequence
         */
        WordStats(WordSequence ws) {
            size = 0;
            maxScore = -Float.MAX_VALUE;
            minScore = Float.MAX_VALUE;
            this.ws = ws;
        }


        /**
         * Updates the statistics based upon the scores for the given token
         *
         * @param t the token
         */
        void update(Token t) {
            size++;
            if (t.getScore() > maxScore) {
                maxScore = t.getScore();
            }
            if (t.getScore() < minScore) {
                minScore = t.getScore();
            }
        }


        /**
         * Returns a string representation of the statistics
         *
         * @return a string representation
         */
        @Override
        public String toString() {
            return "states:" + size + " max:" + maxScore + " min:" + minScore + ' '
                    + ws;
        }
    }
}


package edu.cmu.sphinx.decoder.search.stats;

import java.util.HashMap;
import java.util.Map;

import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.linguist.HMMSearchState;

/** This debugging class is used to track the number of active tokens per state */

public class TokenTracker {

    private Map<Object, TokenStats> stateMap;
    private boolean enabled;
    private int frame;

    private int utteranceStateCount;
    private int utteranceMaxStates;
    private int utteranceSumStates;


    /**
     * Enables or disables the token tracker
     *
     * @param enabled if <code>true</code> the tracker is enabled
     */
    void setEnabled(boolean enabled) {
        this.enabled = enabled;
    }


    /** Starts the per-utterance tracking */
    void startUtterance() {
        if (enabled) {
            frame = 0;
            utteranceStateCount = 0;
            utteranceMaxStates = -Integer.MAX_VALUE;
            utteranceSumStates = 0;
        }
    }


    /** stops the per-utterance tracking */
    void stopUtterance() {
        if (enabled) {
            dumpSummary();
        }
    }


    /** Starts the per-frame tracking */
    void startFrame() {
        if (enabled) {
            stateMap = new HashMap<Object, TokenStats>();
        }
    }


    /**
     * Adds a new token to the tracker
     *
     * @param t the token to add.
     */
    public void add(Token t) {
        if (enabled) {
            TokenStats stats = getStats(t);
            stats.update(t);
        }
    }


    /** Stops the per-frame tracking */
    void stopFrame() {
        if (enabled) {
            frame++;
            dumpDetails();
        }
    }


    /** Dumps summary info about the tokens */
    public void dumpSummary() {
        if (enabled) {
            float avgStates = 0f;
            if (utteranceStateCount > 0) {
                avgStates = ((float) utteranceSumStates) / utteranceStateCount;
            }
            System.out.print("# Utterance stats ");
            System.out.print(" States: " + utteranceStateCount / frame);

            if (utteranceStateCount > 0) {
                System.out.print(" Paths: " + utteranceSumStates / frame);
                System.out.print(" Max: " + utteranceMaxStates);
                System.out.print(" Avg: " + avgStates);
            }

            System.out.println();
        }
    }


    /** Dumps detailed info about the tokens */
    public void dumpDetails() {
        if (enabled) {
            int maxStates = -Integer.MAX_VALUE;
            int hmmCount = 0;
            int sumStates = 0;

            for (TokenStats stats : stateMap.values()) {
                if (stats.isHMM) {
                    hmmCount++;
                }
                sumStates += stats.count;
                utteranceSumStates += stats.count;
                if (stats.count > maxStates) {
                    maxStates = stats.count;
                }

                if (stats.count > utteranceMaxStates) {
                    utteranceMaxStates = stats.count;
                }
            }

            utteranceStateCount += stateMap.size();

            float avgStates = 0f;
            if (!stateMap.isEmpty()) {
                avgStates = ((float) sumStates) / stateMap.size();
            }
            System.out.print("# Frame " + frame);
            System.out.print(" States: " + stateMap.size());

            if (!stateMap.isEmpty()) {
                System.out.print(" Paths: " + sumStates);
                System.out.print(" Max: " + maxStates);
                System.out.print(" Avg: " + avgStates);
                System.out.print(" HMM: " + hmmCount);
            }

            System.out.println();
        }
    }


    /**
     * Gets the statistics for a particular token
     *
     * @param t the token of interest
     * @return the token statistics associated with the given token
     */
    private TokenStats getStats(Token t) {
        TokenStats stats = stateMap.get(t.getSearchState()
                .getLexState());
        if (stats == null) {
            stats = new TokenStats();
            stateMap.put(t.getSearchState().getLexState(), stats);
        }
        return stats;
    }

    /**
     * A class for keeping track of statistics about tokens. Tracks the count,
     * minimum and maximum score for a particular state.
     */
    class TokenStats {

        int count;
        float maxScore;
        float minScore;
        boolean isHMM;


        TokenStats() {
            count = 0;
            maxScore = -Float.MAX_VALUE;
            minScore = Float.MIN_VALUE;
        }


        /** Update this state with the given token
         * @param t*/
        public void update(Token t) {
            count++;
            if (t.getScore() > maxScore) {
                maxScore = t.getScore();
            }

            if (t.getScore() < minScore) {
                minScore = t.getScore();
            }

            isHMM = t.getSearchState() instanceof HMMSearchState;
        }
    }

}



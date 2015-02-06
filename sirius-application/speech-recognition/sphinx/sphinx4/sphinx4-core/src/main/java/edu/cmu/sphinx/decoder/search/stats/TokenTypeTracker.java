package edu.cmu.sphinx.decoder.search.stats;

import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.linguist.HMMSearchState;
import edu.cmu.sphinx.linguist.SearchState;
import edu.cmu.sphinx.linguist.UnitSearchState;
import edu.cmu.sphinx.linguist.WordSearchState;
import edu.cmu.sphinx.linguist.acoustic.HMM;

/**
 * A tool for tracking the types tokens created and placed in the beam
 * <p/>
 * TODO: Develop a mechanism  for adding trackers such as these in a more general fashion.
 */
public class TokenTypeTracker {
    // keep track of the various types of states

    private int numWords;
    private int numUnits;
    private int numOthers;
    private int numHMMBegin;
    private int numHMMEnd;
    private int numHMMSingle;
    private int numHMMInternal;
    private int numTokens;


    /**
     * Adds a token to this tracker. Records statistics about the type of token.
     *
     * @param t the token to track
     */
    public void add(Token t) {
        numTokens++;
        SearchState s = t.getSearchState();

        if (s instanceof WordSearchState) {
            numWords++;
        } else if (s instanceof UnitSearchState) {
            numUnits++;
        } else if (s instanceof HMMSearchState) {
            HMM hmm = ((HMMSearchState) s).getHMMState().getHMM();
            switch (hmm.getPosition()) {
                case BEGIN: numHMMBegin++; break;
                case END: numHMMEnd++; break;
                case SINGLE: numHMMSingle++; break;
                case INTERNAL: numHMMInternal++; break;
                default: break;
            }
        } else {
            numOthers++;
        }
    }


    /** Shows the accumulated statistics */
    public void dump() {
        System.out.println("TotalTokens: " + numTokens);
        System.out.println("      Words: " + numWords + pc(numWords));
        System.out.println("      Units: " + numUnits + pc(numUnits));
        System.out.println("      HMM-b: " + numHMMBegin + pc(numHMMBegin));
        System.out.println("      HMM-e: " + numHMMEnd + pc(numHMMEnd));
        System.out.println("      HMM-s: " + numHMMSingle + pc(numHMMSingle));
        System.out.println("      HMM-i: " + numHMMInternal +
                pc(numHMMInternal));
        System.out.println("     Others: " + numOthers + pc(numOthers));
    }


    /**
     * Utility method for generating integer percents
     *
     * @param num the value to be converted into percent
     * @return a string representation as a percent
     */
    private String pc(int num) {
        int percent = ((100 * num) / numTokens);
        return " (" + percent + "%)";
    }
}

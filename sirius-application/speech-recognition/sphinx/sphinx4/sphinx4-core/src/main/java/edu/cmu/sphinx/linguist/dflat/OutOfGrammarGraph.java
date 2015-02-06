/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.linguist.dflat;

import edu.cmu.sphinx.decoder.scorer.ScoreProvider;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.linguist.*;
import edu.cmu.sphinx.linguist.acoustic.*;
import edu.cmu.sphinx.linguist.dictionary.Pronunciation;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.util.LogMath;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;


/** Builds a grammar sub-graph that matches all phones. This is suitable for use as an out-of-grammar detector */
public class OutOfGrammarGraph {

    private final AcousticModel acousticModel;
    private final float logOutOfGrammarBranchProbability;
    private final float logPhoneInsertionProbability;
    private final static SearchStateArc[] EMPTY_ARCS = new SearchStateArc[0];
    private final FirstBranchState fbs;
    private final LastBranchState lbs;
    private final UnknownWordState uws;
    private final SearchStateArc[] lbsArcSet;


    /**
     * Creates an OutOfGrammarGraph
     *
     * @param model                        the acoustic model
     *                                     probability of branching to this graph
     * @param logPhoneInsertionProbability probability of inserting a phone
     */
    public OutOfGrammarGraph(AcousticModel model,
                             float logOutOfGrammarBranchProbability,
                             float logPhoneInsertionProbability) {
        this.acousticModel = model;
        this.logOutOfGrammarBranchProbability = logOutOfGrammarBranchProbability;
        this.logPhoneInsertionProbability = logPhoneInsertionProbability;
        fbs = new FirstBranchState();
        lbs = new LastBranchState();
        uws = new UnknownWordState();
        lbsArcSet = new SearchStateArc[1];
        lbsArcSet[0] = lbs;

    }


    /**
     * Returns an arc to this out-of-grammar graph
     *
     * @return an arc to the graph
     */
    public SearchStateArc getOutOfGrammarGraph() {
        return uws;
    }


    /** Represents the unknown word */
    class UnknownWordState extends OogSearchState implements WordSearchState {

        private final SearchStateArc[] successors;


        /** Creates the unknown word state */
        UnknownWordState() {
            successors = new SearchStateArc[1];
            successors[0] = fbs;
        }


        /**
         * Returns the pronunciation for this word
         *
         * @return the pronunciation
         */
        public Pronunciation getPronunciation() {
            return Word.UNKNOWN.getPronunciations()[0];
        }


        /**
         * Gets the state order for this state
         *
         * @return the state order
         */
        @Override
        public int getOrder() {
            return 1;
        }


        /**
         * Returns the signature for this state
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "oogUNK";
        }


        /**
         * Gets the successor states for this search graph
         *
         * @return the successor states
         */
        @Override
        public SearchStateArc[] getSuccessors() {
            return successors;
        }


        /**
         * Gets the language probability for transitioning to this state
         *
         * @return the language probability
         */
        @Override
        public float getLanguageProbability() {
            return logOutOfGrammarBranchProbability;
        }


        /**
         * Returns true if this UnknownWordState indicates the start of a word. Returns false if this UnknownWordState
         * indicates the end of a word.
         *
         * @return true if this UnknownWordState indicates the start of a word, false if this UnknownWordState indicates
         *         the end of a word
         */
        public boolean isWordStart() {
            return true;
        }
    }

    /** Represents the first branch state in the grammar */
    class FirstBranchState extends OogSearchState {

        private final SearchStateArc[] successors;


        /** Creates the first branch state */
        FirstBranchState() {
            List<OogHMM> successorList = new ArrayList<OogHMM>();
            for (Iterator<Unit> i = acousticModel.getContextIndependentUnitIterator(); i.hasNext();) {
                Unit unit = i.next();
                OogHMM hmm = new OogHMM(unit);
                successorList.add(hmm);
            }
            successors = successorList.toArray(new SearchStateArc[successorList.size()]);
        }


        /**
         * Gets the state order for this state
         *
         * @return the state order
         */
        @Override
        public int getOrder() {
            return 2;
        }


        /**
         * Returns the signature for this state
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "oogFBS";
        }


        /**
         * Gets the successor states for this search graph
         *
         * @return the successor states
         */
        @Override
        public SearchStateArc[] getSuccessors() {
            return successors;
        }
    }

    /** Represents an HMM Unit in the search graph */
    class OogHMM extends OogSearchState implements UnitSearchState {

        private final HMM hmm;
        private final SearchStateArc[] successors;


        /**
         * Creates an HMM unit state
         *
         * @param unit the unit represented by this state
         */
        OogHMM(Unit unit) {
            hmm = acousticModel.lookupNearestHMM(unit, HMMPosition.UNDEFINED, false);
            successors = new SearchStateArc[1];
            successors[0] = new OogHMMState(hmm.getInitialState(),
                    LogMath.LOG_ONE);
        }


        /**
         * Gets the unit
         *
         * @return the unit
         */
        public Unit getUnit() {
            return hmm.getBaseUnit();
        }


        /**
         * Gets the state order for this state
         *
         * @return the state order
         */
        @Override
        public int getOrder() {
            return 3;
        }


        /**
         * Returns the signature for this state
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "oogHMM-" + getUnit();
        }


        /**
         * Gets the successor states for this search graph
         *
         * @return the successor states
         */
        @Override
        public SearchStateArc[] getSuccessors() {
            return successors;
        }


        /**
         * Gets the insertion probability of entering this state
         *
         * @return the log probability
         */
        @Override
        public float getInsertionProbability() {
            return logPhoneInsertionProbability;
        }
    }

    /** Represents a single hmm state in the search graph */
    class OogHMMState extends OogSearchState implements
            HMMSearchState, ScoreProvider {

        final HMMState hmmState;
        final float logProbability;


        /**
         * Creates an OogHMMState
         *
         * @param hmmState       the hmm state associated with this search state
         * @param logProbability the probability of transitioning to this state
         */
        OogHMMState(HMMState hmmState, float logProbability) {
            this.hmmState = hmmState;
            this.logProbability = logProbability;
        }


        /**
         * Returns the signature for this state
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "oog-" + hmmState;
        }


        /**
         * Returns the hmm state
         *
         * @return the hmm state
         */
        public HMMState getHMMState() {
            return hmmState;
        }


        /**
         * Determines if this is an emitting state
         *
         * @return true if this is an emitting state
         */
        @Override
        public boolean isEmitting() {
            return hmmState.isEmitting();
        }


        /**
         * Generate a hashcode for an object
         *
         * @return the hashcode
         */
        @Override
        public int hashCode() {
            return 191 + hmmState.hashCode();
        }

        
        /**
         * Determines if the given object is equal to this object
         *
         * @param o the object to test
         * @return <code>true</code> if the object is equal to this
         */
        @Override
        public boolean equals(Object o) {
            if (o == this) {
                return true;
            } else if (o instanceof OogHMMState) {
                OogHMMState other = (OogHMMState) o;
                return other.hmmState == hmmState;
            } else {
                return false;
            }
        }


        /**
         * Returns the order of this state type among all of the search states
         *
         * @return the order
         */
        @Override
        public int getOrder() {
            return isEmitting() ? 4 : 0;
        }


        /**
         * Gets the successor states for this search graph
         *
         * @return the successor states
         */
        @Override
        public SearchStateArc[] getSuccessors() {
            if (hmmState.isExitState()) {
                return lbsArcSet;
            } else {
                HMMStateArc[] arcs = hmmState.getSuccessors();
                SearchStateArc[] successors = new SearchStateArc[arcs.length];
                for (int i = 0; i < arcs.length; i++) {
                    successors[i] = new OogHMMState(arcs[i].getHMMState(),
                            arcs[i].getLogProbability());
                }
                return successors;
            }
        }


        public float getScore(Data data) {
            return hmmState.getScore(data);
        }
    }

    /** Represents the last branch state in the search graph */
    class LastBranchState extends OogSearchState {

        private final SearchStateArc[] successors;


        /** Creates the last branch state */
        LastBranchState() {
            successors = new SearchStateArc[2];
            successors[0] = fbs;
            successors[1] = new FinalState();
        }


        /**
         * Gets the state order for this state
         *
         * @return the state order
         */
        @Override
        public int getOrder() {
            return 1;
        }


        /**
         * Returns the signature for this state
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "oogLBS";
        }


        /**
         * Gets the successor states for this search graph
         *
         * @return the successor states
         */
        @Override
        public SearchStateArc[] getSuccessors() {
            return successors;
        }
    }

    /** Represents the final state in the search graph */
    class FinalState extends OogSearchState {

        /**
         * Gets the state order for this state
         *
         * @return the state order
         */
        @Override
        public int getOrder() {
            return 2;
        }


        /**
         * Returns the signature for this state
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "oogFinal";
        }


        /**
         * Determines if this is a final state
         *
         * @return true if this is a final state
         */
        @Override
        public boolean isFinal() {
            return true;
        }


        /**
         * Gets the successor states for this search graph
         *
         * @return the successor states
         */
        @Override
        public SearchStateArc[] getSuccessors() {
            return EMPTY_ARCS;
        }
    }

    /** The base search state for this dynamic flat linguist. */
    abstract class OogSearchState implements SearchState, SearchStateArc {

        final static int ANY = 0;


        /**
         * Gets the set of successors for this state
         *
         * @return the set of successors
         */
        public abstract SearchStateArc[] getSuccessors();


        /**
         * Returns a unique string representation of the state. This string is suitable (and typically used) for a label
         * for a GDL node
         *
         * @return the signature
         */
        public abstract String getSignature();


        /**
         * Returns the order of this state type among all of the search states
         *
         * @return the order
         */
        public abstract int getOrder();


        /**
         * Determines if this state is an emitting state
         *
         * @return true if this is an emitting state
         */
        public boolean isEmitting() {
            return false;
        }


        /**
         * Determines if this is a final state
         *
         * @return true if this is a final state
         */
        public boolean isFinal() {
            return false;
        }


        /**
         * Returns a lex state associated with the searc state (not applicable to this linguist)
         *
         * @return the lex state (null for this linguist)
         */
        public Object getLexState() {
            return null;
        }


        /**
         * Returns a well formatted string representation of this state
         *
         * @return the formatted string
         */
        public String toPrettyString() {
            return toString();
        }


        /**
         * Returns a string representation of this object
         *
         * @return a string representation
         */
        @Override
        public String toString() {
            return getSignature();
        }


        /**
         * Returns the word history for this state (not applicable to this linguist)
         *
         * @return the word history (null for this linguist)
         */
        public WordSequence getWordHistory() {
            return null;
        }


        /**
         * Gets a successor to this search state
         *
         * @return the sucessor state
         */
        public SearchState getState() {
            return this;
        }


        /**
         * Gets the composite probability of entering this state
         *
         * @return the log probability
         */
        public float getProbability() {
            return getLanguageProbability() + getInsertionProbability();
        }


        /**
         * Gets the language probability of entering this state
         *
         * @return the log probability
         */
        public float getLanguageProbability() {
            return LogMath.LOG_ONE;
        }


        /**
         * Gets the insertion probability of entering this state
         *
         * @return the log probability
         */
        public float getInsertionProbability() {
            return LogMath.LOG_ONE;
        }
    }
}

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

package edu.cmu.sphinx.linguist.flat;

import edu.cmu.sphinx.linguist.SearchGraph;
import edu.cmu.sphinx.linguist.SearchState;
import edu.cmu.sphinx.linguist.WordSearchState;
import edu.cmu.sphinx.linguist.acoustic.*;
import edu.cmu.sphinx.linguist.dictionary.Pronunciation;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.util.LogMath;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * Constructs a loop of all the context-independent phones. This loop is used in the static flat linguist for detecting
 * out-of-grammar utterances. A 'phoneInsertionProbability' will be added to the score each time a new phone is entered.
 * To obtain the all-phone search graph loop, simply called the method {@link #getSearchGraph() getSearchGraph}.
 * <p/>
 * For futher details of this approach cf. 'Modeling Out-of-vocabulary Words for Robust Speech Recognition', Brazzi,
 * 2000, Proc. ICSLP
 */
public class CIPhoneLoop {

    public final AcousticModel model;
    private final float logPhoneInsertionProbability;
    public final float logOne = LogMath.LOG_ONE;


    /**
     * Creates the CIPhoneLoop with the given acoustic model and phone insertion probability
     *
     * @param model                        the acoustic model
     * @param logPhoneInsertionProbability the insertion probability
     */
    public CIPhoneLoop(AcousticModel model,
                       float logPhoneInsertionProbability) {
        this.model = model;
        this.logPhoneInsertionProbability =
                logPhoneInsertionProbability;
    }


    /**
     * Creates a new loop of all the context-independent phones.
     *
     * @return the phone loop search graph
     */
    public SearchGraph getSearchGraph() {
        return new PhoneLoopSearchGraph();
    }


    protected class PhoneLoopSearchGraph implements SearchGraph {

        protected final Map<String, SearchState> existingStates;
        protected final SentenceHMMState firstState;


        /** Constructs a phone loop search graph. */
        public PhoneLoopSearchGraph() {
            existingStates = new HashMap<String, SearchState>();
            firstState = new UnknownWordState();
            SentenceHMMState branchState = new BranchOutState(firstState);
            attachState(firstState, branchState, logOne, logOne);

            SentenceHMMState lastState = new LoopBackState(firstState);
            lastState.setFinalState(true);
            attachState(lastState, branchState, logOne, logOne);

            for (Iterator<Unit> i = model.getContextIndependentUnitIterator(); i.hasNext();) {
                UnitState unitState = new UnitState(i.next(), HMMPosition.UNDEFINED);

                // attach unit state to the branch out state
                attachState(branchState, unitState, logOne,
                        logPhoneInsertionProbability);

                HMM hmm = model.lookupNearestHMM
                        (unitState.getUnit(), unitState.getPosition(), false);
                HMMState initialState = hmm.getInitialState();
                HMMStateState hmmTree = new HMMStateState(unitState, initialState);
                addStateToCache(hmmTree);

                // attach first HMM state to the unit state
                attachState(unitState, hmmTree, logOne, logOne);

                // expand the HMM tree
                HMMStateState finalState = expandHMMTree(unitState, hmmTree);

                // attach final state of HMM tree to the loopback state
                attachState(finalState, lastState, logOne, logOne);
            }
        }


        /**
         * Retrieves initial search state
         *
         * @return the set of initial search state
         */
        public SearchState getInitialState() {
            return firstState;
        }


        /**
         * Returns the number of different state types maintained in the search graph
         *
         * @return the number of different state types
         */
        public int getNumStateOrder() {
            return 5;
        }


        /**
         * Checks to see if a state that matches the given state already exists
         *
         * @param state the state to check
         * @return true if a state with an identical signature already exists.
         */
        private SentenceHMMState getExistingState(SentenceHMMState state) {
            return (SentenceHMMState) existingStates.get(state.getSignature());
        }


        /**
         * Adds the given state to the cache of states
         *
         * @param state the state to add
         */
        protected void addStateToCache(SentenceHMMState state) {
            existingStates.put(state.getSignature(), state);
        }


        /**
         * Expands the given hmm state tree
         *
         * @param parent the parent of the tree
         * @param tree   the tree to expand
         * @return the final state in the tree
         */
        protected HMMStateState expandHMMTree(UnitState parent,
                                            HMMStateState tree) {
            HMMStateState retState = tree;
            for (HMMStateArc arc : tree.getHMMState().getSuccessors()) {
                HMMStateState newState;
                if (arc.getHMMState().isEmitting()) {
                    newState = new HMMStateState
                        (parent, arc.getHMMState());
                } else {
                    newState = new NonEmittingHMMState
                        (parent, arc.getHMMState());
                }
                SentenceHMMState existingState = getExistingState(newState);
                float logProb = arc.getLogProbability();
                if (existingState != null) {
                    attachState(tree, existingState, logOne, logProb);
                } else {
                    attachState(tree, newState, logOne, logProb);
                    addStateToCache(newState);
                    retState = expandHMMTree(parent, newState);
                }
            }
            return retState;
        }


        protected void attachState(SentenceHMMState prevState,
                                   SentenceHMMState nextState,
                                   float logLanguageProbability,
                                   float logInsertionProbability) {
            SentenceHMMStateArc arc = new SentenceHMMStateArc
                    (nextState,
                     logLanguageProbability,
                     logInsertionProbability);
            prevState.connect(arc);
        }
    }
}

@SuppressWarnings("serial")
class UnknownWordState extends SentenceHMMState implements WordSearchState {

    public Pronunciation getPronunciation() {
        return Word.UNKNOWN.getPronunciations()[0];
    }


    @Override
    public int getOrder() {
        return 0;
    }


    @Override
    public String getName() {
        return "UnknownWordState";
    }


    /**
     * Returns true if this UnknownWordState indicates the start of a word. Returns false if this UnknownWordState
     * indicates the end of a word.
     *
     * @return true if this UnknownWordState indicates the start of a word, false if this UnknownWordState indicates the
     *         end of a word
     */
    @Override
    public boolean isWordStart() {
        return true;
    }
}

@SuppressWarnings("serial")
class LoopBackState extends SentenceHMMState {

    LoopBackState(SentenceHMMState parent) {
        super("CIPhonesLoopBackState", parent, 0);
    }


    @Override
    public int getOrder() {
        return 1;
    }
}

@SuppressWarnings("serial")
class BranchOutState extends SentenceHMMState {

    BranchOutState(SentenceHMMState parent) {
        super("BranchOutState", parent, 0);
    }


    @Override
    public int getOrder() {
        return 1;
    }
}


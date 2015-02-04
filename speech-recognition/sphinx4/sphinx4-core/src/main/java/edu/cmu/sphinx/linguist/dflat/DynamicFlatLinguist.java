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
import edu.cmu.sphinx.linguist.language.grammar.Grammar;
import edu.cmu.sphinx.linguist.language.grammar.GrammarArc;
import edu.cmu.sphinx.linguist.language.grammar.GrammarNode;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.Timer;
import edu.cmu.sphinx.util.TimerPool;
import edu.cmu.sphinx.util.props.*;

import java.io.IOException;
import java.util.*;
import java.util.logging.Logger;

/**
 * A simple form of the linguist. It makes the following simplifying assumptions: 1) Zero or one word per grammar node
 * 2) No fan-in allowed ever 3) No composites (yet) 4) Only Unit, HMMState, and pronunciation states (and the
 * initial/final grammar state are in the graph (no word, alternative or grammar states attached). 5) Only valid
 * transitions (matching contexts) are allowed 6) No tree organization of units 7) Branching grammar states are
 * allowed
 * <p/>
 * This is a dynamic version of the flat linguist that is more efficient in terms of startup time and overall footprint
 * <p/>
 * Note that all probabilities are maintained in the log math domain
 */

public class DynamicFlatLinguist implements Linguist, Configurable {

    /** The property used to define the grammar to use when building the search graph */
    @S4Component(type = Grammar.class)
    public final static String GRAMMAR = "grammar";

    /** The property used to define the unit manager to use when building the search graph */
    @S4Component(type = UnitManager.class)
    public final static String UNIT_MANAGER = "unitManager";

    /** The property used to define the acoustic model to use when building the search graph */
    @S4Component(type = AcousticModel.class)
    public final static String ACOUSTIC_MODEL = "acousticModel";

    /** The property that specifies whether to add a branch for detecting out-of-grammar utterances. */
    @S4Boolean(defaultValue = false)
    public final static String ADD_OUT_OF_GRAMMAR_BRANCH = "addOutOfGrammarBranch";

    /** The property for the probability of entering the out-of-grammar branch. */
    @S4Double(defaultValue = 1.0)
    public final static String OUT_OF_GRAMMAR_PROBABILITY = "outOfGrammarProbability";

    /** The property for the probability of inserting a CI phone in the out-of-grammar ci phone loop */
    @S4Double(defaultValue = 1.0)
    public static final String PHONE_INSERTION_PROBABILITY = "phoneInsertionProbability";

    /** The property for the acoustic model to use to build the phone loop that detects out of grammar utterances. */
    @S4Component(type = AcousticModel.class)
    public final static String PHONE_LOOP_ACOUSTIC_MODEL = "phoneLoopAcousticModel";

    // ----------------------------------
    // Subcomponents that are configured
    // by the property sheet
    // -----------------------------------
    private Grammar grammar;
    private AcousticModel acousticModel;
    private AcousticModel phoneLoopAcousticModel;
    private LogMath logMath;
    private UnitManager unitManager;
    // ------------------------------------
    // Data that is configured by the
    // property sheet
    // ------------------------------------
    private float logWordInsertionProbability;
    private float logSilenceInsertionProbability;
    private float logUnitInsertionProbability;
    private float logFillerInsertionProbability;
    private float languageWeight;
    private float logOutOfGrammarBranchProbability;
    private float logPhoneInsertionProbability;
    private boolean addOutOfGrammarBranch;

    // ------------------------------------
    // Data used for building and maintaining
    // the search graph
    // -------------------------------------
    private SearchGraph searchGraph;
    private Logger logger;
    private HMMPool hmmPool;
    SearchStateArc outOfGrammarGraph;
    private GrammarNode initialGrammarState;

    // this map is used to manage the set of follow on units for a
    // particular grammar node. It is used to select the set of
    // possible right contexts as we leave a node

    private Map<GrammarNode, int[]> nodeToNextUnitArrayMap;

    // this map is used to manage the set of possible entry units for
    // a grammar node. It is used to filter paths so that we only
    // branch to grammar nodes that match the current right context.

    private Map<GrammarNode, Set<Unit>> nodeToUnitSetMap;

    // an empty arc (just waiting for Noah, I guess)
    private final SearchStateArc[] EMPTY_ARCS = new SearchStateArc[0];

    public DynamicFlatLinguist(AcousticModel acousticModel, Grammar grammar, UnitManager unitManager,
            double wordInsertionProbability, double silenceInsertionProbability, double unitInsertionProbability,
            double fillerInsertionProbability, float languageWeight, boolean addOutOfGrammarBranch,
            double outOfGrammarBranchProbability, double phoneInsertionProbability, AcousticModel phoneLoopAcousticModel) {

        this.logger = Logger.getLogger(getClass().getName());
        this.acousticModel = acousticModel;
        logMath = LogMath.getInstance();
        this.grammar = grammar;
        this.unitManager = unitManager;

        this.logWordInsertionProbability = logMath.linearToLog(wordInsertionProbability);
        this.logSilenceInsertionProbability = logMath.linearToLog(silenceInsertionProbability);
        this.logUnitInsertionProbability = logMath.linearToLog(unitInsertionProbability);
        this.logFillerInsertionProbability = logMath.linearToLog(fillerInsertionProbability);
        this.languageWeight = languageWeight;
        this.addOutOfGrammarBranch = addOutOfGrammarBranch;
        this.logOutOfGrammarBranchProbability = logMath.linearToLog(outOfGrammarBranchProbability);

        this.logPhoneInsertionProbability = logMath.linearToLog(logPhoneInsertionProbability);
        if (addOutOfGrammarBranch) {
            this.phoneLoopAcousticModel = phoneLoopAcousticModel;
        }
    }

    public DynamicFlatLinguist() {
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();
        logMath = LogMath.getInstance();

        acousticModel = (AcousticModel) ps.getComponent(ACOUSTIC_MODEL);
        grammar = (Grammar) ps.getComponent(GRAMMAR);
        unitManager = (UnitManager) ps.getComponent(UNIT_MANAGER);

        // get the rest of the configuration data
        logWordInsertionProbability = logMath.linearToLog(ps.getDouble(PROP_WORD_INSERTION_PROBABILITY));
        logSilenceInsertionProbability = logMath.linearToLog(ps.getDouble(PROP_SILENCE_INSERTION_PROBABILITY));
        logUnitInsertionProbability = logMath.linearToLog(ps.getDouble(PROP_UNIT_INSERTION_PROBABILITY));
        logFillerInsertionProbability = logMath.linearToLog(ps.getDouble(PROP_FILLER_INSERTION_PROBABILITY));
        languageWeight = ps.getFloat(Linguist.PROP_LANGUAGE_WEIGHT);
        addOutOfGrammarBranch = ps.getBoolean(ADD_OUT_OF_GRAMMAR_BRANCH);
        logOutOfGrammarBranchProbability = logMath.linearToLog(ps.getDouble(OUT_OF_GRAMMAR_PROBABILITY));
        
        logPhoneInsertionProbability = logMath.linearToLog(ps.getDouble(PHONE_INSERTION_PROBABILITY));
        if (addOutOfGrammarBranch) {
            phoneLoopAcousticModel = (AcousticModel) ps.getComponent(PHONE_LOOP_ACOUSTIC_MODEL);
        }
    }


    /**
     * Returns the search graph
     *
     * @return the search graph
     */
    public SearchGraph getSearchGraph() {
        return searchGraph;
    }

    public void allocate() throws IOException {
        logger.info("Allocating DFLAT");
        allocateAcousticModel();
        grammar.allocate();
        hmmPool = new HMMPool(acousticModel, logger, unitManager);
        nodeToNextUnitArrayMap = new HashMap<GrammarNode, int[]>();
        nodeToUnitSetMap = new HashMap<GrammarNode, Set<Unit>>();
        Timer timer = TimerPool.getTimer(this, "compileGrammar");
        timer.start();
        compileGrammar();
        timer.stop();
        logger.info("Done allocating  DFLAT");
    }


    /** Allocates the acoustic model.
     * @throws java.io.IOException*/
    protected void allocateAcousticModel() throws IOException {
        acousticModel.allocate();
        if (addOutOfGrammarBranch) {
            phoneLoopAcousticModel.allocate();
        }
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.linguist.Linguist#deallocate()
    */
    public void deallocate() {
        if (acousticModel != null) {
            acousticModel.deallocate();
        }
        grammar.deallocate();
    }


    /** Called before a recognition */
    public void startRecognition() {
        if (grammarHasChanged()) {
            compileGrammar();
        }
    }


    /** Called after a recognition */
    public void stopRecognition() {
    }


    /**
     * Returns the log silence insertion probability.
     *
     * @return the log silence insertion probability.
     */
    public float getLogSilenceInsertionProbability() {
        return logSilenceInsertionProbability;
    }

    /**
     * Determines if the underlying grammar has changed since we last compiled the search graph
     *
     * @return true if the grammar has changed
     */
    private boolean grammarHasChanged() {
        return initialGrammarState == null ||
                initialGrammarState != grammar.getInitialNode();
    }

    private void compileGrammar() {
        initialGrammarState = grammar.getInitialNode();

        for (GrammarNode node : grammar.getGrammarNodes()) {
            initUnitMaps(node);
        }

        searchGraph = new DynamicFlatSearchGraph();
    }


    /**
     * Initializes the unit maps for this linguist. There are two unit maps: (a) nodeToNextUnitArrayMap contains an
     * array of unit ids for all possible units that immediately follow the given grammar node. This is used to
     * determine the set of exit contexts for words within a grammar node. (b) nodeToUnitSetMap contains the set of
     * possible entry units for a given grammar node. This is typically used to determine if a path with a given right
     * context should branch into a particular grammar node
     *
     * @param node the units maps will be created for this node.
     */
    private void initUnitMaps(GrammarNode node) {

        // collect the set of next units for this node

        if (nodeToNextUnitArrayMap.get(node) == null) {
            Set<GrammarNode> vistedNodes = new HashSet<GrammarNode>();
            Set<Unit> unitSet = new HashSet<Unit>();

            GrammarArc[] arcs = node.getSuccessors();
            for (GrammarArc arc : arcs) {
                GrammarNode nextNode = arc.getGrammarNode();
                collectNextUnits(nextNode, vistedNodes, unitSet);
            }
            int[] nextUnits = new int[unitSet.size()];
            int index = 0;
            for (Unit unit : unitSet) {
                nextUnits[index++] = unit.getBaseID();
            }
            nodeToNextUnitArrayMap.put(node, nextUnits);
        }

        // collect the set of entry units for this node

        if (nodeToUnitSetMap.get(node) == null) {
            Set<GrammarNode> vistedNodes = new HashSet<GrammarNode>();
            Set<Unit> unitSet = new HashSet<Unit>();
            collectNextUnits(node, vistedNodes, unitSet);
            nodeToUnitSetMap.put(node, unitSet);
        }
    }


    /**
     * For the given grammar node, collect the set of possible next units.
     *
     * @param thisNode    the grammar node
     * @param vistedNodes the set of visited grammar nodes, used to ensure that we don't attempt to expand a particular
     *                    grammar node more than once (which could lead to a death spiral)
     * @param unitSet     the entry units are collected here.
     */
    private void collectNextUnits(GrammarNode thisNode,
                                  Set<GrammarNode> vistedNodes, Set<Unit> unitSet) {
        if (vistedNodes.contains(thisNode)) {
            return;
        }

        vistedNodes.add(thisNode);
        if (thisNode.isFinalNode()) {
            unitSet.add(UnitManager.SILENCE);
        } else if (!thisNode.isEmpty()) {
            Word word = thisNode.getWord();
            Pronunciation[] pronunciations = word.getPronunciations();
            for (Pronunciation pronunciation : pronunciations) {
                unitSet.add(pronunciation.getUnits()[0]);
            }
        } else {
            GrammarArc[] arcs = thisNode.getSuccessors();
            for (GrammarArc arc : arcs) {
                GrammarNode nextNode = arc.getGrammarNode();
                collectNextUnits(nextNode, vistedNodes, unitSet);
            }
        }
    }


    final Map<SearchState, SearchStateArc[]> successorCache = new HashMap<SearchState, SearchStateArc[]>();

    /** The base search state for this dynamic flat linguist. */
    abstract class FlatSearchState implements SearchState, SearchStateArc {

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
         * @return the successor state
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


        /**
         * Get the arcs from the cache if the exist
         *
         * @return the cached arcs or null
         */
        SearchStateArc[] getCachedSuccessors() {
            return successorCache.get(this);
        }


        /**
         * Places the set of successor arcs in the cache
         *
         * @param successors the set of arcs to be cached for this state
         */
        void cacheSuccessors(SearchStateArc[] successors) {
            successorCache.put(this, successors);
        }
    }

    /**
     * Represents a grammar node in the search graph. A grammar state needs to keep track of the associated grammar node
     * as well as the left context and next base unit.
     */
    class GrammarState extends FlatSearchState {

        private final GrammarNode node;
        private final int lc;
        private final int nextBaseID;
        private final float languageProbability;


        /**
         * Creates a grammar state for the given node with a silence Lc
         *
         * @param node the grammar node
         */
        GrammarState(GrammarNode node) {
            this(node, LogMath.LOG_ONE, UnitManager.SILENCE.getBaseID());
        }


        /**
         * Creates a grammar state for the given node and left context. The path will connect to any possible next base
         *
         * @param node                the grammar node
         * @param languageProbability the probability of transistioning to this word
         * @param lc                  the left context for this path
         */
        GrammarState(GrammarNode node, float languageProbability, int lc) {
            this(node, languageProbability, lc, ANY);
        }


        /**
         * Creates a grammar state for the given node and left context and next base ID.
         *
         * @param node                the grammar node
         * @param languageProbability the probability of transistioning to this word
         * @param lc                  the left context for this path
         * @param nextBaseID          the next base ID
         */
        GrammarState(GrammarNode node, float languageProbability,
                     int lc, int nextBaseID) {
            this.lc = lc;
            this.nextBaseID = nextBaseID;
            this.node = node;
            this.languageProbability = languageProbability;
        }


        /**
         * Gets the language probability of entering this state
         *
         * @return the log probability
         */
        @Override
        public float getLanguageProbability() {
            return languageProbability * languageWeight;
        }


        /**
         * Generate a hashcode for an object. Equality for a  grammar state includes the grammar node, the lc and the
         * next base ID
         *
         * @return the hashcode
         */
        @Override
        public int hashCode() {
            return node.hashCode() * 17 + lc * 7 + nextBaseID;
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
            } else if (o instanceof GrammarState) {
                GrammarState other = (GrammarState) o;
                return other.node == node && lc == other.lc
                        && nextBaseID == other.nextBaseID;
            } else {
                return false;
            }
        }


        /**
         * Determines if this is a final state in the search graph
         *
         * @return true if this is a final state in the search graph
         */
        @Override
        public boolean isFinal() {
            return node.isFinalNode();
        }


        /**
         * Gets the set of successors for this state
         * 
         * @return the set of successors
         */
        @Override
        public SearchStateArc[] getSuccessors() {

            SearchStateArc[] arcs = getCachedSuccessors();

            if (arcs != null) {
                return arcs;
            }

            if (isFinal()) {
                arcs = EMPTY_ARCS;
            } else if (node.isEmpty()) {
                arcs = getNextGrammarStates(lc, nextBaseID);
            } else {
                Word word = node.getWord();
                Pronunciation[] pronunciations = word.getPronunciations();

                // This can potentially speedup computation
                // pronunciations = filter(pronunciations, nextBaseID);

                SearchStateArc[] nextArcs = new SearchStateArc[pronunciations.length];

                for (int i = 0; i < pronunciations.length; i++) {
                    nextArcs[i] = new PronunciationState(this,
                            pronunciations[i]);
                }
                arcs = nextArcs;
            }

            cacheSuccessors(arcs);
            return arcs;
        }


        /**
         * Gets the set of arcs to the next set of grammar states that match the given nextBaseID
         *
         * @param lc         the current left context
         * @param nextBaseID the desired next base ID

         */
        SearchStateArc[] getNextGrammarStates(int lc, int nextBaseID) {
            GrammarArc[] nextNodes = node.getSuccessors();
            nextNodes = filter(nextNodes, nextBaseID);
            SearchStateArc[] nextArcs = new SearchStateArc[nextNodes.length];

            for (int i = 0; i < nextNodes.length; i++) {
                GrammarArc arc = nextNodes[i];
                nextArcs[i] = new GrammarState(arc.getGrammarNode(),
                        arc.getProbability(), lc, nextBaseID);
            }
            return nextArcs;
        }


        /**
         * Returns a unique string representation of the state. This string is suitable (and typically used) for a label
         * for a GDL node
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "GS " + node + "-lc-" + hmmPool.getUnit(lc) + "-rc-" +
                    hmmPool.getUnit(nextBaseID);
        }


        /**
         * Returns the order of this state type among all of the search states
         *
         * @return the order
         */
        @Override
        public int getOrder() {
            return 1;
        }


        /**
         * Given a set of arcs and the ID of the desired next unit, return the set of arcs containing only those that
         * transition to the next unit
         *
         * @param arcs     the set of arcs to filter
         * @param nextBase the ID of the desired next unit

         */
        GrammarArc[] filter(GrammarArc[] arcs, int nextBase) {
            if (nextBase != ANY) {
                List<GrammarArc> list = new ArrayList<GrammarArc>();
                for (GrammarArc arc : arcs) {
                    GrammarNode node = arc.getGrammarNode();
                    if (hasEntryContext(node, nextBase)) {
                        list.add(arc);
                    }
                }
                arcs = list.toArray(new GrammarArc[list.size()]);
            }
            return arcs;
        }


        /**
         * Determines if the given node starts with the specified unit
         *
         * @param node   the grammar node
         * @param unitID the id of the unit

         */
        private boolean hasEntryContext(GrammarNode node, int unitID) {
            Set<Unit> unitSet = nodeToUnitSetMap.get(node);
            return unitSet.contains(hmmPool.getUnit(unitID));
        }

        /**
         * Retain only the pronunciations that start with the unit indicated by
         * nextBase. This method can be used instead of filter to reduce search
         * space. It's not used by default but could potentially lead to a
         * decoding speedup.
         * 
         * @param p
         *            the set of pronunciations to filter
         * @param nextBase
         *            the ID of the desired initial unit
         */
        Pronunciation[] filter(Pronunciation[] pronunciations, int nextBase) {

            if (nextBase == ANY) {
                return pronunciations;
            }

            ArrayList<Pronunciation> filteredPronunciation = new ArrayList<Pronunciation>(
                    pronunciations.length);
            for (Pronunciation pronunciation : pronunciations) {
                if (pronunciation.getUnits()[0].getBaseID() == nextBase) {
                    filteredPronunciation.add(pronunciation);
                }
            }
            return filteredPronunciation
                    .toArray(new Pronunciation[filteredPronunciation.size()]);
        }

        /**
         * Gets the ID of the left context unit for this path
         *
         * @return the left context ID
         */
        int getLC() {
            return lc;
        }


        /**
         * Gets the ID of the desired next unit
         *
         * @return the ID of the next unit
         */
        int getNextBaseID() {
            return nextBaseID;
        }


        /**
         * Returns the set of IDs for all possible next units for this grammar node
         *
         * @return the set of IDs of all possible next units
         */
        int[] getNextUnits() {
            return nodeToNextUnitArrayMap.get(node);
        }


        /**
         * Returns a string representation of this object
         *
         * @return a string representation
         */
        @Override
        public String toString() {
            return node + "[" + hmmPool.getUnit(lc) + ',' + hmmPool.getUnit(nextBaseID) + ']';
        }


        /**
         * Returns the grammar node associated with this grammar state
         *
         * @return the grammar node
         */
        GrammarNode getGrammarNode() {
            return node;
        }

    }

    class InitialState extends FlatSearchState {

        private final List<SearchStateArc> nextArcs = new ArrayList<SearchStateArc>();


        /**
         * Gets the set of successors for this state
         *
         * @return the set of successors
         */
        @Override
        public SearchStateArc[] getSuccessors() {
            return nextArcs.toArray(new
                    SearchStateArc[nextArcs.size()]);
        }


        public void addArc(SearchStateArc arc) {
            nextArcs.add(arc);
        }


        /**
         * Returns a unique string representation of the state. This string is suitable (and typically used) for a label
         * for a GDL node
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "initialState";
        }


        /**
         * Returns the order of this state type among all of the search states
         *
         * @return the order
         */
        @Override
        public int getOrder() {
            return 1;
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
    }

    /** This class representations a word punctuation in the search graph */
    class PronunciationState extends FlatSearchState implements
            WordSearchState {

        private final GrammarState gs;
        private final Pronunciation pronunciation;


        /**
         * Creates a PronunciationState
         *
         * @param gs the associated grammar state
         * @param p  the pronunciation
         */
        PronunciationState(GrammarState gs, Pronunciation p) {
            this.gs = gs;
            this.pronunciation = p;
        }


        /**
         * Gets the insertion probability of entering this state
         *
         * @return the log probability
         */
        @Override
        public float getInsertionProbability() {
            if (pronunciation.getWord().isFiller()) {
                return LogMath.LOG_ONE;
            } else {
                return logWordInsertionProbability;
            }
        }


        /**
         * Generate a hashcode for an object
         *
         * @return the hashcode
         */
        @Override
        public int hashCode() {
            return 13 * gs.hashCode() + pronunciation.hashCode();
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
            } else if (o instanceof PronunciationState) {
                PronunciationState other = (PronunciationState) o;
                return other.gs.equals(gs) &&
                        other.pronunciation.equals(pronunciation);
            } else {
                return false;
            }
        }


        /**
         * Gets the successor states for this search graph
         *
         * @return the successor states
         */
        @Override
        public SearchStateArc[] getSuccessors() {
            SearchStateArc[] arcs = getCachedSuccessors();
            if (arcs == null) {
                arcs = getSuccessors(gs.getLC(), 0);
                cacheSuccessors(arcs);
            }
            return arcs;
        }


        /**
         * Gets the successor states for the unit and the given position and left context
         *
         * @param lc    the ID of the left context
         * @param index the position of the unit within the pronunciation
         * @return the set of sucessor arcs
         */
        SearchStateArc[] getSuccessors(int lc, int index) {
            SearchStateArc[] arcs;
            if (index == pronunciation.getUnits().length - 1) {
                if (isContextIndependentUnit(
                        pronunciation.getUnits()[index])) {
                    arcs = new SearchStateArc[1];
                    arcs[0] = new FullHMMSearchState(this, index, lc, ANY);
                } else {
                    int[] nextUnits = gs.getNextUnits();
                    arcs = new SearchStateArc[nextUnits.length];
                    for (int i = 0; i < arcs.length; i++) {
                        arcs[i] = new
                                FullHMMSearchState(this, index, lc, nextUnits[i]);
                    }
                }
            } else {
                arcs = new SearchStateArc[1];
                arcs[0] = new FullHMMSearchState(this, index, lc);
            }
            return arcs;
        }


        /**
         * Gets the pronunciation assocated with this state
         *
         * @return the pronunciation
         */
        public Pronunciation getPronunciation() {
            return pronunciation;
        }


        /**
         * Determines if the given unit is a CI unit
         *
         * @param unit the unit to test
         * @return true if the unit is a context independent unit
         */
        private boolean isContextIndependentUnit(Unit unit) {
            return unit.isFiller();
        }


        /**
         * Returns a unique string representation of the state. This string is suitable (and typically used) for a label
         * for a GDL node
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "PS " + gs.getSignature() + '-' + pronunciation;
        }


        /**
         * Returns a string representation of this object
         *
         * @return a string representation
         */
        @Override
        public String toString() {
            return pronunciation.getWord().getSpelling();
        }


        /**
         * Returns the order of this state type among all of the search states
         *
         * @return the order
         */
        @Override
        public int getOrder() {
            return 2;
        }


        /**
         * Returns the grammar state associated with this state
         *
         * @return the grammar state
         */
        GrammarState getGrammarState() {
            return gs;
        }


        /**
         * Returns true if this WordSearchState indicates the start of a word. Returns false if this WordSearchState
         * indicates the end of a word.
         *
         * @return true if this WordSearchState indicates the start of a word, false if this WordSearchState indicates
         *         the end of a word
         */
        public boolean isWordStart() {
            return true;
        }
    }


    /** Represents a unit (as an HMM) in the search graph */
    class FullHMMSearchState extends FlatSearchState implements
            UnitSearchState {

        private final PronunciationState pState;
        private final int index;
        private final int lc;
        private final int rc;
        private final HMM hmm;
        private final boolean isLastUnitOfWord;


        /**
         * Creates a FullHMMSearchState
         *
         * @param p     the parent PronunciationState
         * @param which the index of the unit within the pronunciation
         * @param lc    the ID of the left context
         */
        FullHMMSearchState(PronunciationState p, int which, int lc) {
            this(p, which, lc,
                    p.getPronunciation().getUnits()[which + 1].getBaseID());
        }


        /**
         * Creates a FullHMMSearchState
         *
         * @param p     the parent PronunciationState
         * @param which the index of the unit within the pronunciation
         * @param lc    the ID of the left context
         * @param rc    the ID of the right context
         */
        FullHMMSearchState(PronunciationState p, int which, int lc, int rc) {
            this.pState = p;
            this.index = which;
            this.lc = lc;
            this.rc = rc;
            int base =
                    p.getPronunciation().getUnits()[which].getBaseID();
            int id = hmmPool.buildID(base, lc, rc);
            hmm = hmmPool.getHMM(id, getPosition());
            isLastUnitOfWord =
                    which == p.getPronunciation().getUnits().length - 1;
        }


        /**
         * Determines the insertion probability based upon the type of unit
         *
         * @return the insertion probability
         */
        @Override
        public float getInsertionProbability() {
            Unit unit = hmm.getBaseUnit();

            if (unit.isSilence()) {
                return logSilenceInsertionProbability;
            } else if (unit.isFiller()) {
                return logFillerInsertionProbability;
            } else {
                return logUnitInsertionProbability;
            }
        }


        /**
         * Returns a string representation of this object
         *
         * @return a string representation
         */
        @Override
        public String toString() {
            return hmm.getUnit().toString();
        }


        /**
         * Generate a hashcode for an object
         *
         * @return the hashcode
         */
        @Override
        public int hashCode() {
            return pState.getGrammarState().getGrammarNode().hashCode() * 29 +
                    pState.getPronunciation().hashCode() * 19 +
                    index * 7 + 43 * lc + rc;
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
            } else if (o instanceof FullHMMSearchState) {
                FullHMMSearchState other = (FullHMMSearchState) o;
                // the definition for equal for a FullHMMState:
                // Grammar Node equal
                // Pronunciation equal
                // index equal
                // rc equal

                return pState.getGrammarState().getGrammarNode() ==
                        other.pState.getGrammarState().getGrammarNode() &&
                        pState.getPronunciation() == other.pState.getPronunciation() &&
                        index == other.index && lc == other.lc && rc == other.rc;
            } else {
                return false;
            }
        }


        /**
         * Returns the unit associated with this state
         *
         * @return the unit
         */
        public Unit getUnit() {
            return hmm.getBaseUnit();
        }


        /**
         * Gets the set of successors for this state
         *
         * @return the set of successors
         */
        @Override
        public SearchStateArc[] getSuccessors() {
            SearchStateArc[] arcs = getCachedSuccessors();
            if (arcs == null) {
                arcs = new SearchStateArc[1];
                arcs[0] = new HMMStateSearchState(this, hmm.getInitialState());
                cacheSuccessors(arcs);
            }
            return arcs;
        }


        /**
         * Determines if this unit is the last unit of a word
         *
         * @return true if this unit is the last unit of a word
         */
        boolean isLastUnitOfWord() {
            return isLastUnitOfWord;
        }


        /**
         * Determines the position of the unit within the word
         *
         * @return the position of the unit within the word
         */
        HMMPosition getPosition() {
            int len = pState.getPronunciation().getUnits().length;
            if (len == 1) {
                return HMMPosition.SINGLE;
            } else if (index == 0) {
                return HMMPosition.BEGIN;
            } else if (index == len - 1) {
                return HMMPosition.END;
            } else {
                return HMMPosition.INTERNAL;
            }
        }


        /**
         * Returns the HMM for this state
         *
         * @return the HMM
         */
        HMM getHMM() {
            return hmm;
        }


        /**
         * Returns the order of this state type among all of the search states
         *
         * @return the order
         */
        @Override
        public int getOrder() {
            return 3;
        }

        /**
         * Returns a unique string representation of the state. This string is suitable (and typically used) for a label
         * for a GDL node
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "HSS " + pState.getGrammarState().getGrammarNode() +
                    pState.getPronunciation() + index + '-' + rc + '-' + lc;
        }

        /**
         * Returns the ID of the right context for this state
         *
         * @return the right context unit ID
         */
        int getRC() {
            return rc;
        }


        /**
         * Returns the next set of arcs after this state and all substates have been processed
         *
         * @return the next set of arcs
         */
        SearchStateArc[] getNextArcs() {
            SearchStateArc[] arcs;
            // this is the last state of the hmm
            // so check to see if we are at the end
            // of a word, if not get the next full hmm in the word
            // otherwise generate arcs to the next set of words

//            Pronunciation pronunciation = pState.getPronunciation();
            int nextLC = getHMM().getBaseUnit().getBaseID();

            if (!isLastUnitOfWord()) {
                arcs = pState.getSuccessors(nextLC, index + 1);
            } else {
                // we are at the end of the word, so we transit to the
                // next grammar nodes
                GrammarState gs = pState.getGrammarState();
                arcs = gs.getNextGrammarStates(nextLC, getRC());
            }
            return arcs;
        }
    }

    /** Represents a single hmm state in the search graph */
    class HMMStateSearchState extends FlatSearchState implements
            HMMSearchState, ScoreProvider {

        private final FullHMMSearchState fullHMMSearchState;
        private final HMMState hmmState;
        private final float probability;


        /**
         * Creates an HMMStateSearchState
         *
         * @param hss      the parent hmm state
         * @param hmmState which hmm state
         */
        HMMStateSearchState(FullHMMSearchState hss, HMMState hmmState) {
            this(hss, hmmState, LogMath.LOG_ONE);
        }


        /**
         * Creates an HMMStateSearchState
         *
         * @param hss      the parent hmm state
         * @param hmmState which hmm state
         * @param prob     the transition probability
         */
        HMMStateSearchState(FullHMMSearchState hss, HMMState hmmState,
                            float prob) {
            this.probability = prob;
            fullHMMSearchState = hss;
            this.hmmState = hmmState;
        }

        
        /**
         * Returns the acoustic probability for this state
         * 
         * @return the probability
         */
        @Override
        public float getInsertionProbability() {
            return probability;
        } 

        /**
         * Generate a hashcode for an object
         *
         * @return the hashcode
         */
        @Override
        public int hashCode() {
            return 7 * fullHMMSearchState.hashCode() + hmmState.hashCode();
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
            } else if (o instanceof HMMStateSearchState) {
                HMMStateSearchState other = (HMMStateSearchState) o;
                return other.fullHMMSearchState.equals(fullHMMSearchState)
                        && other.hmmState.equals(hmmState);
            } else {
                return false;
            }
        }


        /**
         * Determines if this state is an emitting state
         *
         * @return true if this is an emitting state
         */
        @Override
        public boolean isEmitting() {
            return hmmState.isEmitting();
        }


        /**
         * Gets the set of successors for this state
         *
         * @return the set of successors
         */
        @Override
        public SearchStateArc[] getSuccessors() {

            SearchStateArc[] arcs = getCachedSuccessors();
            if (arcs == null) {
                if (hmmState.isExitState()) {
                    arcs = fullHMMSearchState.getNextArcs();
                } else {
                    HMMStateArc[] next = hmmState.getSuccessors();
                    arcs = new SearchStateArc[next.length];
                    for (int i = 0; i < arcs.length; i++) {
                        arcs[i] = new
                                HMMStateSearchState(fullHMMSearchState,
                                next[i].getHMMState(),
                                next[i].getLogProbability());
                    }
                }
                cacheSuccessors(arcs);
            }
            return arcs;
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
         * Returns a unique string representation of the state. This string is suitable (and typically used) for a label
         * for a GDL node
         *
         * @return the signature
         */
        @Override
        public String getSignature() {
            return "HSSS " + fullHMMSearchState.getSignature() + '-' + hmmState;
        }


        /**
         * Returns the hmm state for this search state
         *
         * @return the hmm state
         */
        public HMMState getHMMState() {
            return hmmState;
        }


        public float getScore(Data data) {
            return hmmState.getScore(data);
        }
    }


    /** The search graph that is produced by the flat linguist. */
    class DynamicFlatSearchGraph implements SearchGraph {

        /*
        * (non-Javadoc)
        *
        * @see edu.cmu.sphinx.linguist.SearchGraph#getInitialState()
        */
        public SearchState getInitialState() {
            InitialState initialState = new InitialState();
            initialState.addArc(new GrammarState(grammar.getInitialNode()));
            // add an out-of-grammar branch if configured to do so
            if (addOutOfGrammarBranch) {
                OutOfGrammarGraph oogg = new OutOfGrammarGraph
                        (phoneLoopAcousticModel,
                                logOutOfGrammarBranchProbability,
                                logPhoneInsertionProbability);

                initialState.addArc(oogg.getOutOfGrammarGraph());
            }
            return initialState;
        }


        /*
         * (non-Javadoc)
         * 
         * @see edu.cmu.sphinx.linguist.SearchGraph#getNumStateOrder()
         */
        public int getNumStateOrder() {
            return 5;
        }
    }
}

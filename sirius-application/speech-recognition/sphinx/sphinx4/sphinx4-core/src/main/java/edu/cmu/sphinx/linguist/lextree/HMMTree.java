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

package edu.cmu.sphinx.linguist.lextree;

import edu.cmu.sphinx.linguist.WordSequence;
import edu.cmu.sphinx.linguist.acoustic.HMM;
import edu.cmu.sphinx.linguist.acoustic.HMMPool;
import edu.cmu.sphinx.linguist.acoustic.HMMPosition;
import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.linguist.dictionary.Pronunciation;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.linguist.language.ngram.LanguageModel;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.TimerPool;
import edu.cmu.sphinx.util.Utilities;

import java.util.*;
import java.util.logging.Logger;


/** Represents a node in the HMM Tree */

// For large vocabularies we may create millions of these objects,
// therefore they are extremely space sensitive. So we want to make
// these objects as small as possible.  The requirements for these
// objects when building the tree of nodes are very different from once
// we have built it. When building, we need to easily add successor
// nodes and quickly identify duplicate children nodes. After the tree
// is built we just need to quickly identify successors.  We want the
// flexibility of a map to manage successors at startup, but we don't
// want the space penalty (at least 5 32 bit fields per map), instead
// we'd like an array.  To support this dual mode, we manage the
// successors in an Object which can either be a Map or a List
// depending upon whether the node has been frozen or not.

class Node {

    private static int nodeCount;
    private static int successorCount;
    private static final Map<Pronunciation, WordNode> wordNodeMap = new HashMap<Pronunciation, WordNode>();
    
    /** 
     * This can be either Map during tree construction or Array after
     * tree freeze. Conversion to array helps to save memory.
     */
    private Object successors;
    private float logUnigramProbability;


    /**
     * Creates a node
     *
     * @param probability the unigram probability for the node
     */
    Node(float probability) {
        logUnigramProbability = probability;
        nodeCount++;
//        if ((nodeCount % 10000) == 0) {
//             System.out.println("NC " + nodeCount);
//        }
    }


    /**
     * Returns the unigram probability
     *
     * @return the unigram probability
     */
    public float getUnigramProbability() {
        return logUnigramProbability;
    }


    /**
     * Sets the unigram probability
     *
     * @param probability the unigram probability
     */
    public void setUnigramProbability(float probability) {
        logUnigramProbability = probability;
    }


    /**
     * Given an object get the set of successors for this object
     *
     * @param key the object key
     * @return the node containing the successors
     */
    private Node getSuccessor(Object key) {
        Map<Object, Node> successors = getSuccessorMap();
        return successors.get(key);
    }


    /**
     * Add the child to the set of successors
     *
     * @param key   the object key
     * @param child the child to add
     */
    void putSuccessor(Object key, Node child) {
        Map<Object, Node> successors = getSuccessorMap();
        successors.put(key, child);
    }


    /**
     * Gets the successor map for this node
     *
     * @return the successor map
     */
    @SuppressWarnings({"unchecked"})
    public Map<Object, Node> getSuccessorMap() {
        if (successors == null) {
            successors = new HashMap<Object, Node>(4);
        }

        assert successors instanceof Map;
        return (Map<Object, Node>) successors;
    }


    /** Freeze the node. Convert the successor map into an array list */
    void freeze() {
        if (successors instanceof Map<?,?>) {
            Map<Object, Node> map = getSuccessorMap();
            successors = map.values().toArray(new Node[map.size()]);
            for (Node node : map.values()) {
                node.freeze();
            }
            successorCount += map.size();
        }
    }


    static void dumpNodeInfo() {
        System.out.println("Nodes: " + nodeCount + " successors " +
                successorCount + " avg " + (successorCount / nodeCount));
    }


    /**
     * Adds a child node holding an hmm to the successor.  If a node similar to the child has already been added, we use
     * the previously added node, otherwise we add this. Also, we record the base unit of the child in the set of right
     * context
     *
     * @param hmm the hmm to add\n     * @return the node that holds the hmm (new or old)
     */
    Node addSuccessor(HMM hmm, float probability) {
        Node child = null;
        Node matchingChild = getSuccessor(hmm);
        if (matchingChild == null) {
            child = new HMMNode(hmm, probability);
            putSuccessor(hmm, child);
        } else {
            if (matchingChild.getUnigramProbability() < probability) {
                matchingChild.setUnigramProbability(probability);
            }
            child = matchingChild;
        }
        return child;
    }


    /**
     * Adds a child node holding a pronunciation to the successor. If a node similar to the child has already been
     * added, we use the previously added node, otherwise we add this. Also, we record the base unit of the child in the
     * set of right context
     *
     * @param pronunciation the pronunciation to add
     * @return the node that holds the pronunciation (new or old)
     */
    WordNode addSuccessor(Pronunciation pronunciation, float probability) {
        WordNode child = null;
        WordNode matchingChild = (WordNode) getSuccessor(pronunciation);
        if (matchingChild == null) {
            child = getWordNode(pronunciation, probability);
            putSuccessor(pronunciation, child);
        } else {
            if (matchingChild.getUnigramProbability() < probability) {
                matchingChild.setUnigramProbability(probability);
            }
            child = matchingChild;
        }
        return child;
    }


    void addSuccessor(WordNode wordNode) {
        putSuccessor(wordNode, wordNode);
    }


    /**
     * Adds an EndNode to the set of successors for this node If a node similar to the child has already been added, we
     * use the previously added node, otherwise we add this.
     *
     * @param child       the endNode to add
     * @param probability probability for this transition
     * @return the node that holds the endNode (new or old)
     */
    EndNode addSuccessor(EndNode child, float probability) {
        Unit baseUnit = child.getBaseUnit();
        EndNode matchingChild = (EndNode) getSuccessor(baseUnit);
        if (matchingChild == null) {
            putSuccessor(baseUnit, child);
        } else {
            if (matchingChild.getUnigramProbability() < probability) {
                matchingChild.setUnigramProbability(probability);
            }
            child = matchingChild;
        }
        return child;
    }


    /**
     * Gets a word node associated with the pronunciation.
     *
     * @param p the pronunciation
     * @return the word node
     */
    private WordNode getWordNode(Pronunciation p, float probability) {
        WordNode node = wordNodeMap.get(p);
        if (node == null) {
            node = new WordNode(p, probability);
            wordNodeMap.put(p, node);
        }
        return node;
    }


    /**
     * Adds a child node to the successor.  If a node similar to the child has already been added, we use the previously
     * added node, otherwise we add this. Also, we record the base unit of the child in the set of right context
     *
     * @param child the child to add
     * @return the node (may be different than child if there was already a node attached holding the hmm held by
     *         child)
     */
    UnitNode addSuccessor(UnitNode child) {
        UnitNode matchingChild = (UnitNode) getSuccessor(child.getKey());
        if (matchingChild == null) {
            putSuccessor(child.getKey(), child);
        } else {
            child = matchingChild;
        }

        return child;
    }


    /**
     * Returns the successors for this node
     *
     * @return the set of successor nodes
     */
    Node[] getSuccessors() {
        if (successors instanceof Map<?, ?>) {
            freeze();
        }
        return (Node[])successors;
    }


    /**
     * Returns the string representation for this object
     *
     * @return the string representation of the object
     */
    @Override
    public String toString() {
        return "Node ";
    }
}


/** A node representing a word in the HMM tree */
class WordNode extends Node {

    private final Pronunciation pronunciation;
    private final boolean isFinal;

    /**
     * Creates a word node
     *
     * @param pronunciation the pronunciation to wrap in this node
     * @param probability   the word unigram probability
     */
    WordNode(Pronunciation pronunciation, float probability) {
        super(probability);
        this.pronunciation = pronunciation;
        this.isFinal = pronunciation.getWord().isSentenceEndWord();
    }


    /**
     * Gets the word associated with this node
     *
     * @return the word
     */
    Word getWord() {
        return pronunciation.getWord();
    }


    /**
     * Gets the pronunciation associated with this node
     *
     * @return the pronunciation
     */
    Pronunciation getPronunciation() {
        return pronunciation;
    }


    /**
     * Gets the last unit for this word
     *
     * @return the last unit
     */
    Unit getLastUnit() {
        Unit[] units = pronunciation.getUnits();
        return units[units.length - 1];
    }


    /**
     * Returns the successors for this node
     *
     * @return the set of successor nodes
     */
    @Override
    Node[] getSuccessors() {
        throw new Error("Not supported");
    }


    /**
     * Returns a string representation for this object
     *
     * @return a string representation
     */
    @Override
    public String toString() {
        return "WordNode " + pronunciation + " p " +
                getUnigramProbability();
    }


    public boolean isFinal() {
        return isFinal;
    }
}


/**
 * A class that represents the initial word in the search space. It is treated specially because we need to keep track
 * of the context as well. The context is embodied in the parent node
 */
class InitialWordNode extends WordNode {

    final HMMNode parent;


    /**
     * Creates an InitialWordNode
     *
     * @param pronunciation the pronunciation
     * @param parent        the parent node
     */
    InitialWordNode(Pronunciation pronunciation, HMMNode parent) {
        super(pronunciation, LogMath.LOG_ONE);
        this.parent = parent;
    }


    /**
     * Gets the parent for this word node
     *
     * @return the parent
     */
    HMMNode getParent() {
        return parent;
    }

}


abstract class UnitNode extends Node {

    public final static int SIMPLE_UNIT = 1;
    public final static int WORD_BEGINNING_UNIT = 2;
    public final static int SILENCE_UNIT = 3;
    public final static int FILLER_UNIT = 4;

    private int type;


    /**
     * Creates the UnitNode
     *
     * @param probablilty the probability for the node
     */
    UnitNode(float probablilty) {
        super(probablilty);
    }


    /**
     * Returns the base unit for this hmm node
     *
     * @return the base unit
     */
    abstract Unit getBaseUnit();


    abstract Object getKey();


    abstract HMMPosition getPosition();


    /**
     * Gets the unit type (one of SIMPLE_UNIT, WORD_BEGINNING_UNIT, SIMPLE_UNIT or FILLER_UNIT
     *
     * @return the unit type
     */
    int getType() {
        return type;
    }


    /**
     * Sets the unit type
     *
     * @param type the unit type
     */
    void setType(int type) {
        this.type = type;
    }

}

/** A node that represents an HMM in the hmm tree */

class HMMNode extends UnitNode {

    private final HMM hmm;

    // There can potentially be a large number of nodes (millions),
    // therefore it is important to conserve space as much as
    // possible.  While building the HMMNodes, we keep right contexts
    // in a set to allow easy pruning of duplicates.  Once the tree is
    // entirely built, we no longer need to manage the right contexts
    // as a set, a simple array will do. The freeze method converts
    // the set to the array of units.  This rcSet object holds the set
    // during construction and the array after the freeze.

    private Object rcSet;


    /**
     * Creates the node, wrapping the given hmm
     *
     * @param hmm the hmm to hold
     */
    HMMNode(HMM hmm, float probablilty) {
        super(probablilty);
        this.hmm = hmm;

        Unit base = getBaseUnit();

        int type = SIMPLE_UNIT;
        if (base.isSilence()) {
            type = SILENCE_UNIT;
        } else if (base.isFiller()) {
            type = FILLER_UNIT;
        } else if (hmm.getPosition().isWordBeginning()) {
            type = WORD_BEGINNING_UNIT;
        }
        setType(type);
    }


    /**
     * Returns the base unit for this hmm node
     *
     * @return the base unit
     */
    @Override
    Unit getBaseUnit() {
        // return hmm.getUnit().getBaseUnit();
        return hmm.getBaseUnit();
    }


    /**
     * Returns the hmm for this node
     *
     * @return the hmm
     */
    HMM getHMM() {
        return hmm;
    }


    @Override
    HMMPosition getPosition() {
        return hmm.getPosition();
    }


    @Override
    HMM getKey() {
        return getHMM();
    }


    /**
     * Returns a string representation for this object
     *
     * @return a string representation
     */
    @Override
    public String toString() {
        return "HMMNode " + hmm + " p " + getUnigramProbability();
    }


    /**
     * Adds a right context to the set of possible right contexts for this node. This is typically only needed for hmms
     * at the ends of words.
     *
     * @param rc the right context.
     */
    void addRC(Unit rc) {
        getRCSet().add(rc);
    }


    /** Freeze this node. Convert the set into an array to reduce memory overhead */
    @Override
    @SuppressWarnings({"unchecked"})
    void freeze() {
        super.freeze();
        if (rcSet instanceof Set) {
            Set<Unit> set = (Set<Unit>) rcSet;
            rcSet = set.toArray(new Unit[set.size()]);
        }
    }


    /**
     * Gets the rc as a set. If we've already been frozen it is an error
     *
     * @return the set of right contexts
     */
    @SuppressWarnings({"unchecked"})
    private Set<Unit> getRCSet() {
        if (rcSet == null) {
            rcSet = new HashSet<Unit>();
        }

        assert rcSet instanceof HashSet;
        return (Set<Unit>) rcSet;
    }


    /**
     * returns the set of right contexts for this node
     *
     * @return the set of right contexts
     */
    Unit[] getRC() {
        if (rcSet instanceof HashSet<?>) {
            freeze();
        }
        return (Unit[]) rcSet;
    }
}


class EndNode extends UnitNode {

    final Unit baseUnit;
    final Unit leftContext;
    final Integer key;


    /**
     * Creates the node, wrapping the given hmm
     *
     * @param baseUnit    the base unit for this node
     * @param lc          the left context
     * @param probablilty the probability for the transition to this node
     */
    EndNode(Unit baseUnit, Unit lc, float probablilty) {
        super(probablilty);
        this.baseUnit = baseUnit;
        this.leftContext = lc;
        key = baseUnit.getBaseID() * 121 + leftContext.getBaseID();
    }


    /**
     * Returns the base unit for this hmm node
     *
     * @return the base unit
     */
    @Override
    Unit getBaseUnit() {
        return baseUnit;
    }


    /**
     * Returns the base unit for this hmm node
     *
     * @return the base unit
     */
    Unit getLeftContext() {
        return leftContext;
    }


    @Override
    Integer getKey() {
        return key;
    }


    @Override
    HMMPosition getPosition() {
        return HMMPosition.END;
    }


    /**
     * Returns a string representation for this object
     *
     * @return a string representation
     */
    @Override
    public String toString() {
        return "EndNode base:" + baseUnit + " lc " + leftContext + ' ' + key;
    }


    /** Freeze this node. Convert the set into an array to reduce memory overhead */
    @Override
    void freeze() {
        super.freeze();
    }
}



/**
 * Represents the vocabulary as a lex tree with nodes in the tree representing either words (WordNode) or units
 * (HMMNode). HMMNodes may be shared.
 */
class HMMTree {

    private final HMMPool hmmPool;
    private InitialWordNode initialNode;
    private Dictionary dictionary;

    private LanguageModel lm;
    private final boolean addFillerWords;
    private final boolean addSilenceWord = true;
    private final Set<Unit> entryPoints = new HashSet<Unit>();
    private Set<Unit> exitPoints = new HashSet<Unit>();
    private Set<Word> allWords;
    private EntryPointTable entryPointTable;
    private boolean debug;
    private final float languageWeight;
    private final Map<Object, HMMNode[]> endNodeMap;
    private WordNode sentenceEndWordNode;
    private Logger logger;


    /**
     * Creates the HMMTree
     *
     * @param pool           the pool of HMMs and units
     * @param dictionary     the dictionary containing the pronunciations
     * @param lm             the source of the set of words to add to the lex tree
     * @param addFillerWords if <code>false</code> add filler words
     * @param languageWeight the languageWeight
     */
    HMMTree(HMMPool pool, Dictionary dictionary, LanguageModel lm,
            boolean addFillerWords, float languageWeight) {
        this.hmmPool = pool;
        this.dictionary = dictionary;
        this.lm = lm;
        this.endNodeMap = new HashMap<Object, HMMNode[]>();
        this.addFillerWords = addFillerWords;
        this.languageWeight = languageWeight;
        
        logger = Logger.getLogger(HMMTree.class.getSimpleName());

        TimerPool.getTimer(this,"Create HMM Tree").start();
        compile();
        TimerPool.getTimer(this,"Create HMM Tree").stop();
    }


    /**
     * Given a base unit and a left context, return the set of entry points into the lex tree
     *
     * @param lc   the left context
     * @param base the center unit
     * @return the set of entry points
     */
    public Node[] getEntryPoint(Unit lc, Unit base) {
        EntryPoint ep = entryPointTable.getEntryPoint(base);
        return ep.getEntryPointsFromLeftContext(lc).getSuccessors();
    }


    /**
     * Gets the  set of hmm nodes associated with the given end node
     *
     * @param endNode the end node
     * @return an array of associated hmm nodes
     */
    public HMMNode[] getHMMNodes(EndNode endNode) {
        HMMNode[] results = endNodeMap.get(endNode.getKey());
        if (results == null) {
            // System.out.println("Filling cache for " + endNode.getKey()
            //        + " size " + endNodeMap.size());
            Map<HMM, HMMNode> resultMap = new HashMap<HMM, HMMNode>();
            Unit baseUnit = endNode.getBaseUnit();
            Unit lc = endNode.getLeftContext();
            for (Unit rc : entryPoints) {
                HMM hmm = hmmPool.getHMM(baseUnit, lc, rc, HMMPosition.END);
                HMMNode hmmNode = resultMap.get(hmm);
                if (hmmNode == null) {
                    hmmNode = new HMMNode(hmm, LogMath.LOG_ONE);
                    resultMap.put(hmm, hmmNode);
                }
                hmmNode.addRC(rc);
                for (Node node : endNode.getSuccessors()) {
                    WordNode wordNode = (WordNode)node;
                    hmmNode.addSuccessor(wordNode);
                }
            }

            // cache it
            results = resultMap.values().toArray(new HMMNode[resultMap.size()]);
            endNodeMap.put(endNode.getKey(), results);
        }

        // System.out.println("GHN: " + endNode + " " + results.length);
        return results;
    }


    /**
     * Returns the word node associated with the sentence end word
     *
     * @return the sentence end word node
     */
    public WordNode getSentenceEndWordNode() {
        assert sentenceEndWordNode != null;
        return sentenceEndWordNode;
    }


//    private Object getKey(EndNode endNode) {
//        Unit base = endNode.getBaseUnit();
//        Unit lc = endNode.getLeftContext();
//        return null;
//    }


    /** Compiles the vocabulary into an HMM Tree */
    private void compile() {
        collectEntryAndExitUnits();
        entryPointTable = new EntryPointTable(entryPoints);
        addWords();
        entryPointTable.createEntryPointMaps();
        freeze();
    }


    /** Dumps the tree */
    void dumpTree() {
        System.out.println("Dumping Tree ...");
        Map<Node, Node> dupNode = new HashMap<Node, Node>();
        dumpTree(0, getInitialNode(), dupNode);
        System.out.println("... done Dumping Tree");
    }


    /**
     * Dumps the tree
     *
     * @param level   the level of the dump
     * @param node    the root of the tree to dump
     * @param dupNode map of visited nodes
     */
    private void dumpTree(int level, Node node, Map<Node, Node> dupNode) {
        if (dupNode.get(node) == null) {
            dupNode.put(node, node);
            System.out.println(Utilities.pad(level) + node);
            if (!(node instanceof WordNode)) {
                for (Node nextNode : node.getSuccessors()) {
                    dumpTree(level + 1, nextNode, dupNode);
                }
            }
        }
    }


    /** Collects all of the entry and exit points for the vocabulary. */
    private void collectEntryAndExitUnits() {
        Collection<Word> words = getAllWords();
        for (Word word : words) {
            for (int j = 0; j < word.getPronunciations().length; j++) {
                Pronunciation p = word.getPronunciations()[j];
                Unit first = p.getUnits()[0];
                Unit last = p.getUnits()[p.getUnits().length - 1];
                entryPoints.add(first);
                exitPoints.add(last);
            }
        }

        if (debug) {
            System.out.println("Entry Points: " + entryPoints.size());
            System.out.println("Exit Points: " + exitPoints.size());
        }
    }


    /**
     * Called after the lex tree is built. Frees all temporary structures. After this is called, no more words can be
     * added to the lex tree.
     */
    private void freeze() {
        entryPointTable.freeze();
        dictionary = null;
        lm = null;
        exitPoints = null;
        allWords = null;
    }


    /** Adds the given collection of words to the lex tree */
    private void addWords() {
        Set<Word> words = getAllWords();
        for (Word word : words) {
            addWord(word);
        }
    }


    /**
     * Adds a single word to the lex tree
     *
     * @param word the word to add
     */
    private void addWord(Word word) {
        float prob = getWordUnigramProbability(word);
        Pronunciation[] pronunciations = word.getPronunciations();
        for (Pronunciation pronunciation : pronunciations) {
            addPronunciation(pronunciation, prob);
        }
    }


    /**
     * Adds the given pronunciation to the lex tree
     *
     * @param pronunciation the pronunciation
     * @param probability   the unigram probability
     */
    private void addPronunciation(Pronunciation pronunciation,
                                  float probability) {
        Unit baseUnit;
        Unit lc;
        Unit rc;
        Node curNode;
        WordNode wordNode;

        Unit[] units = pronunciation.getUnits();
        baseUnit = units[0];
        EntryPoint ep = entryPointTable.getEntryPoint(baseUnit);

        ep.addProbability(probability);

        if (units.length > 1) {
            curNode = ep.getNode();
            lc = baseUnit;
            for (int i = 1; i < units.length - 1; i++) {
                baseUnit = units[i];
                rc = units[i + 1];
                HMM hmm = hmmPool.getHMM(baseUnit, lc, rc, HMMPosition.INTERNAL);
                if (hmm == null) {
                    logger.severe("Missing HMM for unit " + baseUnit.getName() + " with lc=" + lc.getName() + " rc=" + rc.getName());
                } else {
                    curNode = curNode.addSuccessor(hmm, probability);
                }
                lc = baseUnit;          // next lc is this baseUnit
            }

            // now add the last unit as an end unit
            baseUnit = units[units.length - 1];
            EndNode endNode = new EndNode(baseUnit, lc, probability);
            curNode = curNode.addSuccessor(endNode, probability);
            wordNode = curNode.addSuccessor(pronunciation, probability);
            if (wordNode.getWord().isSentenceEndWord()) {
                sentenceEndWordNode = wordNode;
            }
        } else {
            ep.addSingleUnitWord(pronunciation);
        }
    }

    
    /**
     * Gets the unigram probability for the given word
     *
     * @param word the word
     * @return the unigram probability for the word.
     */
    private float getWordUnigramProbability(Word word) {
        float prob = LogMath.LOG_ONE;
        if (!word.isFiller()) {
            Word[] wordArray = new Word[1];
            wordArray[0] = word;
            prob = lm.getProbability((new WordSequence(wordArray)));
            // System.out.println("gwup: " + word + " " + prob);
            prob *= languageWeight;
        }
        return prob;
    }


    /**
     * Returns the entire set of words, including filler words
     *
     * @return the set of all words (as Word objects)
     */
    private Set<Word> getAllWords() {
        if (allWords == null) {
            allWords = new HashSet<Word>();
            for (String spelling : lm.getVocabulary()) {
                Word word = dictionary.getWord(spelling);
                if (word != null) {
                    allWords.add(word);
                }
            }

            if (addFillerWords) {
                allWords.addAll(Arrays.asList(dictionary.getFillerWords()));
            } else if (addSilenceWord) {
                allWords.add(dictionary.getSilenceWord());
            }
        }
        return allWords;
    }


    /**
     * Returns the initial node for this lex tree
     *
     * @return the initial lex node
     */
    InitialWordNode getInitialNode() {
        return initialNode;
    }


    /** The EntryPoint table is used to manage the set of entry points into the lex tree. */
    class EntryPointTable {

        private final Map<Unit, EntryPoint> entryPoints;


        /**
         * Create the entry point table give the set of all possible entry point units
         *
         * @param entryPointCollection the set of possible entry points
         */
        EntryPointTable(Collection<Unit> entryPointCollection) {
            entryPoints = new HashMap<Unit, EntryPoint>();
            for (Unit unit : entryPointCollection) {
                entryPoints.put(unit, new EntryPoint(unit));
            }
        }


        /**
         * Given a CI unit, return the EntryPoint object that manages the entry point for the unit
         *
         * @param baseUnit the unit of interest (A ci unit)
         * @return the object that manages the entry point for the unit
         */
        EntryPoint getEntryPoint(Unit baseUnit) {
            return entryPoints.get(baseUnit);
        }


        /** Creates the entry point maps for all entry points. */
        void createEntryPointMaps() {
            for (EntryPoint ep : entryPoints.values()) {
                ep.createEntryPointMap();
            }
        }


        /** Freezes the entry point table */
        void freeze() {
            for (EntryPoint ep : entryPoints.values()) {
                ep.freeze();
            }
        }


        /** Dumps the entry point table */
        void dump() {
            for (EntryPoint ep : entryPoints.values()) {
                ep.dump();
            }
        }
    }


    /** Manages a single entry point. */
    class EntryPoint {

        final Unit baseUnit;
        final Node baseNode;      // second units and beyond start here
        final Map<Unit, Node> unitToEntryPointMap;
        List<Pronunciation> singleUnitWords;
        int nodeCount;
        Set<Unit> rcSet;
        float totalProbability;


        /**
         * Creates an entry point for the given unit
         *
         * @param baseUnit the EntryPoint is created for this unit
         */
        EntryPoint(Unit baseUnit) {
            this.baseUnit = baseUnit;
            this.baseNode = new Node(LogMath.LOG_ZERO);
            this.unitToEntryPointMap = new HashMap<Unit, Node>();
            this.singleUnitWords = new ArrayList<Pronunciation>();
            this.totalProbability = LogMath.LOG_ZERO;
        }


        /**
         * Given a left context get a node that represents a single set of entry points into this unit
         *
         * @param leftContext the left context of interest
         * @return the node representing the entry point
         */
        Node getEntryPointsFromLeftContext(Unit leftContext) {
            return unitToEntryPointMap.get(leftContext);
        }


        /**
         * Accumulates the probability for this entry point
         *
         * @param probability a new  probability
         */
        void addProbability(float probability) {
            if (probability > totalProbability) {
                totalProbability = probability;
            }
        }


        /**
         * Returns the probability for all words reachable from this node
         *
         * @return the log probability
         */
        float getProbability() {
            return totalProbability;
        }


        /** Once we have built the full entry point we can eliminate some fields */
        void freeze() {
            for (Node node : unitToEntryPointMap.values()) {
                node.freeze();
            }
            singleUnitWords = null;
            rcSet = null;
        }


        /**
         * Gets the base node for this entry point
         *
         * @return the base node
         */
        Node getNode() {
            return baseNode;
        }


        /**
         * Adds a one-unit word to this entry point. Such single unit words need to be dealt with specially.
         *
         * @param p the pronunciation of the single unit word
         */
        void addSingleUnitWord(Pronunciation p) {
            singleUnitWords.add(p);
        }


        /**
         * Gets the set of possible right contexts that we can transition to from this entry point
         *
         * @return the set of possible transition points.
         */
        private Collection<Unit> getEntryPointRC() {
            if (rcSet == null) {
                rcSet = new HashSet<Unit>();
                for (Node node : baseNode.getSuccessorMap().values()) {
                    UnitNode unitNode = (UnitNode) node;
                    rcSet.add(unitNode.getBaseUnit());
                }
            }
            return rcSet;
        }


        /**
         * A version of createEntryPointMap that compresses common hmms across all entry points.
         */
        void createEntryPointMap() {
            HashMap<HMM, Node> map = new HashMap<HMM, Node>();
            HashMap<HMM, HMMNode> singleUnitMap = new HashMap<HMM, HMMNode>();

            for (Unit lc : exitPoints) {
                Node epNode = new Node(LogMath.LOG_ZERO);
                for (Unit rc : getEntryPointRC()) {
                    HMM hmm = hmmPool.getHMM(baseUnit, lc, rc, HMMPosition.BEGIN);
                    Node addedNode;

                    if ((addedNode = map.get(hmm)) == null) {
                        addedNode = epNode.addSuccessor(hmm, getProbability());
                        map.put(hmm, addedNode);
                    } else {
                        epNode.putSuccessor(hmm, addedNode);
                    }

                    nodeCount++;
                    connectEntryPointNode(addedNode, rc);
                }
                connectSingleUnitWords(lc, epNode, singleUnitMap);
                unitToEntryPointMap.put(lc, epNode);
            }
        }


        /**
         * Connects the single unit words associated with this entry point.   The singleUnitWords list contains all
         * single unit pronunciations that have as their sole unit, the unit associated with this entry point. Entry
         * points for these words are added to the epNode for all possible left (exit) and right (entry) contexts.
         *
         * @param lc     the left context
         * @param epNode the entry point node
         */
        private void connectSingleUnitWords(Unit lc, Node epNode, HashMap<HMM, HMMNode> map) {
            if (!singleUnitWords.isEmpty()) {
                
                for (Unit rc : entryPoints) {
                    HMM hmm = hmmPool.getHMM(baseUnit, lc, rc, HMMPosition.SINGLE);
                    
                    HMMNode tailNode;
                    if (( tailNode = map.get(hmm)) == null) {
                        tailNode = (HMMNode)
                                epNode.addSuccessor(hmm, getProbability());
                        map.put(hmm, tailNode);
                    } else {
                        epNode.putSuccessor(hmm, tailNode);
                    }
                    WordNode wordNode;
                    tailNode.addRC(rc);
                    nodeCount++;

                    for (Pronunciation p : singleUnitWords) {
                        if (p.getWord() == dictionary.getSentenceStartWord()) {
                            initialNode = new InitialWordNode(p, tailNode);
                        } else {
                            float prob = getWordUnigramProbability(p.getWord());
                            wordNode = tailNode.addSuccessor(p, prob);
                            if (p.getWord() ==
                                dictionary.getSentenceEndWord()) {
                                sentenceEndWordNode = wordNode;
                            }
                        }
                        nodeCount++;
                    }
                }
            }
        }


        /**
         * Connect the entry points that match the given rc to the given epNode
         *
         * @param epNode add matching successors here
         * @param rc     the next unit
         */
        private void connectEntryPointNode(Node epNode, Unit rc) {
            for (Node node : baseNode.getSuccessors()) {
                UnitNode successor = (UnitNode) node;
                if (successor.getBaseUnit() == rc) {
                    epNode.addSuccessor(successor);
                }
            }
        }


        /** Dumps the entry point */
        void dump() {
            System.out.println("EntryPoint " + baseUnit + " RC Followers: "
                    + getEntryPointRC().size());
            int count = 0;
            Collection<Unit> rcs = getEntryPointRC();
            System.out.print("    ");
            for (Unit rc : rcs) {
                System.out.print(Utilities.pad(rc.getName(), 4));
                if (count++ >= 12) {
                    count = 0;
                    System.out.println();
                    System.out.print("    ");
                }
            }
            System.out.println();
        }
    }


}


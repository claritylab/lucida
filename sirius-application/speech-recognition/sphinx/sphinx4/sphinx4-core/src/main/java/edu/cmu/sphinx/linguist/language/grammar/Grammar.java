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
package edu.cmu.sphinx.linguist.language.grammar;

import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.util.Timer;
import edu.cmu.sphinx.util.TimerPool;
import edu.cmu.sphinx.util.props.*;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Classes that implement this interface create grammars. A grammar is represented internally as a graph of {@link
 * GrammarNode GrammarNodes} linked together by {@link GrammarArc GrammarArcs}. Calling {@link #getInitialNode()
 * getInitialNode} will return the first node of the grammar graph. To traverse the grammar graph, one should call
 * GrammarNode.getSuccessors, which will return an array of GrammarArcs, from which you can reach the neighboring
 * GrammarNodes.
 * <p/>
 * Note that all grammar probabilities are maintained in LogMath log domain.
 */

public abstract class Grammar implements Configurable, GrammarInterface {

    /** Property to control the the dumping of the grammar */
    @S4Boolean(defaultValue = false)
    public final static String PROP_SHOW_GRAMMAR = "showGrammar";
    /** The default value for PROP_SHOW_GRAMMAR. */

    @S4Boolean(defaultValue = true)
    public final static String PROP_OPTIMIZE_GRAMMAR = "optimizeGrammar";

    /** Property to control whether silence words are inserted into the graph */
    @S4Boolean(defaultValue = false)
    public final static String PROP_ADD_SIL_WORDS = "addSilenceWords";

    /** Property to control whether filler words are inserted into the graph */
    @S4Boolean(defaultValue = false)
    public final static String PROP_ADD_FILLER_WORDS = "addFillerWords";

    /** Property that defines the dictionary to use for this grammar */
    @S4Component(type = Dictionary.class)
    public final static String PROP_DICTIONARY = "dictionary";

    // ----------------------------
    // Configuration data
    // -----------------------------
    protected Logger logger;

    private boolean optimizeGrammar = true;
    private boolean addSilenceWords;
    private boolean addFillerWords;
    protected Dictionary dictionary;
    protected GrammarNode initialNode;
    private Set<GrammarNode> grammarNodes;

    private final static Word[][] EMPTY_ALTERNATIVE = new Word[0][0];
    private final Random randomizer = new Random(56); // use fixed initial to make get deterministic random value for testing
    private int maxIdentity;
    private boolean idCheck;

    public Grammar(boolean showGrammar,boolean optimizeGrammar,boolean addSilenceWords, boolean addFillerWords, Dictionary dictionary ) {
        this.logger = Logger.getLogger(getClass().getName());
        this.optimizeGrammar = optimizeGrammar;
        this.addSilenceWords = addSilenceWords;
        this.addFillerWords = addFillerWords;
        this.dictionary = dictionary;
    }

    public Grammar() {

    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();
        optimizeGrammar = ps.getBoolean(PROP_OPTIMIZE_GRAMMAR);

        addSilenceWords = ps.getBoolean(PROP_ADD_SIL_WORDS);
        addFillerWords = ps.getBoolean(PROP_ADD_FILLER_WORDS);

        dictionary = (Dictionary) ps.getComponent(PROP_DICTIONARY);
    }


    /** Create the grammar
     * @throws java.io.IOException*/
    public void allocate() throws IOException {
        dictionary.allocate();
        newGrammar();
        Timer timer = TimerPool.getTimer(this, "grammarLoad");
        timer.start();
        initialNode = createGrammar();
        timer.stop();
    }


    /** Deallocate resources allocated to this grammar */
    public void deallocate() {
        initialNode = null;
        grammarNodes = null;
        dictionary.deallocate();
    }


    /**
     * Returns the initial node for the grammar
     *
     * @return the initial grammar node
     */
    public GrammarNode getInitialNode() {
        return initialNode;
    }


    /**
     * Perform the standard set of grammar post processing. This can include
     * inserting silence nodes and optimizing out empty nodes
     */
    protected void postProcessGrammar() {
        if (addFillerWords) {
            addFillerWords();
        } else if (addSilenceWords) {
            addSilenceWords();
        }

        if (optimizeGrammar) {
            optimizeGrammar();
        }
        dumpStatistics();
        if (true) {
            dumpGrammar("grammar.gdl");
            dumpRandomSentences("sentences.txt", 100);
            logger.info("Total number of nodes " + grammarNodes.size());
        }
    }


    /** Dumps statistics for this grammar */
    public void dumpStatistics() {
        if (logger.isLoggable(Level.INFO)) {
            int successorCount = 0;
            logger.info("Num nodes : " + getNumNodes());
            for (GrammarNode grammarNode : grammarNodes)
                successorCount += grammarNode.getSuccessors().length;

            logger.info("Num arcs  : " + successorCount);
            logger.info("Avg arcs  : "
                    + ((float) successorCount / getNumNodes()));
        }
    }


    /**
     * Dump a set of random sentences that fit this grammar
     *
     * @param path  the name of the file to dump the sentences to
     * @param count dumps no more than this. May dump less than this depending upon the number of uniqe sentences in the
     *              grammar.
     */
    public void dumpRandomSentences(String path, int count) {
        try {
            Set<String> set = new HashSet<String>();
            PrintWriter out = new PrintWriter(new FileOutputStream(path));
            for (int i = 0; i < count; i++) {
                String s = getRandomSentence();
                if (!set.contains(s)) {
                    set.add(s);
                    out.println(s);
                }
            }
            out.close();
        } catch (IOException ioe) {
            logger.severe("Can't write random sentences to " + path + ' ' + ioe);
        }
    }


    /**
     * Dump a set of random sentences that fit this grammar
     *
     * @param count dumps no more than this. May dump less than this depending upon the number of uniqe sentences in the
     *              grammar.
     */
    public void dumpRandomSentences(int count) {
        Set<String> set = new HashSet<String>();
        for (int i = 0; i < count; i++) {
            String s = getRandomSentence();
            if (!set.contains(s)) {
                set.add(s);
            }
        }
        List<String> sampleList = new ArrayList<String>(set);
        Collections.sort(sampleList);

        for (String sentence : sampleList) {
            System.out.println(sentence);
        }
    }


    /**
     * Returns a random sentence that fits this grammar
     *
     * @return a random sentence that fits this grammar
     */
    public String getRandomSentence() {
        StringBuilder sb = new StringBuilder();
        GrammarNode node = getInitialNode();
        while (!node.isFinalNode()) {
            if (!node.isEmpty()) {
                Word word = node.getWord();
                if (!word.isFiller())
                    sb.append(word.getSpelling()).append(' ');
            }
            node = selectRandomSuccessor(node);
        }
        return sb.toString().trim();
    }


    /**
     * Given a node, select a random successor from the set of possible successor nodes
     *
     * @param node the node
     * @return a random successor node.
     */
    private GrammarNode selectRandomSuccessor(GrammarNode node) {
        GrammarArc[] arcs = node.getSuccessors();

        // select a transition arc with respect to the arc-probabilities (which are log and we don't have a logMath here
        // which makes the implementation a little bit messy
        if (arcs.length > 1) {
            double[] linWeights = new double[arcs.length];
            double linWeightsSum = 0;

            final double EPS = 1E-10;

            for (int i = 0; i < linWeights.length; i++) {
                linWeights[i] = (arcs[0].getProbability() + EPS) / (arcs[i].getProbability() + EPS);
                linWeightsSum += linWeights[i];
            }

            for (int i = 0; i < linWeights.length; i++) {
                linWeights[i] /= linWeightsSum;
            }


            double selIndex = randomizer.nextDouble();
            int index = 0;
            for (int i = 0; selIndex > EPS; i++) {
                index = i;
                selIndex -= linWeights[i];
            }

            return arcs[index].getGrammarNode();

        } else {
            return arcs[0].getGrammarNode();
        }
    }


    /** Dumps the grammar
     * @param name*/
    public void dumpGrammar(String name) {
        getInitialNode().dumpDot(name);
    }

    /**
     * returns the number of nodes in this grammar
     *
     * @return the number of nodes
     */
    public int getNumNodes() {
        return grammarNodes.size();
    }


    /**
     * returns the set of of nodes in this grammar
     *
     * @return the set of nodes
     */
    public Set<GrammarNode> getGrammarNodes() {
        return grammarNodes;
    }


    /** Prepare to create a new grammar */
    protected void newGrammar() {
        maxIdentity = 0;
        grammarNodes = new HashSet<GrammarNode>();
        initialNode = null;
    }


    /**
     * Creates a grammar. Subclasses of grammar should implement this method.
     *
     * @return the initial node for the grammar
     * @throws java.io.IOException if the grammar could not be loaded
     */
    protected abstract GrammarNode createGrammar() throws IOException;


    /**
     * Create class from reference text (not implemented).
     *
     * @param bogusText dummy variable
     * @throws NoSuchMethodException if called with reference sentence
     */
    protected GrammarNode createGrammar(String bogusText)
            throws NoSuchMethodException {
        throw new NoSuchMethodException("Does not create "
                + "grammar with reference text");
    }


    /**
     * Gets the dictionary for this grammar
     *
     * @return the dictionary
     */
    public Dictionary getDictionary() {
        return dictionary;
    }


    /**
     * Returns a new GrammarNode with the given set of alternatives.
     *
     * @param identity the id for this node
     * @param alts     the set of alternative word lists for this GrammarNode
     */
    protected GrammarNode createGrammarNode(int identity, String[][] alts) {
        GrammarNode node;
        Word[][] alternatives = new Word[alts.length][];
        for (int i = 0; i < alternatives.length; i++) {
            alternatives[i] = new Word[alts[i].length];
            for (int j = 0; j < alts[i].length; j++) {
                Word word = getDictionary().getWord(alts[i][j]);
                // Pronunciation[] pronunciation =
                // word.getPronunciations(null);
                if (word == null) {
                    alternatives = EMPTY_ALTERNATIVE;
                    break;
                } else {
                    alternatives[i][j] = word;
                }
            }
        }
        node = new GrammarNode(identity, alternatives);
        add(node);

        return node;
    }


    /**
     * Returns a new GrammarNode with the given single word. If the word is not in the dictionary, an empty node is
     * created. The grammar id is automatically assigned
     *
     * @param word the word for this grammar node
     */

    protected GrammarNode createGrammarNode(String word) {
        GrammarNode node = createGrammarNode(maxIdentity + 1, word);
        return node;
    }


    /**
     * Creates an empty  grammar node in this grammar. The gramar ID is automatically assigned.
     *
     * @param isFinal if true, this is a final node
     * @return the grammar node
     */
    protected GrammarNode createGrammarNode(boolean isFinal) {
        return createGrammarNode(maxIdentity + 1, isFinal);
    }


    /**
     * Returns a new GrammarNode with the given single word. If the word is not in the dictionary, an empty node is
     * created
     *
     * @param identity the id for this node
     * @param word     the word for this grammar node
     */
    protected GrammarNode createGrammarNode(int identity, String word) {
        GrammarNode node;
        Word[][] alternatives = EMPTY_ALTERNATIVE;
        Word wordObject = getDictionary().getWord(word);
        // Pronunciation[] pronunciation = wordObject.getPronunciations(null);
        if (wordObject != null) {
            alternatives = new Word[1][];
            alternatives[0] = new Word[1];
            alternatives[0][0] = wordObject;
            node = new GrammarNode(identity, alternatives);
            add(node);
        } else {
            node = createGrammarNode(identity, false);
            logger.warning("Can't find pronunciation for " + word);
        }
        return node;
    }


    /**
     * Creates a grammar node in this grammar with the given identity
     *
     * @param identity the identity of the node
     * @param isFinal  if true, this is a final node
     * @return the grammar node
     */
    protected GrammarNode createGrammarNode(int identity, boolean isFinal) {
        GrammarNode node;
        node = new GrammarNode(identity, isFinal);
        add(node);
        return node;
    }


    /**
     * Adds the given grammar node to the set of nodes for this grammar
     *
     * @param node the grammar node
     * @throws Error
     */
    private void add(GrammarNode node) throws Error {
        if (node.getID() > maxIdentity) {
            maxIdentity = node.getID();
        }

        // check to see if there is already a node with the given ID.
        if (idCheck) {
            for (GrammarNode grammarNode : grammarNodes) {
                if (grammarNode.getID() == node.getID()) {

                    throw new Error("DUP ID " + grammarNode + " and " + node);
                }
            }
        }

        grammarNodes.add(node);

    }


    /**
     * Eliminate unnecessary nodes from the grammar. This method goes through the grammar and looks for branches to
     * nodes that have no words and have only a single exit and bypasses these nodes.
     */
    private void optimizeGrammar() {
        Set<GrammarNode> nodes = getGrammarNodes();
        for (GrammarNode node : nodes)
            node.optimize();
    }


    /** Adds an optional silence word after every non-filler word in the grammar */
    private void addSilenceWords() {
        HashSet<GrammarNode> nodes = new HashSet<GrammarNode>(getGrammarNodes());
        for (GrammarNode g : nodes) {
            if (!g.isEmpty() && !g.getWord().isFiller()) {
                GrammarNode silNode = createGrammarNode(maxIdentity + 1,
                        dictionary.getSilenceWord().getSpelling());

                GrammarNode branchNode = g.splitNode(maxIdentity + 1);
                add(branchNode);

                g.add(silNode, 0.00f);
                silNode.add(branchNode, 0.0f);
                silNode.add(silNode, 0.0f);
            }
        }
    }


    /** Adds an optional filler word loop after every non-filler word in the grammar */
    private void addFillerWords() {
        Set<GrammarNode> nodes = new HashSet<GrammarNode>(getGrammarNodes());

        Word[] fillers = getInterWordFillers();

        if (fillers.length == 0) {
            return;
        }

        for (GrammarNode wordNode : nodes) {
            if (!wordNode.isEmpty() && !wordNode.getWord().isFiller()) {
                GrammarNode wordExitNode = wordNode.splitNode(maxIdentity + 1);
                add(wordExitNode);
                GrammarNode fillerStart = createGrammarNode(false);
                GrammarNode fillerEnd = createGrammarNode(false);
                fillerEnd.add(fillerStart, 0.0f);
                fillerEnd.add(wordExitNode, 0.0f);
                wordNode.add(fillerStart, 0.0f);

                for (Word filler : fillers) {
                    GrammarNode fnode = createGrammarNode(maxIdentity + 1, filler.getSpelling());
                    fillerStart.add(fnode, 0.0f);
                    fnode.add(fillerEnd, 0.0f);
                }
            }
        }
    }


    /**
     * Gets the set of fillers after filtering out fillers that don't go between words.
     *
     * @return the set of inter-word fillers
     */
    private Word[] getInterWordFillers() {
        List<Word> fillerList = new ArrayList<Word>();
        Word[] fillers = dictionary.getFillerWords();

        for (Word fillerWord : fillers) {
            if (fillerWord != dictionary.getSentenceStartWord()
                    && fillerWord != dictionary.getSentenceEndWord()) {
                fillerList.add(fillerWord);
            }
        }
        return fillerList.toArray(new Word[fillerList.size()]);
    }
}

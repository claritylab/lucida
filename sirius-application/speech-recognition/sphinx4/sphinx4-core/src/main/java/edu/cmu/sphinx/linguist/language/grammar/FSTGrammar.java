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
import edu.cmu.sphinx.util.ExtendedStreamTokenizer;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.*;

import java.io.IOException;
import java.util.*;

/**
 * Loads a grammar from a file representing a finite-state transducer (FST) in the 'ARPA' grammar format. The ARPA FST
 * format is like so (the explanation of the format is below): <br>
 * <p/>
 * <pre>
 *  I 2
 *  F 0 2.30259
 *  T 0 1 &lt;unknown&gt; &lt;unknown&gt; 2.30259
 *  T 0 4 wood wood 1.60951
 *  T 0 5 cindy cindy 1.60951
 *  T 0 6 pittsburgh pittsburgh 1.60951
 *  T 0 7 jean jean 1.60951
 *  F 1 2.89031
 *  T 1 0 , , 0.587725
 *  T 1 4 wood wood 0.58785
 *  F 2 3.00808
 *  T 2 0 , , 0.705491
 *  T 2 1 &lt;unknown&gt; &lt;unknown&gt; 0.58785
 *  F 3 2.30259
 *  T 3 0
 *  F 4 2.89031
 *  T 4 0 , , 0.587725
 *  T 4 6 pittsburgh pittsburgh 0.58785
 *  F 5 2.89031
 *  T 5 0 , , 0.587725
 *  T 5 7 jean jean 0.58785
 *  F 6 2.89031
 *  T 6 0 , , 0.587725
 *  T 6 5 cindy cindy 0.58785
 *  F 7 1.28093
 *  T 7 0 , , 0.454282
 *  T 7 4 wood wood 1.28093
 *   </pre>
 * <p/>
 * <b>Key: </b>
 * <p/>
 * <pre>
 *  I - initial node, so &quot;I 2&quot; means node 2 is the initial node
 *  F - final node, e.g., &quot;F 0 2.30259&quot; means that node 0 is a final node,
 *  and the probability of finishing at node 0 is 2.30259 (in -ln)
 *  T - transition, &quot;T 0 4 wood wood 1.60951&quot; means &quot;transitioning from
 *  node 0 to node 4, the output is wood and the machine is now
 *  in the node wood, and the probability associated with the
 *  transition is 1.60951 (in -ln)&quot;. &quot;T 6 0 , , 0.587725&quot; is
 *  a backoff transition, and the output is null (epsilon in
 *  the picture), and the machine is now in the null node.
 *   </pre>
 * <p/>
 * <p/>
 * Probabilities read in from the FST file are in negative natural log format and are converted to the internal logMath
 * log base.
 * <p/>
 * As the FST file is read in, a Grammar object that is structurally equivalent to the FST is created. The steps of
 * converting the FST file to a Grammar object are: <ol>
 * <p/>
 * <li><b>Create all the Grammar nodes </b> <br> Go through the entire FST file and for each word transition, take the
 * destination node ID and create a grammar node using that ID. These nodes are kept in a hashmap to make sure they
 * are created once for each ID. Therefore, we get one word per grammar node.</li>
 * <p/>
 * <br>
 * <p/>
 * <li><b>Create an end node for each Grammar node </b> <br> This is end node is used for backoff transitions into the
 * Grammar node, so that it will not go through the word itself, but instead go directly to the end of the word.
 * Moreover, we also add an <b>optional </b> silence node between the grammar node and its end node. The result of this
 * step on each grammar node (show in Figure 1 below as the circle with "word") is as follows. The end node is the empty
 * circle at the far right: <br> <img src="doc-files/fst-end-node.jpg"> <br> <b>Figure 1: Addition of end node and the
 * <i>optional </i> silence. </b> </li>
 * <p/>
 * <br>
 * <p/>
 * <li><b>Create the transitions </b> <br> Read through the entire FST file, and for each line indicating a transition,
 * connect up the corresponding Grammar nodes. Backoff transitions and null transitions (i.e., the ones that do not
 * output a word) will be linked to the end node of a grammar node.</li>
 * <p/>
 * </ol>
 */

public class FSTGrammar extends Grammar {

    /** The property for the location of the FST n-gram file. */
    @S4String(defaultValue = "default.arpa_gram")
    public final static String PROP_PATH = "path";

    // TODO: If this property turns out to be worthwhile, turn this
    // into a full fledged property
    private boolean addInitialSilenceNode;

    // TODO: If this property turns out to be worthwhile, turn this
    // into a full fledged property

    // ------------------------------
    // Configuration data
    // -------------------------------

    private boolean addOptionalSilence;
    private final boolean ignoreUnknownTransitions = true;
    private String path;
    private LogMath logMath;

    private final Map<String, GrammarNode> nodes = new HashMap<String, GrammarNode>();
    private final Set<GrammarNode> expandedNodes = new HashSet<GrammarNode>();


    /**
     * Create class from reference text (not implemented).
     *
     * @param bogusText dummy variable
     */
    @Override
    protected GrammarNode createGrammar(String bogusText)
            throws NoSuchMethodException {
        throw new NoSuchMethodException("Does not create "
                + "grammar with reference text");
    }


    public FSTGrammar(String path, boolean showGrammar, boolean optimizeGrammar, boolean addSilenceWords, boolean addFillerWords, Dictionary dictionary) {
        super(showGrammar,optimizeGrammar,addSilenceWords,addFillerWords,dictionary);
        this.path = path;
        logMath = LogMath.getInstance();
    }

    public FSTGrammar() {

    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        
        path = ps.getString(PROP_PATH);
    }


    /**
     * Creates the grammar.
     *
     * @return the initial node for the grammar.
     */
    @Override
    protected GrammarNode createGrammar() throws IOException {

        GrammarNode initialNode = null;
        GrammarNode finalNode = null;

        // first pass create the FST nodes
        int maxNodeId = createNodes(path);

        // create the final node:
        finalNode = createGrammarNode(++maxNodeId, Dictionary.SILENCE_SPELLING);
        finalNode.setFinalNode(true);

        // replace each word node with a pair of nodes, which
        // consists of the word node and a new dummy end node, which is
        // for adding null or backoff transitions
        maxNodeId = expandWordNodes(maxNodeId);

        ExtendedStreamTokenizer tok = new ExtendedStreamTokenizer(path, true);

        // Second pass, add all of the arcs

        while (!tok.isEOF()) {
            String token;
            tok.skipwhite();
            token = tok.getString();

            // System.out.println(token);

            if (token == null) {
                break;

            } else if (token.equals("I")) {
                assert initialNode == null;
                int initialID = tok.getInt("initial ID");
                String nodeName = "G" + initialID;

                // TODO: FlatLinguist requires the initial grammar node
                // to contain a single silence. We'll do that for now,
                // but once the FlatLinguist is fixed, this should be
                // returned to its former method of creating an empty
                // initial grammar node
                //          initialNode = createGrammarNode(initialID, false);

                initialNode = createGrammarNode(initialID,
                        Dictionary.SILENCE_SPELLING);
                nodes.put(nodeName, initialNode);

                // optionally add a silence node
                if (addInitialSilenceNode) {
                    GrammarNode silenceNode =
                            createGrammarNode(++maxNodeId,
                                    Dictionary.SILENCE_SPELLING);
                    initialNode.add(silenceNode, LogMath.LOG_ONE);
                    silenceNode.add(initialNode, LogMath.LOG_ONE);
                }

            } else if (token.equals("T")) {
                int thisID = tok.getInt("this id");
                int nextID = tok.getInt("next id");

                GrammarNode thisNode = get(thisID);
                GrammarNode nextNode = get(nextID);

                // if the source node is an FSTGrammarNode, we want
                // to join the endNode to the destination node

                if (hasEndNode(thisNode)) {
                    thisNode = getEndNode(thisNode);
                }

                float lnProb = 0f;        // negative natural log
                String output = tok.getString();

                if (output == null || output.equals(",")) {

                    // these are epsilon (meaning backoff) transitions

                    if (output != null && output.equals(",")) {
                        tok.getString(); // skip the word
                        lnProb = tok.getFloat("probability");
                    }

                    // if the destination node has been expanded
                    // we actually want to add the backoff transition
                    // to the endNode

                    if (hasEndNode(nextNode)) {
                        nextNode = getEndNode(nextNode);
                    }

                } else {
                    String word = tok.getString();     // skip words
                    lnProb = tok.getFloat("probability");

                    if (ignoreUnknownTransitions && word.equals("<unknown>")) {
                        continue;
                    }
                    /*
                    * System.out.println(nextNode + ": " + output);
                    */
                    assert hasWord(nextNode);
                }

                thisNode.add(nextNode, convertProbability(lnProb));

            } else if (token.equals("F")) {
                int thisID = tok.getInt("this id");
                float lnProb = tok.getFloat("probability");

                GrammarNode thisNode = get(thisID);
                GrammarNode nextNode = finalNode;

                if (hasEndNode(thisNode)) {
                    thisNode = getEndNode(thisNode);
                }

                thisNode.add(nextNode, convertProbability(lnProb));
            }
        }
        tok.close();

        assert initialNode != null;

        return initialNode;
    }


    /**
     * Reads the FST file in the given path, and creates the nodes in the FST file.
     *
     * @param path the path of the FST file to read
     * @return the highest ID of all nodes
     * @throws java.io.IOException
     */
    private int createNodes(String path) throws IOException {
        ExtendedStreamTokenizer tok = new ExtendedStreamTokenizer(path, true);
        int maxNodeId = 0;
        while (!tok.isEOF()) {
            tok.skipwhite();
            String token = tok.getString();
            if (token == null) {
                break;
            } else if (token.equals("T")) {
                tok.getInt("src id"); // toss source node
                int id = tok.getInt("dest id"); // dest node numb
                if (id > maxNodeId) {
                    maxNodeId = id;
                }
                String word1 = tok.getString(); // get word
                if (word1 == null) {
                    continue;
                }
                String word2 = tok.getString(); // get word
                tok.getString(); // toss probability
                String nodeName = "G" + id;
                GrammarNode node = nodes.get(nodeName);
                if (node == null) {
                    if (word2.equals(",")) {
                        node = createGrammarNode(id, false);
                    } else {
                        node = createGrammarNode(id, word2.toLowerCase());
                    }
                    nodes.put(nodeName, node);
                } else {
                    if (!word2.equals(",")) {
                        /*
                         * if (!word2.toLowerCase().equals(getWord(node))) {
                         * System.out.println(node + ": " + word2 + ' ' + getWord(node)); }
                         */
                        assert (word2.toLowerCase().equals(getWord(node)));
                    }
                }
            }
        }
        tok.close();
        return maxNodeId;
    }


    /**
     * Expand each of the word nodes into a pair of nodes, as well as adding an optional silence node between the
     * grammar node and its end node.
     *
     * @param maxNodeID the node ID to start with for the new nodes
     * @return the last (or maximum) node ID
     */
    private int expandWordNodes(int maxNodeID) {
        Collection<GrammarNode> allNodes = nodes.values();
        String[][] silence = {{Dictionary.SILENCE_SPELLING}};
        for (GrammarNode node :allNodes) {
            // if it has at least one word, then expand the node
            if (node.getNumAlternatives() > 0) {
                GrammarNode endNode = createGrammarNode(++maxNodeID, false);
                node.add(endNode, LogMath.LOG_ONE);
                // add an optional silence
                if (addOptionalSilence) {
                    GrammarNode silenceNode = createGrammarNode(++maxNodeID,
                            silence);
                    node.add(silenceNode, LogMath.LOG_ONE);
                    silenceNode.add(endNode, LogMath.LOG_ONE);
                }
                expandedNodes.add(node);
            }
        }
        return maxNodeID;
    }


    /**
     * Converts the probability from -ln to logmath
     *
     * @param lnProb the probability to convert. Probabilities in the arpa format in negative natural log format. We
     *               convert them to logmath.
     * @return the converted probability in logMath log base
     */
    private float convertProbability(float lnProb) {
        return logMath.lnToLog(-lnProb);
    }


    /**
     * Given an id returns the associated grammar node
     *
     * @param id the id of interest
     * @return the grammar node or null if none could be found with the proper id
     */
    private GrammarNode get(int id) {
        String name = "G" + id;
        GrammarNode grammarNode = nodes.get(name);
        if (grammarNode == null) {
            grammarNode = createGrammarNode(id, false);
            nodes.put(name, grammarNode);
        }
        return grammarNode;
    }


    /**
     * Determines if the node has a word
     *
     * @param node the grammar node of interest
     * @return true if the node has a word
     */
    private boolean hasWord(GrammarNode node) {
        return (node.getNumAlternatives() > 0);
    }


    /**
     * Gets the word from the given grammar ndoe
     *
     * @param node the node of interest
     * @return the word (or null if the node has no word)
     */
    private String getWord(GrammarNode node) {
        String word = null;
        if (node.getNumAlternatives() > 0) {
            Word[][] alternatives = node.getAlternatives();
            word = alternatives[0][0].getSpelling();
        }
        return word;
    }


    /**
     * Determines if the given node has an end node associated with it.
     *
     * @param node the node of interest
     * @return <code>true</code> if the given node has an end node.
     */
    private boolean hasEndNode(GrammarNode node) {
        return (expandedNodes.contains(node));
    }


    /**
     * Retrieves the end node associated with the given node
     *
     * @param node the node of interest
     * @return the ending node or null if no end node is available
     */
    private GrammarNode getEndNode(GrammarNode node) {
        GrammarArc[] arcs = node.getSuccessors();
        assert arcs != null && arcs.length > 0;
        return arcs[0].getGrammarNode();
    }
}

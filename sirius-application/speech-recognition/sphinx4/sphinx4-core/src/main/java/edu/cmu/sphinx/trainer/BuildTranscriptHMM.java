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

package edu.cmu.sphinx.trainer;

import edu.cmu.sphinx.linguist.acoustic.AcousticModel;
import edu.cmu.sphinx.linguist.acoustic.HMM;
import edu.cmu.sphinx.linguist.acoustic.HMMPosition;
import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.SenoneHMM;
import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.linguist.dictionary.Pronunciation;
import edu.cmu.sphinx.util.LogMath;

/** This class builds an HMM from a transcript, at increasing levels of details. */
public class BuildTranscriptHMM {

    private Graph wordGraph;
    private Graph phonemeGraph;
    private Graph contextDependentPhoneGraph;
    private Graph hmmGraph;
    private TrainerDictionary dictionary;
    private AcousticModel acousticModel;
    private UnitManager unitManager;


    /**
     * Constructor for class BuildTranscriptHMM. When called, this method creates graphs for the transcript at several
     * levels of detail, subsequently mapping from a word graph to a phone graph, to a state graph.
     *
     * @param context       this object's context
     * @param transcript    the transcript to be converted to HMM
     * @param acousticModel the acoustic model to be used
     */
    public BuildTranscriptHMM(String context, Transcript transcript,
                              AcousticModel acousticModel, UnitManager unitManager) {

        this.acousticModel = acousticModel;
        this.unitManager = unitManager;
        wordGraph = buildWordGraph(transcript);
        assert wordGraph.validate() : "Word graph not validated";
        phonemeGraph = buildPhonemeGraph(wordGraph);
        assert phonemeGraph.validate() : "Phone graph not validated";
        contextDependentPhoneGraph =
                buildContextDependentPhonemeGraph(phonemeGraph);
        assert contextDependentPhoneGraph.validate() :
                "Context dependent graph not validated";
        hmmGraph = buildHMMGraph(contextDependentPhoneGraph);
        assert hmmGraph.validate() : "HMM graph not validated";
        // hmmGraph.printGraph();
    }


    /**
     * Returns the graph.
     *
     * @return the graph.
     */
    public Graph getGraph() {
        return hmmGraph;
    }


    /*
    * Build a word graph from this transcript
    */
    private Graph buildWordGraph(Transcript transcript) {
        Graph graph;
        Dictionary transcriptDict = transcript.getDictionary();
        // Make sure the dictionary is a TrainerDictionary before we cast
        assert
                transcriptDict.getClass().getName().endsWith("TrainerDictionary");
        dictionary = (TrainerDictionary) transcriptDict;

        transcript.startWordIterator();
        /* Shouldn't node and edge be part of the graph class? */

        /* The wordgraph must always begin with the <s> */
        graph = new Graph();
        Node initialNode = new Node(NodeType.UTTERANCE_BEGIN);
        graph.addNode(initialNode);
        graph.setInitialNode(initialNode);

        if (transcript.isExact()) {
            Node prevNode = initialNode;
            for (transcript.startWordIterator();
                 transcript.hasMoreWords();) {
                /* create a new node for the next word */
                Node wordNode = new Node(NodeType.WORD,
                        transcript.nextWord());
                /* Link the new node into the graph */
                graph.linkNodes(prevNode, wordNode);

                prevNode = wordNode;
            }
            /* All words are done. Just add the </s> */
            Node wordNode = new Node(NodeType.UTTERANCE_END);
            graph.linkNodes(prevNode, wordNode);
            graph.setFinalNode(wordNode);
        } else {
            /* Begin the utterance with a loopy silence */
            Node silLoopBack =
                    new Node(NodeType.SILENCE_WITH_LOOPBACK);
            graph.linkNodes(initialNode, silLoopBack);

            // Create links with words from the transcript
            for (transcript.startWordIterator();
                 transcript.hasMoreWords();) {
                String word = transcript.nextWord();
                Pronunciation[] pronunciations =
                        dictionary.getWord(word).getPronunciations(null);
                int numberOfPronunciations = pronunciations.length;

                Node[] pronNode = new Node[numberOfPronunciations];

                // Create node at the beginning of the word
                Node dummyWordBeginNode = new Node(NodeType.DUMMY);
                // Allow the silence to be skipped
                // TODO: don't link this, for debugging.
                // graph.linkNodes(prevNode, dummyWordBeginNode);
                // Link the latest silence to the dummy too
                graph.linkNodes(silLoopBack, dummyWordBeginNode);
                // Add word ending dummy node
                Node dummyWordEndNode = new Node(NodeType.DUMMY);
                for (int i = 0; i < numberOfPronunciations; i++) {
                    String wordAlternate
                            = pronunciations[i].getWord().getSpelling();
                    if (i > 0) {
                        wordAlternate += "(" + i + ')';
                    }
                    pronNode[i] = new Node(NodeType.WORD, wordAlternate);
                    graph.linkNodes(dummyWordBeginNode, pronNode[i]);
                    graph.linkNodes(pronNode[i], dummyWordEndNode);
                }

                /* Add silence */
                silLoopBack = new
                        Node(NodeType.SILENCE_WITH_LOOPBACK);
                graph.linkNodes(dummyWordEndNode, silLoopBack);

            }
            Node wordNode = new Node(NodeType.UTTERANCE_END);
            // Link previous node, a dummy word end node
            // TODO: disable this link for now.
            // graph.linkNodes(prevNode, wordNode);
            // Link also the previous silence node
            graph.linkNodes(silLoopBack, wordNode);
            graph.setFinalNode(wordNode);
        }
        return graph;
    }


    /** Convert word graph to phoneme graph */
    private Graph buildPhonemeGraph(Graph wordGraph) {
        Graph phonemeGraph = new Graph();
        phonemeGraph.copyGraph(wordGraph);

        for (Node node : phonemeGraph.nodeToArray()) {
            if (node.getType().equals(NodeType.WORD)) {
                String word = node.getID();
                // "false" means graph won't have additional dummy
                // nodes surrounding the word
                Graph pronunciationGraph = dictionary.getWordGraph(word, false);
                phonemeGraph.insertGraph(pronunciationGraph, node);
            }
        }
        return phonemeGraph;
    }


    /**
     * Convert phoneme graph to a context sensitive phoneme graph. This graph expands paths out to have separate phoneme
     * nodes for phonemes in different contexts.
     *
     * @param phonemeGraph the phoneme graph
     * @return a context dependendent phoneme graph
     */
    public Graph buildContextDependentPhonemeGraph(Graph phonemeGraph) {
        // TODO: Dummy stub for now - return a copy of the original graph
        Graph cdGraph = new Graph();
        cdGraph.copyGraph(phonemeGraph);
        return cdGraph;
    }


    /**
     * Convert the phoneme graph to an HMM.
     *
     * @param cdGraph a context dependent phoneme graph
     * @return an HMM graph for a context dependent phoneme graph
     */
    public Graph buildHMMGraph(Graph cdGraph) {
        Graph hmmGraph = new Graph();

        hmmGraph.copyGraph(cdGraph);

        for (Node node : hmmGraph.nodeToArray()) {
        	Unit unit = null;
            if (node.getType().equals(NodeType.PHONE)) {
                unit = unitManager.getUnit(node.getID());
            } else if (node.getType().equals(NodeType.SILENCE_WITH_LOOPBACK)) {
            	unit = unitManager.getUnit("SIL");
            } else {
                // if it's not a phone, and it's not silence, it's a
                // dummy node, and we don't care.
                continue;
            }
            HMM hmm =
                acousticModel.lookupNearestHMM(unit, HMMPosition.UNDEFINED, false);
            Graph modelGraph = buildModelGraph((SenoneHMM)hmm);
            modelGraph.validate();
            hmmGraph.insertGraph(modelGraph, node);
        }
        return hmmGraph;
    }


    /**
     * Build a graph given an HMM. The graph will not be surrounded by dummy nodes. The number of nodes in the graph is
     * the number of emitting states in the hmm plus one, to account for a final, non-emitting state.
     *
     * @param hmm the HMM
     * @return the graph
     */
    private Graph buildModelGraph(SenoneHMM hmm) {
        Graph graph = new Graph();
        Node prevNode;
        Node stateNode = null;
        float[][] tmat = hmm.getTransitionMatrix();

        prevNode = new Node(NodeType.DUMMY);
        graph.addNode(prevNode);
        graph.setInitialNode(prevNode);

        // 'hmm.getOrder() + 1' to account for final, non-emitting state.
        for (int i = 0; i < hmm.getOrder() + 1; i++) {
            /* create a new node for the next hmmState */
            stateNode = new Node(NodeType.STATE, hmm.getUnit().getName());
            stateNode.setObject(hmm.getState(i));
            graph.addNode(stateNode);
            /* Link the new node into the graph */
            if (i == 0) {
                graph.linkNodes(prevNode, stateNode);
            }
            for (int j = 0; j <= i; j++) {
                // System.out.println("TMAT: " + j + " " + i + " " +
                // tmat[j][i]);
                if (tmat[j][i] != LogMath.LOG_ZERO) {
                    // 'j + 1' to account for the initial dummy node
                    graph.linkNodes(graph.getNode(j + 1), stateNode);
                }
            }
            prevNode = stateNode;
        }
        /* All words are done. Just add the final dummy */
        // stateNode = new Node(NodeType.DUMMY);
        // graph.linkNodes(prevNode, stateNode);
        graph.setFinalNode(stateNode);

        return graph;
    }
}

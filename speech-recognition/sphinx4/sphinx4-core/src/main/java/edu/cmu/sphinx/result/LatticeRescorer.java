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
package edu.cmu.sphinx.result;

import java.util.LinkedList;
import java.util.List;

import edu.cmu.sphinx.linguist.WordSequence;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.linguist.language.ngram.LanguageModel;
import edu.cmu.sphinx.util.LogMath;

/**
 * Class to rescore the lattice with the new Language model.
 */

public class LatticeRescorer {

    protected final Lattice lattice;
    protected final LanguageModel model;
    private int depth;
    private float languageWeigth = 8.0f;

    /**
     * Create a new Lattice optimizer
     * 
     * @param lattice
     */
    public LatticeRescorer(Lattice lattice, LanguageModel model) {
        this.lattice = lattice;
        this.model = model;
        depth = model.getMaxDepth();
    }


    private void rescoreEdges() {
        for (Edge edge : lattice.edges) {

            if (lattice.isFillerNode(edge.getToNode()))
                continue;

            List<String> paths = allPathsTo("", edge, depth);

            float minProb = LogMath.LOG_ZERO;
            for (String path : paths) {
                List<Word> wordList = new LinkedList<Word>();
                for (String pathWord : path.split(" ")) {
                    wordList.add(new Word(pathWord, null, false));
                }
                wordList.add(edge.getToNode().getWord());

                WordSequence seq = new WordSequence(wordList);
                float prob = model.getProbability(seq) * languageWeigth;
                if (minProb < prob)
                    minProb = prob;
            }
            edge.setLMScore(minProb);
        }
    }

    protected List<String> allPathsTo(String path, Edge edge, int currentDepth) {
        List<String> l = new LinkedList<String>();
        String p = edge.getFromNode().getWord().toString() + ' ' + path;

        if (currentDepth == 2
                || edge.getFromNode().equals(lattice.getInitialNode())) {
            l.add(p);
        } else {
            for (Edge e : edge.getFromNode().getEnteringEdges()) {
                l.addAll(allPathsTo(p, e, currentDepth - 1));
            }
        }
        return l;
    }

    public void rescore() {

        lattice.removeFillers();

        rescoreEdges();
    }
}

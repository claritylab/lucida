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

import edu.cmu.sphinx.linguist.WordSequence;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.linguist.language.ngram.LanguageModel;
import edu.cmu.sphinx.util.TimerPool;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * Defines a simple grammar based upon a language model. It generates one {@link GrammarNode grammar node}per word. This
 * grammar can deal with unigram and bigram grammars of up to 1000 or so words. Note that all probabilities are in the
 * log math domain.
 */
public class LMGrammar extends Grammar {

    /** The property for the language model to be used by this grammar */
    @S4Component(type = LanguageModel.class)
    public final static String PROP_LANGUAGE_MODEL = "languageModel";
    // ------------------------
    // Configuration data
    // ------------------------
    private LanguageModel languageModel;

    public LMGrammar(LanguageModel languageModel, boolean showGrammar, boolean optimizeGrammar, boolean addSilenceWords, boolean addFillerWords, Dictionary dictionary) {
        super(showGrammar,optimizeGrammar,addSilenceWords,addFillerWords,dictionary);
        this.languageModel = languageModel;
    }

    public LMGrammar() {

    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        languageModel = (LanguageModel) ps.getComponent(PROP_LANGUAGE_MODEL);
    }


    /**
     * Creates the grammar from the language model. This Grammar contains one word per grammar node. Each word (and
     * grammar node) is connected to all other words with the given probability
     *
     * @return the initial grammar node
     */
    @Override
    protected GrammarNode createGrammar() throws IOException {
        languageModel.allocate();
        TimerPool.getTimer(this,"LMGrammar.create").start();
        GrammarNode firstNode = null;
        if (languageModel.getMaxDepth() > 2) {
            System.out.println("Warning: LMGrammar  limited to bigrams");
        }
        List<GrammarNode> nodes = new ArrayList<GrammarNode>();
        Set<String> words = languageModel.getVocabulary();
        // create all of the word nodes
        for (String word : words) {
            GrammarNode node = createGrammarNode(word);
            if (node != null && !node.isEmpty()) {
                if (node.getWord().equals(
                        getDictionary().getSentenceStartWord())) {
                    firstNode = node;
                } else if (node.getWord().equals(
                        getDictionary().getSentenceEndWord())) {
                    node.setFinalNode(true);
                }
                nodes.add(node);
            }
        }
        if (firstNode == null) {
            throw new Error("No sentence start found in language model");
        }
        for (GrammarNode prevNode : nodes) {
            // don't add any branches out of the final node
            if (prevNode.isFinalNode()) {
                continue;
            }
            for (GrammarNode nextNode : nodes) {
                String prevWord = prevNode.getWord().getSpelling();
                String nextWord = nextNode.getWord().getSpelling();
                Word[] wordArray = {getDictionary().getWord(prevWord),
                        getDictionary().getWord(nextWord)};
                float logProbability = languageModel
                        .getProbability((new WordSequence(wordArray)));
                prevNode.add(nextNode, logProbability);
            }
        }
        TimerPool.getTimer(this,"LMGrammar.create").stop();
        languageModel.deallocate();
        return firstNode;
    }
}

/*
 * Copyright 2009 PC-NG Inc.  
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.linguist.language.ngram;

import edu.cmu.sphinx.linguist.WordSequence;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

import java.io.IOException;
import java.util.*;

/* A model that optimizes the search by giving a preference to 
 * the list of keywords.
 */
public class KeywordOptimizerModel implements LanguageModel {

    /** The property that defines the parent language model. */
    @S4Component(type = LanguageModel.class)
    public final static String PROP_PARENT = "parent";
    
    public HashMap<String, Float> keywordProbs;
    
    // ----------------------------
    // Configuration data
    // ----------------------------
    private LanguageModel parent;

    public KeywordOptimizerModel( LanguageModel parent ) {
        this.parent = parent;
    }

    public KeywordOptimizerModel() {
        
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        parent = (LanguageModel) ps.getComponent(PROP_PARENT);
    }

    /*
     * (non-Javadoc)
     *
     * @see edu.cmu.sphinx.linguist.language.ngram.LanguageModel#allocate()
     */
     public void allocate() throws IOException {
     	parent.allocate();
     }


     /*
     * (non-Javadoc)
     *
     * @see edu.cmu.sphinx.linguist.language.ngram.LanguageModel#deallocate()
     */
     public void deallocate() throws IOException {
    	 parent.deallocate();
     }

    /** Called before a recognition */
    public void start() {
    	parent.start();
    }


    /** Called after a recognition */
    public void stop() {
    	parent.stop();
    }


    /**
     * Gets the ngram probability of the word sequence represented by the word list
     *
     * @param wordSequence the word sequence
     * @return the probability of the word sequence. Probability is in logMath log base
     */
    public float getProbability(WordSequence wordSequence) {
    	float prob = parent.getProbability(wordSequence);
    	
    	if (keywordProbs == null)
    			return prob;
    	
    	for (Word word : wordSequence.getWords()) {
    		String ws = word.toString();
    		if (keywordProbs.containsKey(ws)) {
    			prob *= keywordProbs.get(ws);
    		}
    	}
    	
    	return prob;
    }


    /**
     * Gets the smear term for the given wordSequence
     *
     * @param wordSequence the word sequence
     * @return the smear term associated with this word sequence
     */
    public float getSmear(WordSequence wordSequence) {
        return parent.getSmear(wordSequence);
    }

    /**
     * Returns the maximum depth of the language model
     *
     * @return the maximum depth of the language model
     */
    public int getMaxDepth() {
        return parent.getMaxDepth();
    }


    /**
     * Returns the set of words in the language model. The set is unmodifiable.
     *
     * @return the unmodifiable set of words
     */
    public Set<String> getVocabulary() {
        return parent.getVocabulary();
    }
}

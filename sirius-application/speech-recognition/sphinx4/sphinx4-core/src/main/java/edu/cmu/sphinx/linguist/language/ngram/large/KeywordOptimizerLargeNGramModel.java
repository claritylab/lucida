package edu.cmu.sphinx.linguist.language.ngram.large;

import edu.cmu.sphinx.linguist.language.ngram.large.LargeNGramModel;
import edu.cmu.sphinx.linguist.WordSequence;
import edu.cmu.sphinx.linguist.dictionary.Word;

import java.util.*;

/**
 * Use a largeNGramModel that also can be adjusted depending on context.
 * A model that optimizes the search by giving a preference to 
 * the list of keywords.
 * <p>
 * Example of use in calling program using config file:
 * <br>
 * <code>
 *         KeywordOptimizerLargeNGramModel model = 
 *      	(KeywordOptimizerLargeNGramModel) cm.lookup("trigramModel");
 *         model.keywordProbs = this.hashProbs;
 *          </code> <p>
 * Create hashProbs by loading keywords and changes to probabilities,
 *    string, float pairs. Keywords may be all lower case.<br>
 *    <code>
 *    hashProbs.put("keyword", 0.5f);  </code>
 * @author daktari3
 * @version 2010-12-16
 * @see LargeNGramModel
 * @see edu.cmu.sphinx.linguist.language.ngram.KeywordOptimizerModel
 *
 */

public class KeywordOptimizerLargeNGramModel extends LargeNGramModel {
	
	   /**
	    * hash map of probability adjustments settable by
	    * user program.
	    * 
	    */
	   public HashMap<String, Float> keywordProbs;

    /**
     * Gets the ngram probability of the word sequence represented by the word list
     *
     * @param wordSequence the word sequence
     * @return the probability of the word sequence. Probability is in logMath log base
     */
    @Override
    public float getProbability(WordSequence wordSequence) {
    	float prob = super.getProbability(wordSequence);
    	
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

}

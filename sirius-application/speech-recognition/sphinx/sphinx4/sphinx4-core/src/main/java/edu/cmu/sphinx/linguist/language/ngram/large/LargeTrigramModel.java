/*
 * Copyright 2010 LIUM, based on Carnegie Mellon University previous work.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * Portions Copyright 2010 LIUM, University of Le Mans, France
  -> Yannick Esteve, Anthony Rousseau
 
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.linguist.language.ngram.large;

import java.net.URL;

import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.linguist.language.ngram.LanguageModel;
import edu.cmu.sphinx.util.props.*;

/**
 * A wrapper for LargeNGramModel base on the old LargeTrigramModel class. 
 * 
 * @author Anthony Rousseau, LIUM
 */
public class LargeTrigramModel extends LargeNGramModel {

    /** The property that defines that maximum number of trigrams to be cached */
    @S4Integer(defaultValue = 100000)
    public static final String PROP_TRIGRAM_CACHE_SIZE = "trigramCacheSize";

    /**
     * @param format
     * @param urlLocation
     * @param ngramLogFile
     * @param maxTrigramCacheSize
     * @param maxBigramCacheSize
     * @param clearCacheAfterUtterance
     * @param maxDepth
     * @param dictionary
     * @param applyLanguageWeightAndWip
     * @param languageWeight
     * @param wip
     * @param unigramWeight
     * @param fullSmear
     */
    public LargeTrigramModel(String format, URL urlLocation,
            String ngramLogFile, int maxTrigramCacheSize,
            int maxBigramCacheSize, boolean clearCacheAfterUtterance,
            int maxDepth, Dictionary dictionary,
            boolean applyLanguageWeightAndWip, float languageWeight,
            double wip, float unigramWeight, boolean fullSmear) {
        // Inline conditional statement to prevent maxDepth being > to 3
        // We are in a Trigram wrapper, after all
        super(format, urlLocation, ngramLogFile, maxTrigramCacheSize,
                clearCacheAfterUtterance, (maxDepth > 3 ? 3
                        : maxDepth), dictionary,
                applyLanguageWeightAndWip, languageWeight, wip, unigramWeight,
                fullSmear);
    }

    /**
	 * 
	 */
    public LargeTrigramModel() {
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util
     * .props.PropertySheet)
     */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();
        location = ConfigurationManagerUtils.getResource(PROP_LOCATION, ps);
        ngramLogFile = ps.getString(PROP_QUERY_LOG_FILE);
        clearCacheAfterUtterance = ps
                .getBoolean(PROP_CLEAR_CACHES_AFTER_UTTERANCE);
        maxDepth = ps.getInt(LanguageModel.PROP_MAX_DEPTH);
        ngramCacheSize = ps.getInt(PROP_TRIGRAM_CACHE_SIZE);
        dictionary = (Dictionary) ps.getComponent(PROP_DICTIONARY);
        applyLanguageWeightAndWip = ps
                .getBoolean(PROP_APPLY_LANGUAGE_WEIGHT_AND_WIP);
        languageWeight = ps.getFloat(PROP_LANGUAGE_WEIGHT);
        wip = ps.getDouble(PROP_WORD_INSERTION_PROBABILITY);
        unigramWeight = ps.getFloat(PROP_UNIGRAM_WEIGHT);
        fullSmear = ps.getBoolean(PROP_FULL_SMEAR);
    }
}

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
package edu.cmu.sphinx.linguist.dictionary;

import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.S4Boolean;
import edu.cmu.sphinx.util.props.S4Component;
import edu.cmu.sphinx.util.props.S4Integer;
import edu.cmu.sphinx.util.props.S4String;

import java.io.IOException;

/**
 * Provides a generic interface to a dictionary. The dictionary is responsible for determining how a word is
 * pronounced.
 */
public interface Dictionary extends Configurable {

    /** Spelling of the sentence start word. */
    public static final String SENTENCE_START_SPELLING = "<s>";
    /** Spelling of the sentence end word. */
    public static final String SENTENCE_END_SPELLING = "</s>";
    /** Spelling of the 'word' that marks a silence */
    public static final String SILENCE_SPELLING = "<sil>";

    /** The property for the dictionary file path. */
    @S4String
    public static final String PROP_DICTIONARY = "dictionaryPath";

    /** The property for the g2p model file path. */
    @S4String(defaultValue = "")
    public static final String PROP_G2P_MODEL_PATH = "g2pModelPath";

    /** The property for the g2p model file path. */
    @S4Integer(defaultValue = 1)
    public static final String PROP_G2P_MAX_PRONUNCIATIONS = "g2pMaxPron";

    /** The property for the filler dictionary file path. */
    @S4String
    public static final String PROP_FILLER_DICTIONARY = "fillerPath";

    /** The property that specifies whether to add a duplicate SIL-ending pronunciation. */
    @S4Boolean(defaultValue = false)
    public static final String PROP_ADD_SIL_ENDING_PRONUNCIATION = "addSilEndingPronunciation";

    /**
     * The property that specifies the word to substitute when a lookup fails to find the word in the
     * dictionary. If this is not set, no substitute is performed.
     */
    @S4String(mandatory = false)
    public static final String PROP_WORD_REPLACEMENT = "wordReplacement";

    /**
     * The property that specifies whether the dictionary should return null if a word is not found in
     * the dictionary, or whether it should throw an error. If true, a null is returned for words that are not found in
     * the dictionary (and the 'PROP_WORD_REPLACEMENT' property is not set).
     */
    @S4Boolean(defaultValue = false)
    public static final String PROP_ALLOW_MISSING_WORDS = "allowMissingWords";

    /**
     * The property that specifies whether the Dictionary.getWord() method should return a Word object even if the
     * word does not exist in the dictionary. If this property is true, and property allowMissingWords is also true, the
     * method will return a Word, but the Word will have null Pronunciations. Otherwise, the method will return null.
     * This property is usually only used for testing purposes.
     */
    @S4Boolean(defaultValue = false)
    public static final String PROP_CREATE_MISSING_WORDS = "createMissingWords";

    /** The property that defines the name of the unit manager that is used to convert strings to Unit objects */
    @S4Component(type = UnitManager.class, defaultClass = UnitManager.class)
    public static final String PROP_UNIT_MANAGER = "unitManager";

    /**
     * The property for the custom dictionary file paths. This addenda property points to a possibly
     * empty list of URLs to dictionary addenda.  Each addendum should contain word pronunciations in the same Sphinx-3
     * dictionary format as the main dictionary.  Words in the addendum are added after the words in the main dictionary
     * and will override previously specified pronunciations.  If you wish to extend the set of pronunciations for a
     * particular word, add a new pronunciation by number.  For example, in the following addendum, given that the
     * aforementioned main dictionary is specified, the pronunciation for 'EIGHT' will be overridden by the addenda,
     * while the pronunciation for 'SIX' and 'ZERO' will be augmented and a new pronunciation for 'ELEVEN' will be
     * added.
     * <pre>
     *          EIGHT   OW T
     *          SIX(2)  Z IH K S
     *          ZERO(3)  Z IY Rl AH
     *          ELEVEN   EH L EH V AH N
     * </pre>
     */
    @S4String(mandatory = false)
    public static final String PROP_ADDENDA = "addenda";

    /**
     * Returns a Word object based on the spelling and its classification. The behavior of this method is also affected
     * by the properties wordReplacement, allowMissingWords, and createMissingWords.
     *
     * @param text the spelling of the word of interest.
     * @return a Word object
     * @see edu.cmu.sphinx.linguist.dictionary.Pronunciation
     */
    public Word getWord(String text);


    /**
     * Returns the sentence start word.
     *
     * @return the sentence start word
     */
    public Word getSentenceStartWord();


    /**
     * Returns the sentence end word.
     *
     * @return the sentence end word
     */
    public Word getSentenceEndWord();


    /**
     * Returns the silence word.
     *
     * @return the silence word
     */
    public Word getSilenceWord();


    /**
     * Returns the set of all possible word classifications for this dictionary.
     *
     * @return the set of all possible word classifications
     */
    public WordClassification[] getPossibleWordClassifications();


    /**
     * Gets the set of all filler words in the dictionary
     *
     * @return an array (possibly empty) of all filler words
     */
    public Word[] getFillerWords();


    /**
     * Allocates the dictionary
     *
     * @throws IOException if there is trouble loading the dictionary
     */
    public void allocate() throws IOException;


    /** Deallocates the dictionary */
    public void deallocate();
}

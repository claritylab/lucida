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

import edu.cmu.sphinx.linguist.acoustic.Context;
import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.linguist.g2p.G2PConverter;
import edu.cmu.sphinx.linguist.g2p.Path;
import edu.cmu.sphinx.util.ExtendedStreamTokenizer;
import edu.cmu.sphinx.util.Timer;
import edu.cmu.sphinx.util.TimerPool;
import edu.cmu.sphinx.util.props.ConfigurationManagerUtils;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.*;
import java.util.logging.Logger;

/**
 * Creates a dictionary by reading in an ASCII-based Sphinx-3 format dictionary. Each line of the dictionary specifies
 * the word, followed by spaces or tab, followed by the pronunciation (by way of the list of phones) of the word. Each
 * word can have more than one pronunciations. For example, a digits dictionary will look like:
 * <p/>
 * <pre>
 * ONE                  HH W AH N
 * ONE(2)               W AH N
 * TWO                  T UW
 * THREE                TH R IY
 * FOUR                 F AO R
 * FIVE                 F AY V
 * SIX                  S IH K S
 * SEVEN                S EH V AH N
 * EIGHT                EY T
 * NINE                 N AY N
 * ZERO                 Z IH R OW
 * ZERO(2)              Z IY R OW
 * OH                   OW
 * </pre>
 * <p/>
 * In the above example, the words "one" and "zero" have two pronunciations each.
 * <p/>
 * This dictionary will read in all the words and its pronunciation(s) at startup. Therefore, if the dictionary is big,
 * it will take longer to load and will consume more memory.
 */

public class FullDictionary implements Dictionary {

    // ----------------------------------
    // configuration variables
    // ----------------------------------
    private Logger logger;
    protected boolean addSilEndingPronunciation;
    private boolean allowMissingWords;
    private boolean createMissingWords;
    private String wordReplacement;
    private URL wordDictionaryFile;
    private URL fillerDictionaryFile;
    private boolean allocated;
    private UnitManager unitManager;
    protected URL g2pModelFile;
    protected int g2pMaxPron = 0;
    protected List<URL> addendaUrlList;


    private Map<String, Word> wordDictionary;
    private Map<String, Word> fillerDictionary;
    private Timer loadTimer;
    protected G2PConverter g2pDecoder;

        public FullDictionary(
            URL wordDictionaryFile,
            URL fillerDictionaryFile,
            List<URL> addendaUrlList,
            boolean addSilEndingPronunciation,
            String wordReplacement,
            boolean allowMissingWords,
            boolean createMissingWords,
            UnitManager unitManager
    ) {
        this.logger = Logger.getLogger(getClass().getName());

        this.wordDictionaryFile = wordDictionaryFile;
        this.fillerDictionaryFile = fillerDictionaryFile;
        this.addendaUrlList = addendaUrlList;
        this.addSilEndingPronunciation = addSilEndingPronunciation;
        this.wordReplacement = wordReplacement;
        this.allowMissingWords = allowMissingWords;
        this.createMissingWords = createMissingWords;
        this.unitManager = unitManager;
    }

        public FullDictionary(
                URL wordDictionaryFile,
                URL fillerDictionaryFile,
                List<URL> addendaUrlList,
                boolean addSilEndingPronunciation,
                String wordReplacement,
                boolean allowMissingWords,
                boolean createMissingWords,
                UnitManager unitManager, 
                URL g2pModelFile,
                int g2pMaxPron
        ) {
            this(
                    wordDictionaryFile, 
                    fillerDictionaryFile, 
                    addendaUrlList,
                    addSilEndingPronunciation,
                    wordReplacement,
                    allowMissingWords,
                    createMissingWords,
                    unitManager);
            this.g2pModelFile = g2pModelFile;
            this.g2pMaxPron = g2pMaxPron;
        }

    public FullDictionary() {

    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();

        wordDictionaryFile = ConfigurationManagerUtils.getResource(PROP_DICTIONARY, ps);
        fillerDictionaryFile = ConfigurationManagerUtils.getResource(PROP_FILLER_DICTIONARY, ps);
        addendaUrlList = ps.getResourceList(PROP_ADDENDA);
        addSilEndingPronunciation = ps.getBoolean(PROP_ADD_SIL_ENDING_PRONUNCIATION);
        wordReplacement = ps.getString(Dictionary.PROP_WORD_REPLACEMENT);
        allowMissingWords = ps.getBoolean(Dictionary.PROP_ALLOW_MISSING_WORDS);
        createMissingWords = ps.getBoolean(PROP_CREATE_MISSING_WORDS);
        unitManager = (UnitManager) ps.getComponent(PROP_UNIT_MANAGER);
        g2pModelFile = ConfigurationManagerUtils.getResource(PROP_G2P_MODEL_PATH, ps);
        g2pMaxPron = ps.getInt(PROP_G2P_MAX_PRONUNCIATIONS);
    }


    /* (non-Javadoc)
    * @see edu.cmu.sphinx.linguist.dictionary.Dictionary#allocate()
    */
    public void allocate() throws IOException {

        if (!allocated) {
            loadTimer = TimerPool.getTimer(this, "Load Dictionary");
            loadTimer.start();
            // NOTE: "location" can be null here, in which case the
            // "wordDictionaryFile" and "fillerDictionaryFile" should
            // contain the full path to the Dictionaries.
            logger.info("Loading dictionary from: " + wordDictionaryFile);
            wordDictionary =
                    loadDictionary(wordDictionaryFile.openStream(), false);
                        
            loadCustomDictionaries(addendaUrlList);

            logger.info("Loading filler dictionary from: " +
                    fillerDictionaryFile);
            fillerDictionary =
                    loadDictionary(fillerDictionaryFile.openStream(), true);

            if(g2pModelFile != null && !g2pModelFile.getPath().equals("")) {
                g2pDecoder = new G2PConverter(g2pModelFile);
            }
            loadTimer.stop();
            logger.finest(dumpToString());
            allocated = true;
        }
    }


    /* (non-Javadoc)
    * @see edu.cmu.sphinx.linguist.dictionary.Dictionary#deallocate()
    */
    public void deallocate() {
        if (allocated) {
            fillerDictionary = null;
            wordDictionary = null;
            g2pDecoder = null;
            loadTimer = null;
            allocated = false;
        }
    }


    /**
     * Loads the given sphinx3 style simple dictionary from the given InputStream. The InputStream is assumed to contain
     * ASCII data.
     *
     * @param inputStream  the InputStream of the dictionary
     * @param isFillerDict true if this is a filler dictionary, false otherwise
     * @throws java.io.IOException if there is an error reading the dictionary
     */
    protected Map<String, Word> loadDictionary(InputStream inputStream, boolean isFillerDict)
            throws IOException {
        Map<String, List<Pronunciation>> dictPronunciations = new HashMap<String, List<Pronunciation>>();
        ExtendedStreamTokenizer est = new ExtendedStreamTokenizer(inputStream,
                true);
        String word;
        while ((word = est.getString()) != null) {
            word = removeParensFromWord(word);
            word = word.toLowerCase();
            List<Unit> units = new ArrayList<Unit>(20);

            List<Pronunciation> pronunciations = dictPronunciations.get(word);
            if (pronunciations == null) {
                pronunciations = new LinkedList<Pronunciation>();
            }
            
            // Count units and add them 
            String unitText;
            while ((unitText = est.getString()) != null) {
                units.add(getCIUnit(unitText, isFillerDict));
            }
            pronunciations.add(new Pronunciation(units));
            // if we are adding a SIL ending duplicate
            if (!isFillerDict && addSilEndingPronunciation) {
                units.add(UnitManager.SILENCE);
                pronunciations.add(new Pronunciation(units));
            }
            
            dictPronunciations.put(word, pronunciations);
        }
        inputStream.close();
        est.close();
        return createWords(dictPronunciations, isFillerDict);
    }


    /**
     * Converts the spelling/Pronunciations mappings in the dictionary into spelling/Word mappings.
     *
     * @param isFillerDict if true this is a filler dictionary
     */
    protected HashMap<String, Word> createWords(Map<String, List<Pronunciation>> pronunciationList, boolean isFillerDict) {
        HashMap <String, Word> result = new HashMap <String, Word>();
        for (Map.Entry<String, List<Pronunciation>> entry : pronunciationList.entrySet()) {
            String spelling = entry.getKey();
            List<Pronunciation> pronunciations = entry.getValue();
            Pronunciation[] pros = new Pronunciation[pronunciations.size()];
            pronunciations.toArray(pros);
            Word word = new Word(spelling, pros, isFillerDict);
            for (Pronunciation pro : pros) {
                pro.setWord(word);
            }
            result.put(spelling, word);
        }
        return result;
    }


    /**
     * Gets a context independent unit. There should only be one instance of any CI unit
     *
     * @param name     the name of the unit
     * @param isFiller if true, the unit is a filler unit
     * @return the unit
     */
    protected Unit getCIUnit(String name, boolean isFiller) {
        return unitManager.getUnit(name, isFiller, Context.EMPTY_CONTEXT);
    }


    /**
     * Returns a new string that is the given word but with the ending parenthesis removed.
     * <p/>
     * Example:
     * <p/>
     * <pre>
     *  "LEAD(2)" returns "LEAD"
     *  "LEAD" returns "LEAD"
     *  the word to be stripped
     * <p/>
     *  @return the given word but with all characters from the first
     *  open parentheses removed
     */
    protected String removeParensFromWord(String word) {
        if (word.charAt(word.length() - 1) == ')') {
            int index = word.lastIndexOf('(');
            if (index > 0) {
                word = word.substring(0, index);
            }
        }
        return word;
    }


    /**
     * Returns a Word object based on the spelling and its classification. The behavior of this method is affected by
     * the properties wordReplacement, allowMissingWords, and createMissingWords.
     *
     * @param text the spelling of the word of interest.
     * @return a Word object
     * @see edu.cmu.sphinx.linguist.dictionary.Word
     */
    public Word getWord(String text) {
        text = text.toLowerCase();
        Word word = lookupWord(text);
        if (word == null) {
            logger.warning("The dictionary is missing a phonetic transcription for the word '" + text + "'");
            if (wordReplacement != null) {
                word = lookupWord(wordReplacement);
                logger.warning("Replacing " + text + " with " +
                        wordReplacement);
                if (word == null) {
                    logger.severe("Replacement word " + wordReplacement
                            + " not found!");
                }
            } else if (allowMissingWords) {
                if (createMissingWords) {
                    if (g2pModelFile != null && !g2pModelFile.getPath().equals("")) {
                        logger.warning("Generating phonetic transcription(s) for the word '" + text + "' using g2p model");
                        ArrayList<Path> paths = g2pDecoder.phoneticize(text, g2pMaxPron);
                        List<Pronunciation> pronunciations = new LinkedList<Pronunciation>();
                        for(Path p : paths) {
                            int unitCount = p.getPath().size();
                            ArrayList<Unit> units = new ArrayList<Unit>(unitCount);
                            for(String token : p.getPath()) {
                                    units.add(getCIUnit(token, false));
                                }
                                pronunciations.add(new Pronunciation(units));
                                if (addSilEndingPronunciation) {
                                    units.add(UnitManager.SILENCE);
                                    pronunciations.add(new Pronunciation(units));
                                }
                            }
                        Pronunciation[] pronunciationsArray = pronunciations.toArray(new Pronunciation[pronunciations.size()]);
                        word = new Word(text, pronunciationsArray, false);
                        for (Pronunciation pronunciation : pronunciationsArray) {
                            pronunciation.setWord(word);
                        }
                        wordDictionary.put(text, word);
                    } else {
                        word = new Word(text, null, false);
                        wordDictionary.put(text, word);
                        return null;
                    }
                } else {
                    return null;
                }
            } else {
                return null;
            }
        }
        return word;
    }


    /**
     * Lookups up a word
     *
     * @param spelling the spelling of the word
     * @return the word or null
     */
    private Word lookupWord(String spelling) {
        Word word = wordDictionary.get(spelling);
        if (word == null) {
            word = fillerDictionary.get(spelling);
        }
        return word;
    }


    /**
     * Returns the sentence start word.
     *
     * @return the sentence start word
     */
    public Word getSentenceStartWord() {
        return getWord(SENTENCE_START_SPELLING);
    }


    /**
     * Returns the sentence end word.
     *
     * @return the sentence end word
     */
    public Word getSentenceEndWord() {
        return getWord(SENTENCE_END_SPELLING);
    }


    /**
     * Returns the silence word.
     *
     * @return the silence word
     */
    public Word getSilenceWord() {
        return getWord(SILENCE_SPELLING);
    }


    /**
     * Returns the set of all possible word classifications for this dictionary.
     *
     * @return the set of all possible word classifications
     */
    public WordClassification[] getPossibleWordClassifications() {
        return null;
    }


    /**
     * Get the word dictionary file
     *
     * @return the URL of the word dictionary file
     */
    public URL getWordDictionaryFile() {
        return wordDictionaryFile;
    }


    /**
     * Get the filler dictionary file
     *
     * @return the URL of the filler dictionary file
     */
    public URL getFillerDictionaryFile() {
        return fillerDictionaryFile;
    }


    /**
     * Returns a string representation of this FullDictionary in alphabetical order.
     *
     * @return a string representation of this FullDictionary
     */
    @Override
    public String toString() {
        return super.toString() + "numWords=" + wordDictionary.size() + " dictLlocation=" + getWordDictionaryFile();
    }


    private String dumpToString() {
        SortedMap<String, Object> sorted = new TreeMap<String, Object>(wordDictionary);
        StringBuilder result = new StringBuilder();
        sorted.putAll(fillerDictionary);
        for (String text : sorted.keySet()) {
            Word word = getWord(text);
            Pronunciation[] pronunciations = word.getPronunciations(null);
            result.append(word).append('\n');
            for (Pronunciation pronunciation : pronunciations) {
                result.append("   ").append(pronunciation).append('\n');
            }
        }
        return result.toString();
    }


    /**
     * Gets the set of all filler words in the dictionary
     *
     * @return an array (possibly empty) of all filler words
     */
    public Word[] getFillerWords() {
        return fillerDictionary.values().toArray(new Word[fillerDictionary.size()]);
    }
    
    /**
     * Loads the dictionary with a list of URLs to custom dictionary resources
     *
     * @param addenda the list of custom dictionary URLs to be loaded
     * @throws IOException if there is an error reading the resource URL
     */
    private void loadCustomDictionaries(List<URL> addenda) throws IOException {
        if (addenda != null) {
            for (URL addendumUrl : addenda) {
                logger.info("Loading addendum dictionary from: " + addendumUrl);
                loadDictionary(addendumUrl.openStream(), false);
            }
        }
    }
}

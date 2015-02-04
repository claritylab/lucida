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
import edu.cmu.sphinx.util.Timer;
import edu.cmu.sphinx.util.TimerPool;
import edu.cmu.sphinx.util.props.ConfigurationManagerUtils;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.*;
import java.util.logging.Logger;

/**
 * Creates a dictionary by quickly reading in an ASCII-based Sphinx-3 format
 * dictionary. It is called the FastDictionary because the loading is fast. When
 * loaded the dictionary just loads each line of the dictionary into the hash
 * table, assuming that most words are not going to be used. Only when a word is
 * actually used is its pronunciations massaged into an array of pronunciations.
 * <p/>
 * The format of the ASCII dictionary that it explains is the same as the
 * {@link FullDictionary FullDictionary}, i.e., the word, followed by spaces or
 * tab, followed by the pronunciation(s). For example, a digits dictionary will
 * look like:
 * <p/>
 * 
 * <pre>
 *  ONE HH W AH N
 *  ONE(2) W AH N
 *  TWO T UW
 *  THREE TH R IY
 *  FOUR F AO R
 *  FIVE F AY V
 *  SIX S IH K S
 *  SEVEN S EH V AH N
 *  EIGHT EY T
 *  NINE N AY N
 *  ZERO Z IH R OW
 *  ZERO(2) Z IY R OW
 *  OH OW
 * </pre>
 * <p/>
 * <p/>
 * In the above example, the words "one" and "zero" have two pronunciations
 * each.
 */

public class FastDictionary implements Dictionary {

    // -------------------------------
    // Configuration data
    // --------------------------------
    protected Logger logger;
    private boolean addSilEndingPronunciation;
    private boolean allowMissingWords;
    private boolean createMissingWords;
    private String wordReplacement;
    protected URL wordDictionaryFile;
    protected URL fillerDictionaryFile;
    protected URL g2pModelFile;
    protected int g2pMaxPron = 0;
    protected List<URL> addendaUrlList;

    protected UnitManager unitManager;

    // -------------------------------
    // working data
    // -------------------------------
    protected Map<String, String> dictionary;
    protected Map<String, Word> wordDictionary;
    protected G2PConverter g2pDecoder;

    protected final static String FILLER_TAG = "-F-";
    protected Set<String> fillerWords;
    protected boolean allocated;

    public FastDictionary(String wordDictionaryFile,
            String fillerDictionaryFile, List<URL> addendaUrlList,
            boolean addSilEndingPronunciation, String wordReplacement,
            boolean allowMissingWords, boolean createMissingWords,
            UnitManager unitManager) throws MalformedURLException,
            ClassNotFoundException {
        this(ConfigurationManagerUtils.resourceToURL(wordDictionaryFile),
             ConfigurationManagerUtils.resourceToURL(fillerDictionaryFile),
             addendaUrlList,
             addSilEndingPronunciation,
             wordReplacement,
             allowMissingWords,
             createMissingWords,
             unitManager);
    }

    public FastDictionary(URL wordDictionaryFile, URL fillerDictionaryFile,
            List<URL> addendaUrlList, boolean addSilEndingPronunciation,
            String wordReplacement, boolean allowMissingWords,
            boolean createMissingWords, UnitManager unitManager) {
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

    public FastDictionary(URL wordDictionaryFile, URL fillerDictionaryFile,
            List<URL> addendaUrlList, boolean addSilEndingPronunciation,
            String wordReplacement, boolean allowMissingWords,
            boolean createMissingWords, UnitManager unitManager,
            URL g2pModelFile, int g2pMaxPron) {
        this(wordDictionaryFile,
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

    public FastDictionary() {

    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util
     * .props.PropertySheet)
     */

    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();

        wordDictionaryFile = ConfigurationManagerUtils
                .getResource(PROP_DICTIONARY, ps);
        fillerDictionaryFile = ConfigurationManagerUtils
                .getResource(PROP_FILLER_DICTIONARY, ps);
        addendaUrlList = ps.getResourceList(PROP_ADDENDA);
        addSilEndingPronunciation = ps
                .getBoolean(PROP_ADD_SIL_ENDING_PRONUNCIATION);
        wordReplacement = ps.getString(Dictionary.PROP_WORD_REPLACEMENT);
        allowMissingWords = ps.getBoolean(Dictionary.PROP_ALLOW_MISSING_WORDS);
        createMissingWords = ps.getBoolean(PROP_CREATE_MISSING_WORDS);
        unitManager = (UnitManager) ps.getComponent(PROP_UNIT_MANAGER);
        g2pModelFile = ConfigurationManagerUtils
                .getResource(PROP_G2P_MODEL_PATH, ps);
        g2pMaxPron = ps.getInt(PROP_G2P_MAX_PRONUNCIATIONS);
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

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.linguist.dictionary.Dictionary#allocate()
     */

    public void allocate() throws IOException {
        if (!allocated) {
            dictionary = new HashMap<String, String>();
            wordDictionary = new HashMap<String, Word>();

            Timer loadTimer = TimerPool.getTimer(this, "Load Dictionary");
            fillerWords = new HashSet<String>();

            loadTimer.start();

            logger.info("Loading dictionary from: " + wordDictionaryFile);

            loadDictionary(wordDictionaryFile.openStream(), false);

            loadCustomDictionaries(addendaUrlList);

            logger.info("Loading filler dictionary from: "
                    + fillerDictionaryFile);

            loadDictionary(fillerDictionaryFile.openStream(), true);

            if (g2pModelFile != null && !g2pModelFile.getPath().equals("")) {
                g2pDecoder = new G2PConverter(g2pModelFile);
            }
            loadTimer.stop();
        }

    }

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.linguist.dictionary.Dictionary#deallocate()
     */

    public void deallocate() {
        if (allocated) {
            dictionary = null;
            g2pDecoder = null;
            allocated = false;
        }
    }

    /**
     * Loads the given sphinx3 style simple dictionary from the given
     * InputStream. The InputStream is assumed to contain ASCII data.
     * 
     * @param inputStream
     *            the InputStream of the dictionary
     * @param isFillerDict
     *            true if this is a filler dictionary, false otherwise
     * @throws java.io.IOException
     *             if there is an error reading the dictionary
     */
    protected void loadDictionary(InputStream inputStream, boolean isFillerDict)
            throws IOException {
        InputStreamReader isr = new InputStreamReader(inputStream);
        BufferedReader br = new BufferedReader(isr);
        String line;

        while ((line = br.readLine()) != null) {
            if (!line.isEmpty()) {
                int spaceIndex = line.indexOf(' ');
                int spaceIndexTab = line.indexOf('\t');
                if (spaceIndex == -1) {
                    // Case where there's no blank character
                    spaceIndex = spaceIndexTab;
                } else if ((spaceIndexTab >= 0) && (spaceIndexTab < spaceIndex)) {
                    // Case where there's a blank and a tab, but the tab
                    // precedes the blank
                    spaceIndex = spaceIndexTab;
                }
                // TODO: throw an exception if spaceIndex == -1 ?
                if (spaceIndex == -1) {
                    throw new Error("Error loading word: " + line);
                }
                String word = line.substring(0, spaceIndex);
                word = word.toLowerCase();
                // Add numeric index if the word is repeating.
                if (dictionary.containsKey(word)) {
                    int index = 2;
                    String wordWithIdx;
                    do {
                        wordWithIdx = String.format("%s(%d)", word, index++);
                    } while (dictionary.containsKey(wordWithIdx));
                    word = wordWithIdx;
                }

                if (isFillerDict) {
                    dictionary.put(word, (FILLER_TAG + line));
                    fillerWords.add(word);
                } else {
                    dictionary.put(word, line);
                }
            }
        }

        br.close();
        isr.close();
        inputStream.close();
    }

    /**
     * Gets a context independent unit. There should only be one instance of any
     * CI unit
     * 
     * @param name
     *            the name of the unit
     * @param isFiller
     *            if true, the unit is a filler unit
     * @return the unit
     */
    protected Unit getCIUnit(String name, boolean isFiller) {
        return unitManager.getUnit(name, isFiller, Context.EMPTY_CONTEXT);
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
     * Returns a Word object based on the spelling and its classification. The
     * behavior of this method is also affected by the properties
     * wordReplacement, allowMissingWords, and createMissingWords.
     * 
     * @param text
     *            the spelling of the word of interest.
     * @return a Word object
     * @see edu.cmu.sphinx.linguist.dictionary.Word
     */
    public Word getWord(String text) {
        text = text.toLowerCase();
        Word wordObject = wordDictionary.get(text);

        if (wordObject != null) {
            return wordObject;
        }

        String word = dictionary.get(text);
        if (word == null) { // deal with 'not found' case
            logger.warning("The dictionary is missing a phonetic transcription for the word '"
                    + text + "'");
            if (wordReplacement != null) {
                wordObject = getWord(wordReplacement);
            } else if (allowMissingWords) {
                if (createMissingWords) {
                    if (g2pModelFile != null
                            && !g2pModelFile.getPath().equals("")) {
                        logger.warning("Generating phonetic transcription(s) for the word '"
                                + text + "' using g2p model");
                        ArrayList<Path> paths = g2pDecoder
                                .phoneticize(text, g2pMaxPron);
                        List<Pronunciation> pronunciations = new LinkedList<Pronunciation>();
                        for (Path p : paths) {
                            int unitCount = p.getPath().size();
                            ArrayList<Unit> units = new ArrayList<Unit>(unitCount);
                            for (String token : p.getPath()) {
                                units.add(getCIUnit(token, false));
                            }
                            pronunciations.add(new Pronunciation(units));
                            if (addSilEndingPronunciation) {
                                units.add(UnitManager.SILENCE);
                                pronunciations.add(new Pronunciation(units));
                            }
                        }
                        Pronunciation[] pronunciationsArray = pronunciations
                                .toArray(new Pronunciation[pronunciations
                                        .size()]);
                        wordObject = createWord(text,
                                                pronunciationsArray,
                                                false);
                        for (Pronunciation pronunciation : pronunciationsArray) {
                            pronunciation.setWord(wordObject);
                        }
                        wordDictionary.put(text, wordObject);
                    } else {
                        wordObject = createWord(text, null, false);
                    }
                }
            }
        } else { // first lookup for this string
            wordObject = processEntry(text);
        }

        return wordObject;
    }

    /**
     * Create a Word object with the given spelling and pronunciations, and
     * insert it into the dictionary.
     * 
     * @param text
     *            the spelling of the word
     * @param pronunciation
     *            the pronunciation of the word
     * @param isFiller
     *            if <code>true</code> this is a filler word
     * @return the word
     */
    private Word createWord(String text,
                            Pronunciation[] pronunciation,
                            boolean isFiller) {
        Word word = new Word(text, pronunciation, isFiller);
        dictionary.put(text, word.toString());
        return word;
    }

    /**
     * Processes a dictionary entry. When loaded the dictionary just loads each
     * line of the dictionary into the hash table, assuming that most words are
     * not going to be used. Only when a word is actually used is its
     * pronunciations massaged into an array of pronunciations.
     */
    private Word processEntry(String word) {
        List<Pronunciation> pronunciations = new LinkedList<Pronunciation>();
        String line;
        int count = 0;
        boolean isFiller = false;

        do {
            count++;
            String lookupWord = word;
            if (count > 1) {
                lookupWord = lookupWord + '(' + count + ')';
            }
            line = dictionary.get(lookupWord);
            if (line != null) {
                StringTokenizer st = new StringTokenizer(line);

                String tag = st.nextToken();
                isFiller = tag.startsWith(FILLER_TAG);
                int unitCount = st.countTokens();

                ArrayList<Unit> units = new ArrayList<Unit>(unitCount);
                for (int i = 0; i < unitCount; i++) {
                    String unitName = st.nextToken();
                    units.add(getCIUnit(unitName, isFiller));
                }
                pronunciations.add(new Pronunciation(units));
                if (!isFiller && addSilEndingPronunciation) {
                    units.add(UnitManager.SILENCE);
                    pronunciations.add(new Pronunciation(units));
                }
            }
        } while (line != null);

        Pronunciation[] pronunciationsArray = pronunciations
                .toArray(new Pronunciation[pronunciations.size()]);
        Word wordObject = createWord(word, pronunciationsArray, isFiller);

        for (Pronunciation pronunciation : pronunciationsArray) {
            pronunciation.setWord(wordObject);
        }
        wordDictionary.put(word, wordObject);

        return wordObject;
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
     * Returns a string representation of this FastDictionary in alphabetical
     * order.
     * 
     * @return a string representation of this FastDictionary
     */
    @Override
    public String toString() {

        SortedMap<String, String> sorted = new TreeMap<String, String>(dictionary);
        StringBuilder result = new StringBuilder();

        for (Map.Entry<String, String> entry : sorted.entrySet()) {
            result.append(entry.getKey());
            result.append("   ").append(entry.getValue()).append('\n');
        }

        return result.toString();
    }

    /**
     * Gets the set of all filler words in the dictionary
     * 
     * @return an array (possibly empty) of all filler words
     */
    public Word[] getFillerWords() {
        Word[] fillerWordArray = new Word[fillerWords.size()];
        int index = 0;
        for (String spelling : fillerWords) {
            fillerWordArray[index++] = getWord(spelling);
        }
        return fillerWordArray;
    }

    /**
     * Dumps this FastDictionary to System.out.
     */
    public void dump() {
        System.out.print(toString());
    }

    /**
     * Loads the dictionary with a list of URLs to custom dictionary resources
     * 
     * @param addenda
     *            the list of custom dictionary URLs to be loaded
     * @throws IOException
     *             if there is an error reading the resource URL
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

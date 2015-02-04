package edu.cmu.sphinx.linguist.language.classes;

import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.*;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * A component that knows how to map words to classes and vice versa.
 *
 * @author Tanel Alumae
 */
public class ClassMap implements Configurable {
    @S4String
    public final static String PROP_CLASS_DEFS_LOCATION = "classDefsLocation";

    private Logger logger;
    private boolean allocated;
    private URL classDefsLocation;
    private LogMath logMath;

    /**
     * Maps class name to class as a Word
     */
    private Map<String, Word> classVocabulary = new HashMap<String, Word>();

    /**
     * Maps a word to it's class and the probability of the word being in this class
     */
    private Map<String, ClassProbability> wordToClassProbabilities = new HashMap<String, ClassProbability>();

    /**
     * Maps a class to a set of words that belong to this class
     */
    private final HashMap<String, Set<String>> classToWord = new HashMap<String, Set<String>>();

    public ClassMap(URL classDefsLocation) {
        this.logger = Logger.getLogger(getClass().getName());
        this.classDefsLocation = classDefsLocation;
        logMath = LogMath.getInstance();
    }

    public ClassMap() {

    }

    /* (non-Javadoc)
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();

        if (allocated)
            throw new RuntimeException("Can't change properties after allocation");

        classDefsLocation = ConfigurationManagerUtils.getResource(PROP_CLASS_DEFS_LOCATION, ps);
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.linguist.language.ngram.LanguageModel#allocate()
    */
    public void allocate() throws IOException {
        if (!allocated) {
            allocated = true;
            loadClassDefs();
        }
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.linguist.language.ngram.LanguageModel#deallocate()
    */
    public void deallocate() {
        allocated = false;
        wordToClassProbabilities = null;
        classVocabulary = null;
    }

    public ClassProbability getClassProbability(String word) {
        return wordToClassProbabilities.get(word);
    }

    public Word getClassAsWord(String text) {
        return classVocabulary.get(text);
    }

    public Set<String> getWordsInClass(String className) {
        return classToWord.get(className);
    }


    /**
     * Loads class definitions.
     * Class definitions should be in SRILM format:
     * <pre>
     * CLASS1 probability1 WORD1
     * CLASS2 probability2 WORD2
     * ...
     * </pre>
     * Probabilities should be given in linear domain.
     *
     * @throws java.io.IOException If an IO error occurs during loading Class definition resource.
     */
    private void loadClassDefs() throws IOException {
        BufferedReader reader = new BufferedReader
                (new InputStreamReader(classDefsLocation.openStream()));
        String line;
        while ((line = reader.readLine()) != null) {
            StringTokenizer st = new StringTokenizer(line, " \t\n\r\f=");
            if (st.countTokens() != 3) {
                throw new IOException("corrupt word to class def: " + line + "; "
                        + st.countTokens());
            }
            String className = st.nextToken().toLowerCase();
            float linearProb = Float.parseFloat(st.nextToken());
            String word = st.nextToken().toLowerCase();
            if (logger.isLoggable(Level.FINE)) {
                logger.fine(word + " --> " + className + " " + linearProb);
            }
            wordToClassProbabilities.put(word,
                    new ClassProbability(className, logMath.linearToLog(linearProb)));
            classVocabulary.put(className, new Word(className, null, false));
            addWordInClass(className, word);
        }
        reader.close();
        checkClasses();
        logger.info("Loaded word to class mappings for " + wordToClassProbabilities.size() + " words");
    }

    /**
     * Checks that word probabilities in each class sum to 1.
     */
    private void checkClasses() {
        Map<String, Float> sums = new HashMap<String, Float>();
        for (ClassProbability cp : wordToClassProbabilities.values()) {
            Float sum = sums.get(cp.getClassName());
            if (sum == null) {
                sums.put(cp.getClassName(), 0f);
            } else {
                sums.put(cp.getClassName(), (float) logMath.logToLinear(cp.getLogProbability()) + sum);
            }
        }

        for (Map.Entry<String, Float> entry : sums.entrySet()) {
            if (Math.abs(1.0 - entry.getValue()) > 0.001) {
                logger.warning("Word probabilities for class " + entry.getKey() + " sum to " + entry.getValue());
            }
        }
    }

    /**
     * @param className Name of the class
     * @param word      Word String
     */
    private void addWordInClass(String className, String word) {
        Set<String> words = classToWord.get(className);
        if (words == null) {
            words = new HashSet<String>();
            classToWord.put(className, words);
        }
        words.add(word);
    }
}

/*
 * Created on Jan 19, 2005
 */
package edu.cmu.sphinx.linguist.language.classes;

import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.linguist.dictionary.WordClassification;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

import java.io.IOException;

/**
 * The only purpose of this class is to present all classes
 * as words in the getWord() method. This dictionary must be injected to
 * a class LM as a dictionary, otherwise classes will be treated as OOVs.
 *
 * @author Tanel Alumae
 */
public class ClassPresentingDictionary implements Dictionary {

    @S4Component(type = ClassMap.class)
    public final static String PROP_CLASS_MAP = "classMap";

    @S4Component(type = Dictionary.class)
    public final static String PROP_WORD_DICTIONARY = "wordDictionary";

    private boolean allocated = false;
    private Dictionary wordDictionary;
    private ClassMap classMap;

    public ClassPresentingDictionary(ClassMap classMap, Dictionary wordDictionary) {
        this.classMap = classMap;
        this.wordDictionary = wordDictionary;
    }

    public ClassPresentingDictionary() {

    }

    public void newProperties(PropertySheet ps) throws PropertyException {
        classMap = (ClassMap) ps.getComponent(PROP_CLASS_MAP);
        wordDictionary = (Dictionary) ps.getComponent(PROP_WORD_DICTIONARY);

    }

    public void allocate() throws IOException {
        if (!allocated) {
            allocated = true;
            wordDictionary.allocate();
            classMap.allocate();
        }
    }


    public void deallocate() {
        allocated = false;
        wordDictionary = null;
    }

    /**
     * This method disguises all classes as words.
     */
    public Word getWord(String text) {
        Word word = classMap.getClassAsWord(text);
        return (word != null) ? word : wordDictionary.getWord(text);
    }

    /* (non-Javadoc)
     * @see edu.cmu.sphinx.linguist.dictionary.Dictionary#getSentenceStartWord()
     */
    public Word getSentenceStartWord() {
        return wordDictionary.getSentenceStartWord();
    }

    /* (non-Javadoc)
     * @see edu.cmu.sphinx.linguist.dictionary.Dictionary#getSentenceEndWord()
     */
    public Word getSentenceEndWord() {
        return wordDictionary.getSentenceEndWord();
    }

    /* (non-Javadoc)
     * @see edu.cmu.sphinx.linguist.dictionary.Dictionary#getSilenceWord()
     */
    public Word getSilenceWord() {
        return wordDictionary.getSilenceWord();
    }

    /* (non-Javadoc)
     * @see edu.cmu.sphinx.linguist.dictionary.Dictionary#getPossibleWordClassifications()
     */
    public WordClassification[] getPossibleWordClassifications() {
        return wordDictionary.getPossibleWordClassifications();
    }

    /* (non-Javadoc)
     * @see edu.cmu.sphinx.linguist.dictionary.Dictionary#getFillerWords()
     */
    public Word[] getFillerWords() {
        return wordDictionary.getFillerWords();
    }

    @Override
    public String toString() {
        return "Word dictionary:\n" + wordDictionary.toString()
                + "Classes:\n" + classMap.toString();
    }
}

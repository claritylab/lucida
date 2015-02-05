/**
 * Created on Jan 19, 2005
 */
package edu.cmu.sphinx.linguist.language.classes;

import edu.cmu.sphinx.linguist.WordSequence;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.linguist.language.ngram.LanguageModel;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

import java.io.IOException;
import java.util.HashSet;
import java.util.Set;

/**
 * An LM that computes a probability of a word sequence by
 * converting words to classes and asking the class-based probability
 * from a delegate LM.
 *
 * @author Tanel Alumae
 */
public class ClassBasedLanguageModel implements LanguageModel {
    /**
     * The property that defines the classLanguageModel component.
     */
    @S4Component(type = LanguageModel.class)
    public final static String PROP_CLASS_LANGUAGE_MODEL = "classLanguageModel";

    /**
     * The property that defines the classMap component.
     */
    @S4Component(type = ClassMap.class)
    public final static String PROP_CLASS_MAP = "classMap";

    // ----------------------------
    // Configuration data
    // ----------------------------
    private LanguageModel classLM;
    private Set<String> vocabulary;

    private boolean allocated = false;

    private ClassMap classMap;

    public ClassBasedLanguageModel(ClassMap classMap, LanguageModel classLM) {
        this.classMap = classMap;
        this.classLM = classLM;
    }

    public ClassBasedLanguageModel() {

    }
        
    /*
     * (non-Javadoc)
     *
     * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
     */
    public void newProperties(PropertySheet ps) throws PropertyException {
        if (allocated) {
            throw new PropertyException(
                    ClassBasedLanguageModel.class.getName(),
                    null,
                    "Can't change properties after allocation");
        }
        classMap = (ClassMap) ps.getComponent(PROP_CLASS_MAP);
        classLM = (LanguageModel) ps.getComponent(PROP_CLASS_LANGUAGE_MODEL);
    }

    /*
     * (non-Javadoc)
     *
     * @see edu.cmu.sphinx.linguist.language.ngram.LanguageModel#allocate()
     */
    public void allocate() throws IOException {
        if (!allocated) {
            allocated = true;
            classMap.allocate();
            classLM.allocate();
            makeVocabulary();
        }
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.linguist.language.ngram.LanguageModel#deallocate()
    */
    public void deallocate() throws IOException {
        allocated = false;
        classLM.deallocate();
        classMap.deallocate();
        vocabulary = null;
    }

    /**
     * Called before a recognition
     */
    public void start() {
    }

    /**
     * Called after a recognition
     */
    public void stop() {
    }

    /*
     * Actual implementation of the class-based LM:
     * P=P(W|C1)*P(C1|C2,C3..)
     *
     * @see edu.cmu.sphinx.linguist.language.ngram.LanguageModel#getProbability(edu.cmu.sphinx.linguist.WordSequence)
     */
    public float getProbability(WordSequence wordSequence) {
        Word[] classes = new Word[wordSequence.size()];
        float wordToClassProb = 0;
        for (int i = 0; i < classes.length; i++) {
            Word sourceWord = wordSequence.getWord(i);
            ClassProbability classProbability = classMap.getClassProbability(sourceWord.getSpelling());
            classes[i] = (classProbability == null ? sourceWord : classMap.getClassAsWord(classProbability.getClassName()));
            if (i == classes.length - 1) {
                if (classProbability != null) {
                    // the first word of the word sequence is a class
                    wordToClassProb = classProbability.getLogProbability();
                }
            }
        }
        float classBasedProbability = classLM.getProbability(new WordSequence(classes));
        return classBasedProbability + wordToClassProb;
    }


    /**
     * Gets the smear term for the given wordSequence
     *
     * @param wordSequence the word sequence
     * @return the smear term associated with this word sequence
     */
    public float getSmear(WordSequence wordSequence) {
        return 0.0f; // TODO not implemented
    }

    /* (non-Javadoc)
     * @see edu.cmu.sphinx.linguist.language.ngram.LanguageModel#getVocabulary()
     */
    public Set<String> getVocabulary() {
        return vocabulary;
    }

    /**
     * Returns the maximum depth of the language model
     *
     * @return the maximum depth of the language mdoel
     */
    public int getMaxDepth() {
        return classLM.getMaxDepth();
    }

    /**
     * Converts a vocabulary of the class LM to a word vocabulary.
     */
    private void makeVocabulary() {
        vocabulary = new HashSet<String>();
        for (String name : classLM.getVocabulary()) {
            Set<String> wordsInClass = classMap.getWordsInClass(name);
            if (wordsInClass == null) {
                // 'name' not a class
                vocabulary.add(name);
            } else {
                vocabulary.addAll(wordsInClass);
            }
        }
    }


}

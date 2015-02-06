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

import edu.cmu.sphinx.util.ExtendedStreamTokenizer;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.*;

import java.io.IOException;
import java.util.*;

/**
 * Defines a grammar based upon a list of words in a file. The format of the file is just one word per line. For
 * example, for an isolated digits grammar the file will simply look like:
 * <pre>
 * zero
 * one
 * two
 * three
 * four
 * five
 * six
 * seven
 * eight
 * nine
 * </pre>
 * The path to the file is defined by the {@link #PROP_PATH PROP_PATH} property. If the {@link #PROP_LOOP PROP_LOOP}
 * property is true, the grammar created will be a looping grammar. Using the above digits grammar example, setting
 * PROP_LOOP to true will make it a connected-digits grammar.
 * <p/>
 * All probabilities are maintained in LogMath log base.
 */
public class SimpleWordListGrammar extends Grammar implements Configurable {

    /** The property that defines the location of the word list grammar */
    @S4String(defaultValue = "spelling.gram")
    public final static String PROP_PATH = "path";

    /** The property that if true, indicates that this is a looping grammar */
    @S4Boolean(defaultValue = true)
    public final static String PROP_LOOP = "isLooping";

    // ---------------------
    // Configurable data
    // ---------------------
    private String path;
    private boolean isLooping;
    private LogMath logMath;

    public SimpleWordListGrammar(String path, boolean isLooping, boolean showGrammar, boolean optimizeGrammar, boolean addSilenceWords, boolean addFillerWords, edu.cmu.sphinx.linguist.dictionary.Dictionary dictionary) {
        super(showGrammar,optimizeGrammar,addSilenceWords,addFillerWords,dictionary);
        this.path = path;
        this.isLooping = isLooping;
        logMath = LogMath.getInstance();
    }

    public SimpleWordListGrammar() {

    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        
        path = ps.getString(PROP_PATH);
        isLooping = ps.getBoolean(PROP_LOOP);
    }


    /**
     * Create class from reference text (not implemented).
     *
     * @param bogusText dummy variable
     */
    @Override
    protected GrammarNode createGrammar(String bogusText)
            throws NoSuchMethodException {
        throw new NoSuchMethodException("Does not create "
                + "grammar with reference text");
    }


    /** Creates the grammar. */
    @Override
    protected GrammarNode createGrammar() throws IOException {
        ExtendedStreamTokenizer tok = new ExtendedStreamTokenizer(path, true);
        GrammarNode initialNode = createGrammarNode("<sil>");
        GrammarNode branchNode = createGrammarNode(false);
        GrammarNode finalNode = createGrammarNode("<sil>");
        finalNode.setFinalNode(true);
        List<GrammarNode> wordGrammarNodes = new LinkedList<GrammarNode>();
        while (!tok.isEOF()) {
            String word;
            while ((word = tok.getString()) != null) {
                word = word.toLowerCase();
                GrammarNode wordNode = createGrammarNode(word);
                wordGrammarNodes.add(wordNode);
            }
        }
        // now connect all the GrammarNodes together
        initialNode.add(branchNode, LogMath.LOG_ONE);
        float branchScore = logMath.linearToLog(
                1.0 / wordGrammarNodes.size());
        for (GrammarNode wordNode : wordGrammarNodes) {
            branchNode.add(wordNode, branchScore);
            wordNode.add(finalNode, LogMath.LOG_ONE);
            if (isLooping) {
                wordNode.add(branchNode, LogMath.LOG_ONE);
            }
        }

        return initialNode;
    }
}

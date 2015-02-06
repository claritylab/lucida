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

import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.util.props.*;

import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.LineNumberReader;
import java.util.HashMap;
import java.util.Map;

/**
 * @author Peter Wolf
 */
public class BatchForcedAlignerGrammar extends ForcedAlignerGrammar implements GrammarInterface {

    /** Property that defines the reference file containing the transcripts used to create the froced align grammar */
    @S4String(defaultValue = "<refFile not set>")
    public final static String PROP_REF_FILE = "refFile";

    protected String refFile;
    protected final Map<String, GrammarNode> grammars = new HashMap<String, GrammarNode>();
    protected String currentUttName = "";

    public BatchForcedAlignerGrammar(String refFile, boolean showGrammar, boolean optimizeGrammar, boolean addSilenceWords,
            boolean addFillerWords, Dictionary dictionary) {
        super(showGrammar, optimizeGrammar, addSilenceWords, addFillerWords, dictionary);
        this.refFile = refFile;
    }
    
    public BatchForcedAlignerGrammar () {
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        refFile = ps.getString(PROP_REF_FILE);
    }


    @Override
    protected GrammarNode createGrammar() {
        // TODO: FlatLinguist requires the initial grammar node
        // to contain a single silence. We'll do that for now,
        // but once the FlatLinguist is fixed, this should be
        // returned to its former method of creating an empty
        // initial grammar node
        //   initialNode = createGrammarNode(initialID, false);

        initialNode = null;
        finalNode = createGrammarNode(true);
        try {
            LineNumberReader in = new LineNumberReader(new FileReader(refFile));
            String line;
            while (true) {
                line = in.readLine();

                if (line == null || line.isEmpty())
                    break;

                int uttNameStart = line.indexOf('(') + 1;
                int uttNameEnd = line.indexOf(')');

                if (uttNameStart < 0 || uttNameStart > uttNameEnd)
                    continue;

                String uttName = line.substring(uttNameStart, uttNameEnd);
                String transcript = line.substring(0, uttNameStart - 1).trim();

                if (transcript.isEmpty())
                    continue;

                initialNode = createGrammarNode(Dictionary.SILENCE_SPELLING);
                createForcedAlignerGrammar(initialNode, finalNode, transcript);
                grammars.put(uttName, initialNode);
                currentUttName = uttName;
            }
            in.close();
        } catch (FileNotFoundException e) {
            throw new Error(e);
        } catch (IOException e) {
            throw new Error(e);
        } catch (NoSuchMethodException e) {
            throw new Error(e);
        }
        return initialNode;
    }


    @Override
    public GrammarNode getInitialNode() {
        return initialNode;
    }


    public void setUtterance(String utteranceName) {
        initialNode = grammars.get(utteranceName);
        assert initialNode != null;
    }
}

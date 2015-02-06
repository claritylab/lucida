/*
 * 
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.instrumentation;

import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.recognizer.Recognizer.State;
import edu.cmu.sphinx.recognizer.StateListener;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.decoder.ResultListener;
import edu.cmu.sphinx.util.props.*;

/** Tracks and reports rejection accuracy. */
public class RejectionTracker implements
        ResultListener,
        Resetable,
        Monitor,
        StateListener {

    /** The property that defines which recognizer to monitor */
    @S4Component(type = Recognizer.class)
    public final static String PROP_RECOGNIZER = "recognizer";

    /** The property that defines whether summary accuracy information is displayed */
    @S4Boolean(defaultValue = true)
    public final static String PROP_SHOW_SUMMARY = "showSummary";

    /** The property that defines whether detailed accuracy information is displayed */
    @S4Boolean(defaultValue = true)
    public final static String PROP_SHOW_DETAILS = "showDetails";

    // ------------------------------
    // Configuration data
    // ------------------------------
    private String name;
    private Recognizer recognizer;
    private boolean showSummary;
    private boolean showDetails;

    /** total number of utterances */
    private int numUtterances;

    /** actual number of out-of-grammar utterance */
    private int numOutOfGrammarUtterances;

    /** number of correctly classified in-grammar utterances */
    private int numCorrectOutOfGrammarUtterances;

    /** number of in-grammar utterances misrecognized as out-of-grammar */
    private int numFalseOutOfGrammarUtterances;

    /** number of correctly classified out-of-grammar utterances */
    private int numCorrectInGrammarUtterances;

    /** number of out-of-grammar utterances misrecognized as in-grammar */
    private int numFalseInGrammarUtterances;


    public RejectionTracker( Recognizer recognizer, boolean showSummary, boolean showDetails ) {
        initRecognizer(recognizer);
        this.showSummary = showSummary;
        this.showDetails = showDetails;
    }

    public RejectionTracker( ) {
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        initRecognizer((Recognizer)ps.getComponent(PROP_RECOGNIZER));
        showSummary = ps.getBoolean(PROP_SHOW_SUMMARY);
        showDetails = ps.getBoolean(PROP_SHOW_DETAILS);
    }

    private void initRecognizer(Recognizer newRecognizer) {
        if (recognizer == null) {
            recognizer = newRecognizer;
            recognizer.addResultListener(this);
            recognizer.addStateListener(this);
        } else if (recognizer != newRecognizer) {
            recognizer.removeResultListener(this);
            recognizer.removeStateListener(this);
            recognizer = newRecognizer;
            recognizer.addResultListener(this);
            recognizer.addStateListener(this);
        }
    }


    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.instrumentation.Resetable
     */
    public void reset() {
        numUtterances = 0;
        numOutOfGrammarUtterances = 0;
        numCorrectOutOfGrammarUtterances = 0;
        numFalseOutOfGrammarUtterances = 0;
        numCorrectInGrammarUtterances = 0;
        numFalseInGrammarUtterances = 0;
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#getName()
    */
    public String getName() {
        return name;
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.decoder.ResultListener#newResult(edu.cmu.sphinx.result.Result)
    */
    public void newResult(Result result) {
        String ref = result.getReferenceText();
        if (result.isFinal() && ref != null) {
            numUtterances++;
            String hyp = result.getBestResultNoFiller();
            if (ref.equals("<unk>")) {
                numOutOfGrammarUtterances++;
                if (hyp.equals("<unk>")) {
                    numCorrectOutOfGrammarUtterances++;
                } else {
                    numFalseInGrammarUtterances++;
                }
            } else {
                if (hyp.equals("<unk>")) {
                    numFalseOutOfGrammarUtterances++;
                } else {
                    numCorrectInGrammarUtterances++;
                }
            }
            printStats();
        }
    }


    private void printStats() {
        if (showSummary) {
            float correctPercent = ((float)
                    (numCorrectOutOfGrammarUtterances +
                            numCorrectInGrammarUtterances)) /
                    ((float) numUtterances) * 100f;
            System.out.println
                    ("   Rejection Accuracy: " + correctPercent + '%');
        }
        if (showDetails) {
            System.out.println
                    ("   Correct OOG: " + numCorrectOutOfGrammarUtterances +
                            "   False OOG: " + numFalseOutOfGrammarUtterances +
                            "   Correct IG: " + numCorrectInGrammarUtterances +
                            "   False IG: " + numFalseInGrammarUtterances +
                            "   Actual number: " + numOutOfGrammarUtterances);
        }
    }


    public void statusChanged(Recognizer.State status) {
        if (status == State.DEALLOCATED) {
            printStats();
        }
    }
}

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

package edu.cmu.sphinx.instrumentation;

import edu.cmu.sphinx.decoder.scorer.Scoreable;
import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.recognizer.Recognizer.State;
import edu.cmu.sphinx.recognizer.StateListener;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.decoder.ResultListener;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Boolean;
import edu.cmu.sphinx.util.props.S4Component;

import java.text.DecimalFormat;
import java.util.Collections;
import java.util.List;

/**
 * Monitors the absolute and relative beam sizes required to achieve the optimum recognition results and reports this
 * data.
 */
public class BeamFinder implements ResultListener,
        Resetable, StateListener, Monitor {

    /** The property that defines which recognizer to monitor */
    @S4Component(type = Recognizer.class)
    public final static String PROP_RECOGNIZER = "recognizer";

    /** The property that defines whether summary accuracy information is displayed */
    @S4Boolean(defaultValue = true)
    public final static String PROP_SHOW_SUMMARY = "showSummary";

    /** The property that defines whether detailed accuracy information is displayed */
    @S4Boolean(defaultValue = true)
    public final static String PROP_SHOW_DETAILS = "showDetails";

    /** The property that defines whether this beam tracker is enabled */
    @S4Boolean(defaultValue = true)
    public final static String PROP_ENABLED = "enable";

    // ------------------------------
    // Configuration data
    // ------------------------------
    private String name;
    private Recognizer recognizer;
    private boolean showSummary;
    private boolean showDetails;
    private boolean enabled;
    private LogMath logMath;

    private int maxAbsoluteBeam;
    private int avgAbsoluteBeam;
    private float maxRelativeBeam;
    private float avgRelativeBeam;

    private int totMaxAbsoluteBeam;
    private int sumAbsoluteBeam;
    private float totMaxRelativeBeam;
    private float sumRelativeBeam;
    private int totalUtterances;

    private final static DecimalFormat logFormatter = new DecimalFormat("0.#E0");
    public final String TOKEN_RANK = "TOKENRANK";
    
    public BeamFinder( Recognizer recognizer, boolean showSummary, boolean showDetails, boolean enabled) {
        initRecognizer(recognizer);
        logMath = LogMath.getInstance();
        this.showSummary = showSummary;
        this.showDetails = showDetails;
        this.enabled = enabled;
    }

    public BeamFinder( ) {

    }

   /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        initRecognizer((Recognizer) ps.getComponent(PROP_RECOGNIZER));
        showSummary = ps.getBoolean(PROP_SHOW_SUMMARY);
        showDetails = ps.getBoolean(PROP_SHOW_DETAILS);
        enabled = ps.getBoolean(PROP_ENABLED);
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
    * @see edu.cmu.sphinx.util.props.Configurable#getName()
    */
    public String getName() {
        return name;
    }


    /** Resets the beam statistics */
    public void reset() {
        maxAbsoluteBeam = 0;
        avgAbsoluteBeam = 0;
        maxRelativeBeam = 0;
        avgRelativeBeam = 0;

        totMaxAbsoluteBeam = 0;
        sumAbsoluteBeam = 0;
        totMaxRelativeBeam = 0;
        sumRelativeBeam = 0;
        totalUtterances = 0;
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.decoder.ResultListener#newResult(edu.cmu.sphinx.result.Result)
    */
    public void newResult(Result result) {
        if (enabled) {
            process(result);
            if (result.isFinal() && showDetails) {
                showLatestResult();
            }
        }
    }

    public void statusChanged(Recognizer.State status) {
        if (enabled && status == State.DEALLOCATED) {
            if (showSummary) {
                showSummary();
            }
        }
    }

    /**
     * Ranks the given set of tokens
     *
     * @param result the result to process
     */
    private void process(Result result) {
        if (result.isFinal()) {
            collectStatistics(result);
        } else {
            List<Token> tokenList = result.getActiveTokens().getTokens();
            if (!tokenList.isEmpty()) {
                Collections.sort(tokenList, Scoreable.COMPARATOR);
                Token bestToken = tokenList.get(0);
                int rank = 0;
                for (Token token : tokenList) {
                    float scoreDiff = bestToken.getScore() -
                            token.getScore();
                    assert scoreDiff >= 0;

                    token.getTokenProps().put(TOKEN_RANK, new TokenRank(rank++, scoreDiff));
                    assert tokenIsRanked(token);
                }
            }
        }
    }


    /**
     * Checks to make sure that all upstream tokens are ranked. Primarily used for debugging
     *
     * @param token the token to check
     */
    private boolean tokenIsRanked(Token token) {
        while (token != null) {
            if (token.isEmitting()) {
                if (token.getTokenProps().get(TOKEN_RANK) == null) {
                    if (token.getFrameNumber() != 0) {
                        System.out.println("MISSING " + token);
                        return false;
                    }
                } else {
                }
            }
            token = token.getPredecessor();
        }
        return true;
    }


    /** show the latest result */
    public void showLatestResult() {
        System.out.print("   Beam Abs Max: " + maxAbsoluteBeam + "  Avg: "
                + avgAbsoluteBeam);
        System.out.println("   Rel Max: "
                + logFormatter.format(logMath.logToLinear(maxRelativeBeam))
                + "  Avg: "
                + logFormatter.format(logMath.logToLinear(avgRelativeBeam)));
    }


    /** show the summary result */
    public void showSummary() {
        System.out.print("   Summary Beam Abs Max: " + totMaxAbsoluteBeam
                + "  Avg: " + sumAbsoluteBeam / totalUtterances);
        System.out.println("   Rel Max: "
                + logFormatter.format(logMath.logToLinear(totMaxRelativeBeam))
                + "  Avg: "
                + logFormatter.format(logMath.logToLinear(sumRelativeBeam
                / totalUtterances)));
    }


    /**
     * Collect statistics from the collected beam data
     *
     * @param result the result of interest
     */
    private void collectStatistics(Result result) {
        totalUtterances++;
        collectAbsoluteBeamStatistics(result);
        collectRelativeBeamStatistics(result);
    }


    /**
     * Collects the absolute beam statistics
     *
     * @param result the result of interest
     */
    private void collectAbsoluteBeamStatistics(Result result) {
        Token token = result.getBestToken();
        int count = 0;
        int sumBeam = 0;
        maxAbsoluteBeam = 0;
        while (token != null) {
            if (token.isEmitting()) {
                TokenRank rank = (TokenRank) token.getTokenProps().get(TOKEN_RANK);
                if (rank != null) {
                    if (rank.getAbsoluteRank() > maxAbsoluteBeam) {
                        maxAbsoluteBeam = rank.getAbsoluteRank();
                    }
                    sumBeam += rank.getAbsoluteRank();
                    count++;
                } else {
                    if (token.getFrameNumber() > 0) {
                        System.out.println("Null rank! for " + token);
                    }
                }
            }
            token = token.getPredecessor();
        }

        if (count > 0) {
            avgAbsoluteBeam = sumBeam / count;
            if (maxAbsoluteBeam > totMaxAbsoluteBeam) {
                totMaxAbsoluteBeam = maxAbsoluteBeam;
            }
            sumAbsoluteBeam += avgAbsoluteBeam;
        }
    }


    /**
     * Returns the maximum relative beam for a the chain of tokens reachable from the given token
     *
     * @param result the result of interest
     */
    private void collectRelativeBeamStatistics(Result result) {
        Token token = result.getBestToken();
        int count = 0;
        double sumBeam = 0.0;

        maxRelativeBeam = -Float.MAX_VALUE;

        while (token != null) {
            if (token.isEmitting()) {
                TokenRank rank = (TokenRank) token.getTokenProps().get(TOKEN_RANK);
                if (rank != null) {
                    if (rank.getRelativeRank() > maxRelativeBeam) {
                        maxRelativeBeam = rank.getRelativeRank();
                    }
                    sumBeam += rank.getRelativeRank();
                    count++;
                } else {
                    if (token.getFrameNumber() > 0) {
                        System.out.println("Null rank! for " + token);
                    }
                }
            }
            token = token.getPredecessor();
        }

        if (count > 0) {
            avgRelativeBeam = (float) (sumBeam / count);
            if (maxRelativeBeam > totMaxRelativeBeam) {
                totMaxRelativeBeam = maxRelativeBeam;
            }
            sumRelativeBeam += avgRelativeBeam;
        }
    }
}

/** A token application object that keeps track of the absolute and relative rank of a token */

class TokenRank {

    private final int absoluteRank;
    private final float relativeRank;


    /**
     * Creates a token rank object
     *
     * @param abs the absolute rank
     * @param rel the relative rank
     */
    TokenRank(int abs, float rel) {
        absoluteRank = abs;
        relativeRank = rel;
    }


    /**
     * Gets the absolute rank
     *
     * @return the absolute rank
     */
    int getAbsoluteRank() {
        return absoluteRank;
    }


    /**
     * Gets the relative rank
     *
     * @return the relative rank
     */
    float getRelativeRank() {
        return relativeRank;
    }


    /**
     * Returns the string representation of this object
     *
     * @return the string representation of this object
     */
    @Override
    public String toString() {
        return "Rank[" + absoluteRank + ',' + relativeRank + ']';
    }
}

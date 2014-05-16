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

package edu.cmu.sphinx.linguist.acoustic.tiedstate.trainer;

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.linguist.acoustic.HMMState;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.Senone;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.SenoneHMM;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.SenoneSequence;

/** Used to transfer data from the trainer to the acoustic model */
public class TrainerScore {

    private Data feature;
    private float logOutputProbability;
    private float logScalingFactor;
    private int senoneID;
    private HMMState hmmState;
    private Senone senone;
    private float logAlpha;
    private float logBeta;
    private float logGamma;
    private float[] logComponentGamma;
    private float[] logComponentProb;
    static private float logLikelihood;


    /**
     * Creates a new TrainerScore
     *
     * @param feature     the current feature
     * @param probability the score for the current frame
     * @param senone      the id for the current senone
     */
    public TrainerScore(Data feature, float probability, int senone) {
        this.feature = feature;
        this.logOutputProbability = probability;
        this.senoneID = senone;
        logScalingFactor = 0.0f;
        logAlpha = 0.0f;
        logBeta = 0.0f;
        logGamma = 0.0f;
        logComponentProb = new float[1];
        logComponentProb[0] = 0.0f;
        logComponentGamma = new float[1];
        logComponentGamma[0] = 0.0f;
    }


    /**
     * Creates a new buffer
     *
     * @param feature          the current feature
     * @param probability      the score for the current frame
     * @param state            the HMMState for this score object
     * @param logAlpha         the forward probability
     * @param logBeta          the backward probability
     * @param logComponentProb the mixture component a posteriori probabilities
     */
    public TrainerScore(Data feature,
                        float probability,
                        HMMState state,
                        float logAlpha,
                        float logBeta,
                        float[] logComponentProb) {

        this.feature = feature;
        this.hmmState = state;
        logScalingFactor = 0.0f;

        // For dummy state, the state is a null pointer
        if ((state != null) && (state.isEmitting())) {
            // get the index and the HMM for this HMMState
            int stateIndex = state.getState();
            SenoneHMM hmm = (SenoneHMM) state.getHMM();
            // Get the senone sequence associated with this HMM
            SenoneSequence ss = hmm.getSenoneSequence();
            // Get the senone associated with this HMMState, located
            // in the stateIndex-th position in the senone sequence
            senone = ss.getSenones()[stateIndex];
            // After this, we need to go to the senone pool to find
            // out the senone id... Or maybe we can operate directly
            // on the senone
        }
        // Now, the probabilities
        this.logOutputProbability = probability;
        this.logAlpha = logAlpha;
        this.logBeta = logBeta;
        // gamma = alpha * beta;
        this.logGamma = logAlpha + logBeta;
        // Compute the gammas for each component in the mixture
        // this.logComponentProb = logComponentProb;
        if (logComponentProb != null) {
            this.logComponentProb = new float[logComponentProb.length];
            this.logComponentGamma = new float[logComponentProb.length];
            for (int i = 0; i < logComponentProb.length; i++) {
                this.logComponentProb[i] = logComponentProb[i];
                this.logComponentGamma[i] =
                        logComponentProb[i] + this.logGamma;
            }
        } else {
            this.logComponentProb = null;
            this.logComponentGamma = null;
        }
    }


    /**
     * Creates a new buffer
     *
     * @param feature          the current feature
     * @param probability      the score for the current frame
     * @param state            the HMMState for this score object
     * @param logAlpha         the forward probability
     * @param logComponentProb the mixture component a posteriori probabilities
     */
    public TrainerScore(Data feature,
                        float probability,
                        HMMState state,
                        float logAlpha,
                        float[] logComponentProb) {
        this(feature, probability, state, logAlpha, 0.0f, logComponentProb);
    }


    /**
     * Creates a new buffer
     *
     * @param feature          the current feature
     * @param probability      the score for the current frame
     * @param state            the HMMState for this score object
     * @param logComponentProb the mixture component a posteriori probabilities
     */
    public TrainerScore(Data feature,
                        float probability,
                        HMMState state,
                        float[] logComponentProb) {
        this(feature, probability, state, 0.0f, 0.0f, logComponentProb);
    }


    /**
     * Creates a new buffer
     *
     * @param feature     the current feature
     * @param probability the score for the current frame
     * @param state       the HMMState for this score object
     */
    public TrainerScore(Data feature,
                        float probability,
                        HMMState state) {
        this(feature, probability, state, 0.0f, 0.0f, null);
    }


    /**
     * Retrieves the Data.
     *
     * @return the Data
     */
    public Data getData() {
        return feature;
    }


    /**
     * Retrieves the probability.
     *
     * @return the probability
     */
    public float getScore() {
        return logOutputProbability;
    }


    /**
     * Retrieves the forward probability.
     *
     * @return the forward log probability
     */
    public float getAlpha() {
        return logAlpha;
    }


    /**
     * Retrieves the backward probability.
     *
     * @return the backward log probability
     */
    public float getBeta() {
        return logBeta;
    }


    /**
     * Retrieves the utterance's log likelihood
     *
     * @return the log likelihood
     */
    static public float getLogLikelihood() {
        return logLikelihood;
    }


    /**
     * Retrieves the a posteriori probability.
     *
     * @return the a posteriori log probability
     */
    public float getGamma() {
        return logGamma;
    }


    /**
     * Retrieves the mixture component a posteriori probability.
     *
     * @return the a posteriori log probabilities
     */
    public float[] getComponentGamma() {
        return logComponentGamma;
    }


    /**
     * Retrieves the scaling factor.
     *
     * @return the scaling factor
     */
    public float getScalingFactor() {
        return logScalingFactor;
    }


    /**
     * Sets the overall likelihood.
     *
     * @param likelihood the log likelihood of the whole utterance
     */
    static public void setLogLikelihood(float likelihood) {
        logLikelihood = likelihood;
    }


    /**
     * Sets the forward probability.
     *
     * @param logAlpha the forward log probability
     */
    public void setAlpha(float logAlpha) {
        this.logAlpha = logAlpha;
    }


    /**
     * Sets the backward probability.
     *
     * @param logBeta the backward log probability
     */
    public void setBeta(float logBeta) {
        this.logBeta = logBeta;
    }


    /**
     * Computes the a posteriori probability. This is the product of the current alpha and beta, or the summation of the
     * current logAlpha and logBeta. The current beta is updated in the object, and the current alpha and beta are
     * used.
     */
    public void setGamma() {
        logGamma = logAlpha + logBeta + logScalingFactor;
        // Compute the gammas for each component in the mixture
        if (logComponentGamma != null) {
            for (int i = 0; i < logComponentGamma.length; i++) {
                logComponentGamma[i] = logComponentProb[i] + logGamma;
                if (logComponentGamma[i] < -Float.MAX_VALUE) {
                    logComponentGamma[i] = -Float.MAX_VALUE;
                }
            }
        }
    }


    /**
     * Sets the scaling factor for this frame..
     *
     * @param logScalingFactor the log scaling factor
     */
    public void setScalingFactor(float logScalingFactor) {
        this.logScalingFactor = logScalingFactor;
    }


    /**
     * Retrieves the senone ID.
     *
     * @return the senone ID
     */
    public int getSenoneID() {
        return senoneID;
    }


    /**
     * Retrieves the senone.
     *
     * @return the senone
     */
    public Senone getSenone() {
        return senone;
    }


    /**
     * Retrieves the HMM state.
     *
     * @return the HMM state
     */
    public HMMState getState() {
        return hmmState;
    }
}


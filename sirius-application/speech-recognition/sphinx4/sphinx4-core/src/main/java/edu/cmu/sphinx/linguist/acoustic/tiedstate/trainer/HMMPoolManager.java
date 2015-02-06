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

import edu.cmu.sphinx.frontend.FloatData;
import edu.cmu.sphinx.linguist.acoustic.HMMState;
import edu.cmu.sphinx.linguist.acoustic.HMM;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.*;
import edu.cmu.sphinx.util.LogMath;

import java.io.IOException;
import java.util.HashMap;
import java.util.logging.Logger;

/** Manages the HMM pools. */
class HMMPoolManager {

    private HMMManager hmmManager;
    private HashMap<Object, Integer> indexMap;
    private Pool<float[]> meansPool;
    private Pool<float[]> variancePool;
    private Pool<float[][]> matrixPool;
    private Pool<float[]> mixtureWeightsPool;
    
    private Pool<Buffer> meansBufferPool;
    private Pool<Buffer> varianceBufferPool;
    private Pool<Buffer[]> matrixBufferPool;
    private Pool<Buffer> mixtureWeightsBufferPool;

    private Pool<Senone> senonePool;
    private LogMath logMath;

    private float logMixtureWeightFloor;
    private float logTransitionProbabilityFloor;
    private float varianceFloor;
    private float logLikelihood;
    private float currentLogLikelihood;

    /** The logger for this class */
    private static Logger logger = Logger.getLogger("edu.cmu.sphinx.linguist.acoustic.HMMPoolManager");

    /**
     * Constructor for this pool manager. It gets the pointers to the pools from a loader.
     *
     * @param loader the loader
     * @throws IOException 
     */
    protected HMMPoolManager(Loader loader) throws IOException {
    	loader.load();
        hmmManager = loader.getHMMManager();
        indexMap = new HashMap<Object, Integer>();
        meansPool = loader.getMeansPool();
        variancePool = loader.getVariancePool();
        mixtureWeightsPool = loader.getMixtureWeightPool();
        matrixPool = loader.getTransitionMatrixPool();
        senonePool = loader.getSenonePool();

//	logMath = LogMath.getLogMath();
//        float mixtureWeightFloor =
//	    props.getFloat(TiedStateAcousticModel.PROP_MW_FLOOR);
//	logMixtureWeightFloor = logMath.linearToLog(mixtureWeightFloor);
//        float transitionProbabilityFloor =
//	    props.getFloat(TiedStateAcousticModel.PROP_TP_FLOOR);
//	logTransitionProbabilityFloor =
//	    logMath.linearToLog(transitionProbabilityFloor);
//        varianceFloor =
//	    props.getFloat(TiedStateAcousticModel.PROP_VARIANCE_FLOOR);

        createBuffers();
        logLikelihood = 0.0f;
        logMath = LogMath.getInstance();
    }

    /** Recreates the buffers. */
    protected void resetBuffers() {
        createBuffers();
        logLikelihood = 0.0f;
    }

    /** Create buffers for all pools used by the trainer in this pool manager. */
    protected void createBuffers() {
        // the option false or true refers to whether the buffer is in
        // log scale or not, true if it is.
        meansBufferPool = create1DPoolBuffer(meansPool, false);
        varianceBufferPool = create1DPoolBuffer(variancePool, false);
        matrixBufferPool = create2DPoolBuffer(matrixPool, true);
        mixtureWeightsBufferPool = create1DPoolBuffer(mixtureWeightsPool, true);
    }


    /** Create buffers for a given pool. */
    private Pool<Buffer> create1DPoolBuffer(Pool<float[]> pool, boolean isLog) {
        Pool<Buffer> bufferPool = new Pool<Buffer>(pool.getName());

        for (int i = 0; i < pool.size(); i++) {
            float[] element = pool.get(i);
            indexMap.put(element, i);
            Buffer buffer = new Buffer(element.length, isLog, i);
            bufferPool.put(i, buffer);
        }
        return bufferPool;
    }

    /** Create buffers for a given pool. */
    private Pool<Buffer[]> create2DPoolBuffer(Pool<float[][]> pool, boolean isLog) {
        Pool<Buffer[]> bufferPool = new Pool<Buffer[]>(pool.getName());

        for (int i = 0; i < pool.size(); i++) {
            float[][] element = pool.get(i);
            indexMap.put(element, i);
            int poolSize = element.length;
            Buffer[] bufferArray = new Buffer[poolSize];
            for (int j = 0; j < poolSize; j++) {
                bufferArray[j] = new Buffer(element[j].length, isLog, j);
            }
            bufferPool.put(i, bufferArray);
        }
        return bufferPool;
    }

    /**
     * Accumulate the TrainerScore into the buffers.
     *
     * @param index the current index into the TrainerScore vector
     * @param score the TrainerScore
     */
    protected void accumulate(int index, TrainerScore[] score) {
        accumulate(index, score, null);
    }

    /**
     * Accumulate the TrainerScore into the buffers.
     *
     * @param index     the current index into the TrainerScore vector
     * @param score     the TrainerScore for the current frame
     * @param nextScore the TrainerScore for the next time frame
     */
    protected void accumulate(int index, TrainerScore[] score, TrainerScore[] nextScore) {
        int senoneID;
        TrainerScore thisScore = score[index];

        // We should be doing this just once per utterance...
        // currentLogLikelihood = thisScore.getLogLikelihood();

        // Since we're scaling, the loglikelihood disappears...
        currentLogLikelihood = 0;
        // And the total becomes the sum of (-) scaling factors
        logLikelihood -= score[0].getScalingFactor();

        SenoneHMMState state = (SenoneHMMState) thisScore.getState();
        if (state == null) {
            // We only care about the case "all models"
            senoneID = thisScore.getSenoneID();
            if (senoneID == TrainerAcousticModel.ALL_MODELS) {
                accumulateMean(senoneID, score[index]);
                accumulateVariance(senoneID, score[index]);
                accumulateMixture(senoneID, score[index]);
                accumulateTransition(senoneID, index, score, nextScore);
            }
        } else {
            // If state is non-emitting, we presume there's only one
            // transition out of it. Therefore, we only accumulate
            // data for emitting states.
            if (state.isEmitting()) {
                senoneID = senonePool.indexOf(state.getSenone());
                // accumulateMean(senoneID, score[index]);
                // accumulateVariance(senoneID, score[index]);
                accumulateMixture(senoneID, score[index]);
                accumulateTransition(senoneID, index, score, nextScore);
            }
        }
    }

    /** Accumulate the means. */
    private void accumulateMean(int senone, TrainerScore score) {
        if (senone == TrainerAcousticModel.ALL_MODELS) {
            for (int i = 0; i < senonePool.size(); i++) {
                accumulateMean(i, score);
            }
        } else {
            GaussianMixture gaussian = (GaussianMixture)senonePool.get(senone);
            MixtureComponent[] mix = gaussian.getMixtureComponents();
            for (int i = 0; i < mix.length; i++) {
                float[] mean = mix[i].getMean();
                // int indexMean = meansPool.indexOf(mean);
                int indexMean = indexMap.get(mean);
                assert indexMean >= 0;
                assert indexMean == senone;
                Buffer buffer = meansBufferPool.get(indexMean);
                float[] feature = ((FloatData) score.getData()).getValues();
                double[] data = new double[feature.length];
                float prob = score.getComponentGamma()[i];
                prob -= currentLogLikelihood;
                double dprob = logMath.logToLinear(prob);
                // prob = (float) logMath.logToLinear(prob);
                for (int j = 0; j < data.length; j++) {
                    data[j] = feature[j] * dprob;
                }
                buffer.accumulate(data, dprob);
            }
        }
    }


    /** Accumulate the variance. */
    private void accumulateVariance(int senone, TrainerScore score) {
        if (senone == TrainerAcousticModel.ALL_MODELS) {
            for (int i = 0; i < senonePool.size(); i++) {
                accumulateVariance(i, score);
            }
        } else {
            GaussianMixture gaussian = (GaussianMixture)senonePool.get(senone);
            MixtureComponent[] mix = gaussian.getMixtureComponents();
            for (int i = 0; i < mix.length; i++) {
                float[] mean = mix[i].getMean();
                float[] variance = mix[i].getVariance();
                // int indexVariance = variancePool.indexOf(variance);
                int indexVariance = indexMap.get(variance);
                Buffer buffer = varianceBufferPool.get(indexVariance);
                float[] feature = ((FloatData) score.getData()).getValues();
                double[] data = new double[feature.length];
                float prob = score.getComponentGamma()[i];
                prob -= currentLogLikelihood;
                double dprob = logMath.logToLinear(prob);
                for (int j = 0; j < data.length; j++) {
                    data[j] = (feature[j] - mean[j]);
                    data[j] *= data[j] * dprob;
                }
                buffer.accumulate(data, dprob);
            }
        }
    }

    /** Accumulate the mixture weights. */
    private void accumulateMixture(int senone, TrainerScore score) {
        // The index into the senone pool and the mixture weight pool
        // is the same
        if (senone == TrainerAcousticModel.ALL_MODELS) {
            for (int i = 0; i < senonePool.size(); i++) {
                accumulateMixture(i, score);
            }
        } else {
            Buffer buffer = mixtureWeightsBufferPool.get(senone);
            float[] mixw = mixtureWeightsPool.get(senone);
            for (int i = 0; i < mixw.length; i++) {
                float prob = score.getComponentGamma()[i];
                prob -= currentLogLikelihood;
                buffer.logAccumulate(prob, i, logMath);
            }
        }
    }

    /**
     * Accumulate transitions from a given state.
     *
     * @param indexScore the current index into the TrainerScore
     * @param score      the score information
     * @param nextScore  the score information for the next frame
     */
    private void accumulateStateTransition(int indexScore, TrainerScore[] score, TrainerScore[] nextScore) {
        HMMState state = score[indexScore].getState();
        if (state == null) {
            // Non-emitting state
            return;
        }
        int indexState = state.getState();
        SenoneHMM hmm = (SenoneHMM) state.getHMM();
        float[][] matrix = hmm.getTransitionMatrix();

        // Find the index for current matrix in the transition matrix pool
        // int indexMatrix = matrixPool.indexOf(matrix);
        int indexMatrix = indexMap.get(matrix);

        // Find the corresponding buffer
        Buffer[] bufferArray = matrixBufferPool.get(indexMatrix);

        // Let's concentrate on the transitions *from* the current state
        float[] vector = matrix[indexState];

        for (int i = 0; i < vector.length; i++) {
            // Make sure this is a valid transition
            if (vector[i] != LogMath.LOG_ZERO) {

                // We're assuming that if the states have position "a"
                // and "b" in the HMM, they'll have positions "k+a"
                // and "k+b" in the graph, that is, their relative
                // position is the same.

                // Distance between current state and "to" state in
                // the HMM
                int dist = i - indexState;

                // "to" state in the graph
                int indexNextScore = indexScore + dist;

                // Make sure the next state is non-emitting (the last
                // in the HMM), or in the same HMM.
                assert ((nextScore[indexNextScore].getState() == null) ||
                        (nextScore[indexNextScore].getState().getHMM() == hmm));
                float alpha = score[indexScore].getAlpha();
                float beta = nextScore[indexNextScore].getBeta();
                float transitionProb = vector[i];
                float outputProb = nextScore[indexNextScore].getScore();
                float prob = alpha + beta + transitionProb + outputProb;
                prob -= currentLogLikelihood;
                // i is the index into the next state.
                bufferArray[indexState].logAccumulate(prob, i, logMath);
                /*
        if ((indexMatrix == 0) && (i == 2)) {
            //    	    System.out.println("Out: " + outputProb);
                //	    	    bufferArray[indexState].dump();
        }
            */
            }
        }
    }

    /**
     * Accumulate transitions from a given state.
     *
     * @param indexState the state index
     * @param hmm        the HMM
     * @param value      the value to accumulate
     */
    private void accumulateStateTransition(int indexState, SenoneHMM hmm, float value) {
        // Find the transition matrix in this hmm
        float[][] matrix = hmm.getTransitionMatrix();

        // Find the vector with transitions from the current state to
        // other states.
        float[] stateVector = matrix[indexState];

        // Find the index of the current transition matrix in the
        // transition matrix pool.
        // int indexMatrix = matrixPool.indexOf(matrix);
        int indexMatrix = indexMap.get(matrix);

        // Find the buffer for the transition matrix.
        Buffer[] bufferArray = matrixBufferPool.get(indexMatrix);

        // Accumulate for the transitions from current state
        for (int i = 0; i < stateVector.length; i++) {
            // Make sure we're not trying to accumulate in an invalid
            // transition.
            if (stateVector[i] != LogMath.LOG_ZERO) {
                bufferArray[indexState].logAccumulate(value, i, logMath);
            }
        }
    }

    /** Accumulate the transition probabilities. */
    private void accumulateTransition(int indexHmm, int indexScore, TrainerScore[] score, TrainerScore[] nextScore) {
        if (indexHmm == TrainerAcousticModel.ALL_MODELS) {
            // Well, special case... we want to add an amount to all
            // the states in all models
            for (HMM hmm : hmmManager) {
                for (int j = 0; j < hmm.getOrder(); j++) {
                    accumulateStateTransition(j, (SenoneHMM)hmm, score[indexScore].getScore());
                }
            }
        } else {
            // For transition accumulation, we don't consider the last
            // time frame, since there's no transition from there to
            // anywhere...
            if (nextScore != null) {
                accumulateStateTransition(indexScore, score, nextScore);
            }
        }
    }

    /** Update the log likelihood. This method should be called for every utterance. */
    protected void updateLogLikelihood() {
        // logLikelihood += currentLogLikelihood;
    }

    /**
     * Normalize the buffers.
     *
     * @return the log likelihood associated with the current training set
     */
    protected float normalize() {
        normalizePool(meansBufferPool);
        normalizePool(varianceBufferPool);
        logNormalizePool(mixtureWeightsBufferPool);
        logNormalize2DPool(matrixBufferPool, matrixPool);
        return logLikelihood;
    }

    /**
     * Normalize a single buffer pool.
     *
     * @param pool the buffer pool to normalize
     */
    private void normalizePool(Pool<Buffer> pool) {
        assert pool != null;
        for (int i = 0; i < pool.size(); i++) {
            Buffer buffer = pool.get(i);
            if (buffer.wasUsed()) {
                buffer.normalize();
            }
        }
    }

    /**
     * Normalize a single buffer pool in log scale.
     *
     * @param pool the buffer pool to normalize
     */
    private void logNormalizePool(Pool<Buffer> pool) {
        assert pool != null;
        for (int i = 0; i < pool.size(); i++) {
            Buffer buffer = pool.get(i);
            if (buffer.wasUsed()) {
                buffer.logNormalize();
            }
        }
    }

    /**
     * Normalize a 2D buffer pool in log scale. Typically, this is the case with the transition matrix, which also needs
     * a mask for values that are allowed, and therefor have to be updated, or not allowed, and should be ignored.
     *
     * @param pool     the buffer pool to normalize
     * @param maskPool pool containing a mask with zero/non-zero values.
     */
    private void logNormalize2DPool(Pool<Buffer[]> pool, Pool<float[][]> maskPool) {
        assert pool != null;
        for (int i = 0; i < pool.size(); i++) {
            Buffer[] bufferArray = pool.get(i);
            float[][] mask = maskPool.get(i);
            for (int j = 0; j < bufferArray.length; j++) {
                if (bufferArray[j].wasUsed()) {
                    bufferArray[j].logNormalizeNonZero(mask[j]);
                }
            }
        }
    }

    /** Update the models. */
    protected void update() {
        updateMeans();
        updateVariances();
        recomputeMixtureComponents();
        updateMixtureWeights();
        updateTransitionMatrices();
    }

    /**
     * Copy one vector onto another.
     *
     * @param in  the source vector
     * @param out the destination vector
     */
    private void copyVector(float[] in, float[] out) {
        assert in.length == out.length;
        System.arraycopy(in, 0, out, 0, in.length);
    }

    /** Update the means. */
    private void updateMeans() {
        assert meansPool.size() == meansBufferPool.size();
        for (int i = 0; i < meansPool.size(); i++) {
            float[] means = meansPool.get(i);
            Buffer buffer = meansBufferPool.get(i);
            if (buffer.wasUsed()) {
                float[] meansBuffer = buffer.getValues();
                copyVector(meansBuffer, means);
            } else {
                logger.info("Senone " + i + " not used.");
            }
        }
    }

    /** Update the variances. */
    private void updateVariances() {
        assert variancePool.size() == varianceBufferPool.size();
        for (int i = 0; i < variancePool.size(); i++) {
            float[] means = meansPool.get(i);
            float[] variance = variancePool.get(i);
            Buffer buffer = varianceBufferPool.get(i);
            if (buffer.wasUsed()) {
                float[] varianceBuffer = buffer.getValues();
                assert means.length == varianceBuffer.length;
                for (int j = 0; j < means.length; j++) {
                    varianceBuffer[j] -= means[j] * means[j];
                    if (varianceBuffer[j] < varianceFloor) {
                        varianceBuffer[j] = varianceFloor;
                    }
                }
                copyVector(varianceBuffer, variance);
            }
        }
    }

    /** Recompute the precomputed values in all mixture components. */
    private void recomputeMixtureComponents() {
        for (int i = 0; i < senonePool.size(); i++) {
            GaussianMixture gMix = (GaussianMixture) senonePool.get(i);
            MixtureComponent[] mixComponent = gMix.getMixtureComponents();
            for (MixtureComponent component : mixComponent) {
                component.precomputeDistance();
            }
        }
    }

    /** Update the mixture weights. */
    private void updateMixtureWeights() {
        assert mixtureWeightsPool.size() == mixtureWeightsBufferPool.size();
        for (int i = 0; i < mixtureWeightsPool.size(); i++) {
            float[] mixtureWeights = mixtureWeightsPool.get(i);
            Buffer buffer = mixtureWeightsBufferPool.get(i);
            if (buffer.wasUsed()) {
                if (buffer.logFloor(logMixtureWeightFloor)) {
                    buffer.logNormalizeToSum(logMath);
                }
                float[] mixtureWeightsBuffer = buffer.getValues();
                copyVector(mixtureWeightsBuffer, mixtureWeights);
            }
        }
    }

    /** Update the transition matrices. */
    private void updateTransitionMatrices() {
        assert matrixPool.size() == matrixBufferPool.size();
        for (int i = 0; i < matrixPool.size(); i++) {
            float[][] matrix = matrixPool.get(i);
            Buffer[] bufferArray = matrixBufferPool.get(i);
            for (int j = 0; j < matrix.length; j++) {
                Buffer buffer = bufferArray[j];
                if (buffer.wasUsed()) {
                    for (int k = 0; k < matrix[j].length; k++) {
                        float bufferValue = buffer.getValue(k);
                        if (bufferValue != LogMath.LOG_ZERO) {
                            assert matrix[j][k] != LogMath.LOG_ZERO;
                            if (bufferValue < logTransitionProbabilityFloor) {
                                buffer.setValue(k, logTransitionProbabilityFloor);
                            }
                        }
                    }
                    buffer.logNormalizeToSum(logMath);
                    copyVector(buffer.getValues(), matrix[j]);
                }
            }
        }
    }
}

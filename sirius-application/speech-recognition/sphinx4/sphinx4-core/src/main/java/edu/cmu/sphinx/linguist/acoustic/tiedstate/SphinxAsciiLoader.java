/*
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
package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import edu.cmu.sphinx.linguist.acoustic.*;
import static edu.cmu.sphinx.linguist.acoustic.tiedstate.Pool.Feature.*;
import edu.cmu.sphinx.util.ExtendedStreamTokenizer;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.*;

import java.io.*;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Sphinx loader that loads ASCII versions.
 */

public class SphinxAsciiLoader extends Sphinx3Loader {

    public SphinxAsciiLoader(URL location, String model, String dataLocation,
            UnitManager unitManager, float distFloor, float mixtureWeightFloor,
            float varianceFloor, boolean useCDUnits)
    {
        init(location, model, dataLocation, unitManager, distFloor, mixtureWeightFloor, varianceFloor, useCDUnits,
                Logger.getLogger(getClass().getName()));
    }

    public SphinxAsciiLoader(String location, String model,
            String dataLocation, UnitManager unitManager,
            float distFloor, float mixtureWeightFloor, float varianceFloor,
            boolean useCDUnits)
        throws MalformedURLException, ClassNotFoundException
    {
        init(ConfigurationManagerUtils.resourceToURL(location), model,
                dataLocation, unitManager, distFloor,
                mixtureWeightFloor, varianceFloor, useCDUnits, Logger.getLogger(getClass().getName()));
    }

    public SphinxAsciiLoader() {

    }

    @Override
    protected void loadModelFiles(String modelDef) throws IOException, URISyntaxException {

        logger.config("Loading Sphinx3 acoustic model: " + modelDef);
        logger.config("    modelName: " + this.model);
        logger.config("    dataLocation   : " + dataLocation);

        meansPool = loadDensityFile(dataLocation + "means.ascii", -Float.MAX_VALUE);
        variancePool = loadDensityFile(dataLocation + "variances.ascii", varianceFloor);
        mixtureWeightsPool = loadMixtureWeights(dataLocation + "mixture_weights.ascii", mixtureWeightFloor);
        transitionsPool = loadTransitionMatrices(dataLocation + "transition_matrices.ascii");

        senonePool = createSenonePool(distFloor, varianceFloor);
        // load the HMM modelDef file
        InputStream modelStream = getDataStream(this.model);
        if (modelStream == null) {
            throw new IOException("can't find modelDef " + this.model);
        }
        loadHMMPool(useCDUnits, modelStream, this.model);
    }

    /**
     * Loads the sphinx3 density file, a set of density arrays are created and
     * placed in the given pool.
     * 
     * @param path
     *            the name of the data
     * @param floor
     *            the minimum density allowed
     * @return a pool of loaded densities
     * @throws FileNotFoundException
     *             if a file cannot be found
     * @throws IOException
     *             if an error occurs while loading the data
     */
    @Override
    protected Pool<float[]> loadDensityFile(String path, float floor) throws IOException, URISyntaxException {
        logger.fine("Loading density file from: " + path);
        InputStream inputStream = getDataStream(path);
        if (inputStream == null) {
            throw new FileNotFoundException("Error trying to read file " + path);
        }

        // 'false' argument refers to EOL is insignificant
        ExtendedStreamTokenizer est = new ExtendedStreamTokenizer(inputStream, '#', false);
        Pool<float[]> pool = new Pool<float[]>(path);
        est.expectString("param");
        int numStates = est.getInt("numStates");
        int numStreams = est.getInt("numStreams");
        int numGaussiansPerState = est.getInt("numGaussiansPerState");
        pool.setFeature(NUM_SENONES, numStates);
        pool.setFeature(NUM_STREAMS, numStreams);
        pool.setFeature(NUM_GAUSSIANS_PER_STATE, numGaussiansPerState);
        int vectorLength = 39;

        for (int i = 0; i < numStates; i++) {
            est.expectString("mgau");
            est.expectInt("mgau index", i);
            est.expectString("feat");
            est.expectInt("feat index", 0);
            for (int j = 0; j < numGaussiansPerState; j++) {
                est.expectString("density");
                est.expectInt("densityValue", j);
                float[] density = new float[vectorLength];
                for (int k = 0; k < vectorLength; k++) {
                    density[k] = est.getFloat("val");
                    if (density[k] < floor) {
                        density[k] = floor;
                    }
                }
                int id = i * numGaussiansPerState + j;
                pool.put(id, density);
            }
        }
        est.close();
        return pool;
    }

    /**
     * Loads the mixture weights.
     * 
     * @param path
     *            the path to the mixture weight file
     * @param floor
     *            the minimum mixture weight allowed
     * @return a pool of mixture weights
     * @throws FileNotFoundException
     *             if a file cannot be found
     * @throws IOException
     *             if an error occurs while loading the data
     */
    @Override
    protected Pool<float[]> loadMixtureWeights(String path, float floor) throws IOException, URISyntaxException {
        logger.fine("Loading mixture weights from: " + path);
        InputStream inputStream = getDataStream(path);
        if (inputStream == null) {
            throw new FileNotFoundException("Error trying to read file " + path);
        }

        Pool<float[]> pool = new Pool<float[]>(path);
        ExtendedStreamTokenizer est = new ExtendedStreamTokenizer(inputStream, '#', false);
        est.expectString("mixw");
        int numStates = est.getInt("numStates");
        int numStreams = est.getInt("numStreams");
        int numGaussiansPerState = est.getInt("numGaussiansPerState");
        pool.setFeature(NUM_SENONES, numStates);
        pool.setFeature(NUM_STREAMS, numStreams);
        pool.setFeature(NUM_GAUSSIANS_PER_STATE, numGaussiansPerState);
        for (int i = 0; i < numStates; i++) {
            est.expectString("mixw");
            est.expectString("[" + i);
            est.expectString("0]");
            // float total = est.getFloat("total");
            float[] logMixtureWeight = new float[numGaussiansPerState];
            for (int j = 0; j < numGaussiansPerState; j++) {
                float val = est.getFloat("mixwVal");
                if (val < floor) {
                    val = floor;
                }
                logMixtureWeight[j] = val;
            }
            LogMath.getInstance().linearToLog(logMixtureWeight);
            pool.put(i, logMixtureWeight);
        }
        est.close();
        return pool;
    }

    /**
     * Loads the transition matrices.
     * 
     * @param path
     *            the path to the transitions matrices
     * @return a pool of transition matrices
     * @throws FileNotFoundException
     *             if a file cannot be found
     * @throws IOException
     *             if an error occurs while loading the data
     */
    @Override
    protected Pool<float[][]> loadTransitionMatrices(String path) throws IOException, URISyntaxException {
        logger.fine("Loading transition matrices from: " + path);

        InputStream inputStream = getDataStream(path);
        if (inputStream == null) {
            throw new FileNotFoundException("Error trying to read file " + path);
        }

        Pool<float[][]> pool = new Pool<float[][]>(path);
        ExtendedStreamTokenizer est =
            new ExtendedStreamTokenizer(inputStream, '#', false);
        LogMath logMath = LogMath.getInstance();
        est.expectString("tmat");
        int numMatrices = est.getInt("numMatrices");
        int numStates = est.getInt("numStates");
        logger.fine("with " + numMatrices + " and " + numStates + " states");

        // read in the matrices
        for (int i = 0; i < numMatrices; i++) {
            est.expectString("tmat");
            est.expectString("[" + i + ']');
            float[][] tmat = new float[numStates][numStates];
            for (int j = 0; j < numStates; j++) {
                for (int k = 0; k < numStates; k++) {
                    // the last row is just zeros, so we just do
                    // the first (numStates - 1) rows
                    if (j < numStates - 1) {
                        if (k == j || k == j + 1) {
                            tmat[j][k] = est.getFloat("tmat value");
                        }
                    }
                    tmat[j][k] = logMath.linearToLog(tmat[j][k]);
                    if (logger.isLoggable(Level.FINE)) {
                        logger.fine("tmat j " + j + " k " + k + " tm " + tmat[j][k]);
                    }
                }
            }
            pool.put(i, tmat);
        }
        est.close();
        return pool;
    }
}

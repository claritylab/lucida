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

import edu.cmu.sphinx.linguist.acoustic.HMM;
import edu.cmu.sphinx.linguist.acoustic.HMMPosition;
import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.*;
import static edu.cmu.sphinx.linguist.acoustic.tiedstate.Pool.Feature.*;
import edu.cmu.sphinx.util.ExtendedStreamTokenizer;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.StreamFactory;
import edu.cmu.sphinx.util.props.*;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.StreamCorruptedException;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Properties;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * an acoustic model loader that initializes models
 * <p/>
 * Mixture weights and transition probabilities are maintained in logMath log base,
 */
public class ModelInitializerLoader implements Loader {

    private final static String SILENCE_CIPHONE = "SIL";

    public final static String MODEL_VERSION = "0.3";

    private final static int CONTEXT_SIZE = 1;

    private Pool<float[]> meansPool;
    private Pool<float[]> variancePool;
    private Pool<float[][]> matrixPool;
    private Pool<float[][]> meanTransformationMatrixPool;
    private Pool<float[]> meanTransformationVectorPool;
    private Pool<float[][]> varianceTransformationMatrixPool;
    private Pool<float[]> varianceTransformationVectorPool;
    private Pool<float[]> mixtureWeightsPool;

    private Pool<Senone> senonePool;
    private int vectorLength = 39;

    private Map<String, Unit> contextIndependentUnits;
    private Map<String, Integer> phoneList;
    private HMMManager hmmManager;

    @S4String(defaultValue="model")
    public static final String MODEL_NAME = "modelName";

    @S4String(defaultValue = ".")
    public static final String LOCATION = "location";

    @S4String(defaultValue = "phonelist")
    public static final String PHONE_LIST = "phones";

    @S4String(defaultValue = "data")
    public static final String DATA_DIR = "dataDir";

    @S4String(defaultValue = "model.props")
    public static final String PROP_FILE = "propsFile";

    @S4Component(type = UnitManager.class)
    public final static String PROP_UNIT_MANAGER = "unitManager";
    private UnitManager unitManager;

    @S4Boolean(defaultValue = false)
    public final static String PROP_USE_CD_UNITS = "useCDUnits";

    @S4Double(defaultValue = 0.0001f)
    public final static String PROP_VARIANCE_FLOOR = "varianceFloor";

    /** Mixture component score floor. */
    @S4Double(defaultValue = 0.0)
    public final static String PROP_MC_FLOOR = "MixtureComponentScoreFloor";

    /** Mixture weight floor. */
    @S4Double(defaultValue = 1e-7f)
    public final static String PROP_MW_FLOOR = "mixtureWeightFloor";

    private LogMath logMath;

    /** The logger for this class */
    private Logger logger;

    public void newProperties(PropertySheet ps) throws PropertyException {
        logMath = LogMath.getInstance();
        logger = ps.getLogger();

        unitManager = (UnitManager) ps.getComponent(PROP_UNIT_MANAGER);

        hmmManager = new HMMManager();
        contextIndependentUnits = new LinkedHashMap<String, Unit>();
        phoneList = new LinkedHashMap<String, Integer>();

        meanTransformationMatrixPool = createDummyMatrixPool("meanTransformationMatrix");
        meanTransformationVectorPool = createDummyVectorPool("meanTransformationMatrix");
        varianceTransformationMatrixPool = createDummyMatrixPool("varianceTransformationMatrix");
        varianceTransformationVectorPool = createDummyVectorPool("varianceTransformationMatrix");

        String modelName = ps.getString(MODEL_NAME);

        String location = ps.getString(LOCATION);
        String phone = ps.getString(PHONE_LIST);
        String dataDir = ps.getString(DATA_DIR);

        logger.info("Creating Sphinx3 acoustic model: " + modelName);
        logger.info("    Path      : " + location);
        logger.info("    phonelist : " + phone);
        logger.info("    dataDir   : " + dataDir);

        // load the HMM model file
        boolean useCDUnits = ps.getBoolean(PROP_USE_CD_UNITS);

        assert !useCDUnits;
        try {
            loadPhoneList(ps, useCDUnits, StreamFactory.getInputStream(location, phone), location + File.separator + phone);
        } catch (StreamCorruptedException sce) {
            printPhoneListHelp();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /** Prints out a help message with format of phone list. */
    private void printPhoneListHelp() {
        System.out.println("The format for the phone list file is:");
        System.out.println("\tversion 0.1");
        System.out.println("\tsame_sized_models yes");
        System.out.println("\tn_state 3");
        System.out.println("\ttmat_skip (no|yes)");
        System.out.println("\tAA");
        System.out.println("\tAE");
        System.out.println("\tAH");
        System.out.println("\t...");
        System.out.println("Or:");
        System.out.println("\tversion 0.1");
        System.out.println("\tsame_sized_models no");
        System.out.println("\ttmat_skip (no|yes)");
        System.out.println("\tAA 5");
        System.out.println("\tAE 3");
        System.out.println("\tAH 4");
        System.out.println("\t...");
    }

    public Map<String, Unit> getContextIndependentUnits() {
        return contextIndependentUnits;
    }

    /**
     * Adds a model to the senone pool.
     *
     * @param pool          the senone pool
     * @param stateID       vector with senone ID for an HMM
     * @param distFloor     the lowest allowed score
     * @param varianceFloor the lowest allowed variance
     * @return the senone pool
     */
    private void addModelToSenonePool(Pool<Senone> pool, int[] stateID, float distFloor, float varianceFloor) {
        assert pool != null;

//        int numMixtureWeights = mixtureWeightsPool.size();

        /*
      int numMeans = meansPool.size();
      int numVariances = variancePool.size();
      int numSenones = mixtureWeightsPool.getFeature(NUM_SENONES, 0);
      int whichGaussian = 0;

      logger.fine("NG " + numGaussiansPerSenone);
      logger.fine("NS " + numSenones);
      logger.fine("NMIX " + numMixtureWeights);
      logger.fine("NMNS " + numMeans);
      logger.fine("NMNS " + numVariances);

      assert numMixtureWeights == numSenones;
      assert numVariances == numSenones * numGaussiansPerSenone;
      assert numMeans == numSenones * numGaussiansPerSenone;
      */
        int numGaussiansPerSenone = mixtureWeightsPool.getFeature(NUM_GAUSSIANS_PER_STATE, 0);
        assert numGaussiansPerSenone > 0;
        for (int state : stateID) {
            MixtureComponent[] mixtureComponents = new MixtureComponent[numGaussiansPerSenone];
            for (int j = 0; j < numGaussiansPerSenone; j++) {
                int whichGaussian = state * numGaussiansPerSenone + j;
                mixtureComponents[j] = new MixtureComponent(
                    meansPool.get(whichGaussian),
                    meanTransformationMatrixPool.get(0),
                    meanTransformationVectorPool.get(0),
                    variancePool.get(whichGaussian),
                    varianceTransformationMatrixPool.get(0),
                    varianceTransformationVectorPool.get(0),
                    distFloor,
                    varianceFloor);
            }

            Senone senone = new GaussianMixture(mixtureWeightsPool.get(state), mixtureComponents, state);

            pool.put(state, senone);
        }
    }

    /**
     * Adds a set of density arrays to a given pool.
     *
     * @param pool                 the pool to add densities to
     * @param stateID              a vector with the senone id of the states in a model
     * @param numStreams           the number of streams
     * @param numGaussiansPerState the number of Gaussians per state
     * @throws IOException if an error occurs while loading the data
     */
    private void addModelToDensityPool(Pool<float[]> pool, int[] stateID, int numStreams, int numGaussiansPerState)
            throws IOException {
        assert pool != null;
        assert stateID != null;

        int numStates = stateID.length;

        int numInPool = pool.getFeature(NUM_SENONES, 0);
        pool.setFeature(NUM_SENONES, numStates + numInPool);
        numInPool = pool.getFeature(NUM_STREAMS, -1);
        if (numInPool == -1) {
            pool.setFeature(NUM_STREAMS, numStreams);
        } else {
            assert numInPool == numStreams;
        }
        numInPool = pool.getFeature(NUM_GAUSSIANS_PER_STATE, -1);
        if (numInPool == -1) {
            pool.setFeature(NUM_GAUSSIANS_PER_STATE, numGaussiansPerState);
        } else {
            assert numInPool == numGaussiansPerState;
        }

        // TODO: numStreams should be any number > 0, but for now....
        assert numStreams == 1;
        for (int i = 0; i < numStates; i++) {
            int state = stateID[i];
            for (int j = 0; j < numGaussiansPerState; j++) {
                // We're creating densities here, so it's ok if values
                // are all zero.
                float[] density = new float[vectorLength];
                int id = state * numGaussiansPerState + j;
                pool.put(id, density);
            }
        }
    }

    /**
     * If a data point is below 'floor' make it equal to floor.
     *
     * @param data the data to floor
     * @param floor the floored value
     */
    private void floorData(float[] data, float floor) {
        for (int i = 0; i < data.length; i++) {
            if (data[i] < floor) {
                data[i] = floor;
            }
        }
    }

    /**
     * Normalize the given data.
     *
     * @param data the data to normalize
     */
    private void normalize(float[] data) {
        float sum = 0;
        for (float val : data) {
            sum += val;
        }

        if (sum != 0.0f) {
            // Invert, so we multiply instead of dividing inside the loop
            sum = 1.0f / sum;
            for (int i = 0; i < data.length; i++) {
                data[i] = data[i] * sum;
            }
        }
    }

    /**
     * Loads the phone list, which possibly contains the sizes (number of states) of models.
     *
     * @param ps
     * @param useCDUnits  if true, uses context dependent units
     * @param inputStream the open input stream to use
     * @param path        the path to a density file @throws FileNotFoundException if a file cannot be found
     * @throws IOException if an error occurs while loading the data
     */
    private void loadPhoneList(PropertySheet ps, boolean useCDUnits, InputStream inputStream, String path)
            throws IOException {
        int numState = 0;
        // TODO: this should be flexible, but we're hardwiring for now
        int numStreams = 1;
        // Since we're initializing, we start simple.
        int numGaussiansPerState = 1;

        ExtendedStreamTokenizer est = new ExtendedStreamTokenizer(inputStream, '#', false);

        // Initialize the pools we'll need.
        meansPool = new Pool<float[]>("means");
        variancePool = new Pool<float[]>("variances");
        mixtureWeightsPool = new Pool<float[]>("mixtureweights");
        matrixPool = new Pool<float[][]>("transitionmatrices");
        senonePool = new Pool<Senone>("senones");

        float distFloor = ps.getFloat(PROP_MC_FLOOR);
        float mixtureWeightFloor = ps.getFloat(PROP_MW_FLOOR);
        float transitionProbabilityFloor = 0;
        float varianceFloor = ps.getFloat(PROP_VARIANCE_FLOOR);

        logger.info("Loading phone list file from: ");
        logger.info(path);

        // At this point, we only accept version 0.1
        String version = "0.1";
        est.expectString("version");
        est.expectString(version);

        est.expectString("same_sized_models");
        boolean sameSizedModels = est.getString().equals("yes");

        if (sameSizedModels) {
            est.expectString("n_state");
            numState = est.getInt("numBase");
        }

        // for this phone list version, let's assume left-to-right
        // models, with optional state skip.
        est.expectString("tmat_skip");
        boolean tmatSkip = est.getString().equals("yes");

        // Load the phones with sizes

        // stateIndex contains the absolute state index, that is, a
        // unique index in the senone pool.
        for (int stateIndex = 0, unitCount = 0; ;) {
            String phone = est.getString();
            if (est.isEOF()) {
                break;
            }
            int size = numState;
            if (!sameSizedModels) {
                size = est.getInt("ModelSize");
            }
            phoneList.put(phone, size);
            logger.fine("Phone: " + phone + " size: " + size);
            int[] stid = new int[size];
            String position = "-";

            for (int j = 0; j < size; j++, stateIndex++) {
                stid[j] = stateIndex;
            }

            Unit unit = unitManager.getUnit(phone,  phone.equals(SILENCE_CIPHONE));

            contextIndependentUnits.put(unit.getName(), unit);

            if (logger.isLoggable(Level.FINE)) {
                logger.fine("Loaded " + unit + " with " + size + " states");
            }

            // Means
            addModelToDensityPool(meansPool, stid, numStreams, numGaussiansPerState);

            // Variances
            addModelToDensityPool(variancePool, stid, numStreams, numGaussiansPerState);

            // Mixture weights
            addModelToMixtureWeightPool(mixtureWeightsPool, stid, numStreams, numGaussiansPerState, mixtureWeightFloor);

            // Transition matrix
            addModelToTransitionMatrixPool(matrixPool, unitCount, stid.length, transitionProbabilityFloor, tmatSkip);

            // After creating all pools, we create the senone pool.
            addModelToSenonePool(senonePool, stid, distFloor, varianceFloor);

            // With the senone pool in place, we go through all units, and
            // create the HMMs.

            // Create tmat
            float[][] transitionMatrix = matrixPool.get(unitCount);
            SenoneSequence ss = getSenoneSequence(stid);

            HMM hmm = new SenoneHMM(unit, ss, transitionMatrix, HMMPosition.lookup(position));
            hmmManager.put(hmm);
            unitCount++;
        }

        // If we want to use this code to load sizes/create models for
        // CD units, we need to find another way of establishing the
        // number of CI models, instead of just reading until the end
        // of file.

        est.close();
    }

    /**
     * Gets the senone sequence representing the given senones.
     *
     * @param stateid is the array of senone state ids
     * @return the senone sequence associated with the states
     */

    private SenoneSequence getSenoneSequence(int[] stateid) {
        Senone[] senones = new Senone[stateid.length];

        for (int i = 0; i < stateid.length; i++) {
            senones[i] = senonePool.get(stateid[i]);
        }

        // TODO: Is there any advantage in trying to pool these?
        return new SenoneSequence(senones);
    }

    /**
     * Adds model to the mixture weights
     *
     * @param pool                 the pool to add models to
     * @param stateID              vector containing state ids for hmm
     * @param numStreams           the number of streams
     * @param numGaussiansPerState the number of Gaussians per state
     * @param floor                the minimum mixture weight allowed
     * @throws IOException if an error occurs while loading the data
     */
    private void addModelToMixtureWeightPool(Pool<float[]> pool, int[] stateID,
                                             int numStreams, int numGaussiansPerState, float floor)
            throws IOException {

        int numStates = stateID.length;

        assert pool != null;

        int numInPool = pool.getFeature(NUM_SENONES, 0);
        pool.setFeature(NUM_SENONES, numStates + numInPool);
        numInPool = pool.getFeature(NUM_STREAMS, -1);
        if (numInPool == -1) {
            pool.setFeature(NUM_STREAMS, numStreams);
        } else {
            assert numInPool == numStreams;
        }
        numInPool = pool.getFeature(NUM_GAUSSIANS_PER_STATE, -1);
        if (numInPool == -1) {
            pool.setFeature(NUM_GAUSSIANS_PER_STATE, numGaussiansPerState);
        } else {
            assert numInPool == numGaussiansPerState;
        }

        // TODO: allow any number for numStreams
        assert numStreams == 1;
        for (int i = 0; i < numStates; i++) {
            int state = stateID[i];
            float[] logMixtureWeight = new float[numGaussiansPerState];
            // Initialize the weights with the same value, e.g. floor
            floorData(logMixtureWeight, floor);
            // Normalize, so the numbers are not all too low
            normalize(logMixtureWeight);
            logMath.linearToLog(logMixtureWeight);
            pool.put(state, logMixtureWeight);
        }
    }

    /**
     * Adds transition matrix to the transition matrices pool
     *
     * @param pool              the pool to add matrix to
     * @param hmmId             current HMM's id
     * @param numEmittingStates number of states in current HMM
     * @param floor             the transition probability floor
     * @param skip              if true, states can be skipped
     * @throws IOException if an error occurs while loading the data
     */
    private void addModelToTransitionMatrixPool(Pool<float[][]> pool, int hmmId, int numEmittingStates,
                                                float floor, boolean skip)
            throws IOException {

        assert pool != null;

        // Add one to account for the last, non-emitting, state
        int numStates = numEmittingStates + 1;

        float[][] tmat = new float[numStates][numStates];

        for (int j = 0; j < numStates; j++) {
            for (int k = 0; k < numStates; k++) {
                // Just to be sure...
                tmat[j][k] = 0.0f;

                // the last row is just zeros, so we just do
                // the first (numStates - 1) rows

                // The value assigned could be anything, provided
                // we normalize it.
                if (j < numStates - 1) {
                    // Usual case: state can transition to itself
                    // or the next state.
                    if (k == j || k == j + 1) {
                        tmat[j][k] = floor;
                    }
                    // If we can skip, we can also transition to
                    // the next state
                    if (skip) {
                        if (k == j + 2) {
                            tmat[j][k] = floor;
                        }
                    }
                }
            }
            normalize(tmat[j]);
            logMath.linearToLog(tmat[j]);
        }
        pool.put(hmmId, tmat);
    }

    /**
     * Creates a pool with a single identity matrix in it.
     *
     * @param name the name of the pool
     * @return the pool with the matrix
     */
    private Pool<float[][]> createDummyMatrixPool(String name) {
        Pool<float[][]> pool = new Pool<float[][]>(name);
        float[][] matrix = new float[vectorLength][vectorLength];
        logger.info("creating dummy matrix pool " + name);
        for (int i = 0; i < vectorLength; i++) {
            for (int j = 0; j < vectorLength; j++) {
                if (i == j) {
                    matrix[i][j] = 1.0F;
                } else {
                    matrix[i][j] = 0.0F;
                }
            }
        }

        pool.put(0, matrix);
        return pool;
    }

    /**
     * Creates a pool with a single zero vector in it.
     *
     * @param name the name of the pool
     * @return the pool with the vector
     */
    private Pool<float[]> createDummyVectorPool(String name) {
        logger.info("creating dummy vector pool " + name);
        Pool<float[]> pool = new Pool<float[]>(name);
        float[] vector = new float[vectorLength];
        for (int i = 0; i < vectorLength; i++) {
            vector[i] = 0.0f;
        }
        pool.put(0, vector);
        return pool;
    }

    public void load() throws IOException {
    }

    public Pool<float[]> getMeansPool() {
        return meansPool;
    }

    public Pool<float[][]> getMeansTransformationMatrixPool() {
        return meanTransformationMatrixPool;
    }

    public Pool<float[]> getMeansTransformationVectorPool() {
        return meanTransformationVectorPool;
    }

    public Pool<float[]> getVariancePool() {
        return variancePool;
    }

    public Pool<float[][]> getVarianceTransformationMatrixPool() {
        return varianceTransformationMatrixPool;
    }

    public Pool<float[]> getVarianceTransformationVectorPool() {
        return varianceTransformationVectorPool;
    }

    public Pool<float[]> getMixtureWeightPool() {
        return mixtureWeightsPool;
    }

    public Pool<float[][]> getTransitionMatrixPool() {
        return matrixPool;
    }

    public float[][] getTransformMatrix() {
        return null;
    }

    public Pool<Senone> getSenonePool() {
        return senonePool;
    }

    public int getLeftContextSize() {
        return CONTEXT_SIZE;
    }

    public int getRightContextSize() {
        return CONTEXT_SIZE;
    }

    public HMMManager getHMMManager() {
        return hmmManager;
    }

    public void logInfo() {
        logger.info("Sphinx3Loader");
        meansPool.logInfo(logger);
        variancePool.logInfo(logger);
        matrixPool.logInfo(logger);
        senonePool.logInfo(logger);
        meanTransformationMatrixPool.logInfo(logger);
        meanTransformationVectorPool.logInfo(logger);
        varianceTransformationMatrixPool.logInfo(logger);
        varianceTransformationVectorPool.logInfo(logger);
        mixtureWeightsPool.logInfo(logger);
        senonePool.logInfo(logger);
        logger.info("Context Independent Unit Entries: " + contextIndependentUnits.size());
        hmmManager.logInfo(logger);
    }

    public Properties getProperties() {
        return new Properties();
    }
}

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

import edu.cmu.sphinx.linguist.acoustic.*;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.*;
import static edu.cmu.sphinx.linguist.acoustic.tiedstate.Pool.Feature.*;
import edu.cmu.sphinx.util.ExtendedStreamTokenizer;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.StreamFactory;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Integer;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Map;
import java.util.logging.Level;

/**
 * an acoustic model loader that loads sphinx3 ascii data
 * <p/>
 * Mixture weights and transition probabilities are maintained in logMath log base,
 */
class Sphinx4Loader extends Sphinx3Loader {

    protected final static String TMAT_FILE_VERSION = "4.0";

    @S4Integer(defaultValue = 10)
    public final static String MAX_MODEL_SIZE = "maxStatePerModel";
    private int maxModelSize;


    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        maxModelSize = ps.getInt(MAX_MODEL_SIZE);
    }

    /**
     * Loads the sphinx4 density file, a set of density arrays are created and placed in the given pool.
     *
     * {@inheritDoc}
     */
    @Override
    protected void loadHMMPool(boolean useCDUnits, InputStream inputStream, String path)
            throws IOException {
        ExtendedStreamTokenizer est = new ExtendedStreamTokenizer(inputStream, '#', false);

        logger.info("Loading HMM file from: ");
        logger.info(path);

        est.expectString(MODEL_VERSION);

        int numBase = est.getInt("numBase");
        est.expectString("n_base");

        int numTri = est.getInt("numTri");
        est.expectString("n_tri");

        est.getInt("numStateMap");
        est.expectString("n_state_map");

        int numTiedState = est.getInt("numTiedState");
        est.expectString("n_tied_state");

        int numContextIndependentTiedState = est.getInt("numContextIndependentTiedState");
        est.expectString("n_tied_ci_state");

        int numTiedTransitionMatrices = est.getInt("numTiedTransitionMatrices");
        est.expectString("n_tied_tmat");

        int[] maxStid = new int[maxModelSize];

        HMMManager hmmManager = super.getHmmManager();
        Pool<float[][]> matrixPool = super.getMatrixPool();
        Pool<float[]> mixtureWeightsPool = super.getMixtureWeightsPool();
        Map<String, Unit> contextIndependentUnits = super.getContextIndependentUnits();

        assert numTiedState == mixtureWeightsPool.getFeature(NUM_SENONES, 0);
        assert numTiedTransitionMatrices == matrixPool.size();

        // Load the base phones
        for (int i = 0; i < numBase; i++) {
            String left = est.getString();
            String right = est.getString();
            String position = est.getString();
            int tmat = est.getInt("tmat");

            // Read all state ID in the line...
            for (int j = 0; ; j++) {
                String str = est.getString();

                // ... until we reach a "N"

                if (!str.equals("N")) {
                    int id = Integer.parseInt(str);
                    try {
                        maxStid[j] = id;
                    } catch (ArrayIndexOutOfBoundsException aie) {
                        throw new Error("Use a larger value for " +
                                "maxStatePerModel");
                    }
                    assert maxStid[j] >= 0 &&
                            maxStid[j] < numContextIndependentTiedState;
                } else {
                    break;
                }
            }

            assert left.equals("-");
            assert right.equals("-");
            assert position.equals("-");
            assert tmat < numTiedTransitionMatrices;

//          int[] stid = Arrays.copyOf(maxStid, numStates);
//          Unit unit = Unit.getUnit(name, attribute.equals(FILLER));
//          contextIndependentUnits.put(unit.getName(), unit);
//
//          if (logger.isLoggable(Level.FINE)) {
//              logger.fine("Loaded " + unit);
//          }
//           The first filler
//          if (unit.isFiller() && unit.getName().equals(SILENCE_CIPHONE)) {
//                unit = Unit.SILENCE;
//          }
//          float[][] transitionMatrix = matrixPool.get(tmat);
//          SenoneSequence ss = getSenoneSequence(stid);
//          HMM hmm = new SenoneHMM(unit, ss, transitionMatrix, HMMPosition.lookup(position));
//          hmmManager.put(hmm);
        }

        // Load the context dependent phones.  If the useCDUnits
        // property is false, the CD phones will not be created, but
        // the values still need to be read in from the file.

        String lastUnitName = "";
        Unit lastUnit = null;
        int[] lastStid = null;
        SenoneSequence lastSenoneSequence = null;

        for (int i = 0; i < numTri; i++) {
            String name = est.getString();
            String left = est.getString();
            String right = est.getString();
            String position = est.getString();
            String attribute = est.getString();
            int tmat = est.getInt("tmat");

            int numStates = 0;

            // Read all state ID in the line...
            for (int j = 0; ; j++) {
                String str = est.getString();

                // ... until we reach a "N"

                if (!str.equals("N")) {
                    int id = Integer.parseInt(str);
                    maxStid[j] = id;
                    assert maxStid[j] >= numContextIndependentTiedState &&
                            maxStid[j] < numTiedState;
                } else {
                    numStates = j;
                    break;
                }
            }
            int[] stid = Arrays.copyOf(maxStid, numStates);

            assert !left.equals("-");
            assert !right.equals("-");
            assert !position.equals("-");
            assert attribute.equals("n/a");
            assert tmat < numTiedTransitionMatrices;

            if (useCDUnits) {
                Unit unit = null;
                String unitName = (name + ' ' + left + ' ' + right);

                if (unitName.equals(lastUnitName)) {
                    unit = lastUnit;
                } else {
                    Unit[] leftContext = new Unit[1];
                    leftContext[0] = contextIndependentUnits.get(left);

                    Unit[] rightContext = new Unit[1];
                    rightContext[0] = contextIndependentUnits.get(right);

                    //Context context = LeftRightContext.get(leftContext, rightContext);
                    //unit = Unit.getUnit(name, false, context);
                }
                lastUnitName = unitName;
                lastUnit = unit;

                if (logger.isLoggable(Level.FINE)) {
                    logger.fine("Loaded " + unit);
                }

                float[][] transitionMatrix = matrixPool.get(tmat);

                SenoneSequence ss = lastSenoneSequence;
                if (ss == null || !sameSenoneSequence(stid, lastStid)) {
                    ss = getSenoneSequence(stid);
                }
                lastSenoneSequence = ss;
                lastStid = stid;

                HMM hmm = new SenoneHMM(unit, ss, transitionMatrix, HMMPosition.lookup(position));
                hmmManager.put(hmm);
            }
        }

        est.close();
    }

    @Override
    protected Pool<float[][]> loadTransitionMatrices(String path)
            throws FileNotFoundException, IOException {

        String location = "";
        InputStream inputStream = StreamFactory.getInputStream(location, path);

        LogMath logMath = LogMath.getInstance();
        logger.info("Loading transition matrices from: ");
        logger.info(path);

        Pool<float[][]> pool = new Pool<float[][]>(path);
        ExtendedStreamTokenizer est = new ExtendedStreamTokenizer(inputStream, '#', false);

        est.expectString("tmat");
        int numMatrices = est.getInt("numMatrices");
        est.expectString("X");
        // numStates = est.getInt("numStates");

        for (int i = 0; i < numMatrices; i++) {
            est.expectString("tmat");
            est.expectString("[" + i + ']');
            est.expectString("nstate");
            // Number of emitting states + 1, final non-emitting state
            int numStates = est.getInt("numStates") + 1;
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
                        logger.fine("tmat j " + j + " k "
                                + k + " tm " + tmat[j][k]);
                    }
                }
            }
            pool.put(i, tmat);
        }
        est.close();
        return pool;
    }
}

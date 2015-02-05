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

import edu.cmu.sphinx.linguist.acoustic.LeftRightContext;
import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.linguist.acoustic.HMM;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.*;
import static edu.cmu.sphinx.linguist.acoustic.tiedstate.Pool.Feature.*;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.StreamFactory;
import edu.cmu.sphinx.util.Utilities;
import edu.cmu.sphinx.util.props.*;

import java.io.*;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * An acoustic model saver that saves sphinx3 ascii data.
 * <p/>
 * Mixture weights and transition probabilities are saved in linear scale.
 */
public class Sphinx3Saver implements Saver {

    /**
     * The property specifying whether the transition matrices of the acoustic model is in sparse form, i.e.,
     * omitting the zeros of the non-transitioning states.
     */
    @S4Boolean(defaultValue = true)
    public final static String PROP_SPARSE_FORM = "sparseForm";
    protected boolean sparseForm;

    @S4Boolean(defaultValue = true)
    public final static String PROP_USE_CD_UNITS = "useCDUnits";

    @S4Double(defaultValue = 0.0f)
    public final static String PROP_MC_FLOOR = "MixtureComponentScoreFloor";


    @S4Component(type = Loader.class)
    public static final String LOADER = "loader";

    @S4Integer(defaultValue = 39)
    public final static String PROP_VECTOR_LENGTH = "vectorLength";

    protected Logger logger;

    protected final static String FILLER = "filler";
    protected final static String SILENCE_CIPHONE = "SIL";

    protected final static int BYTE_ORDER_MAGIC = 0x11223344;

    public final static String MODEL_VERSION = "0.3";

    protected final static int CONTEXT_SIZE = 1;

    private String checksum;
    private boolean doCheckSum;

    private Pool<float[]> meansPool;
    private Pool<float[]> variancePool;
    private Pool<float[][]> matrixPool;
    private Pool<float[][]> meanTransformationMatrixPool;
    private Pool<float[]> meanTransformationVectorPool;
    private Pool<float[][]> varianceTransformationMatrixPool;
    private Pool<float[]> varianceTransformationVectorPool;
    private Pool<float[]> mixtureWeightsPool;

    private Pool<Senone> senonePool;
    private int vectorLength;

    private Map<String, Unit>  contextIndependentUnits;
    private HMMManager hmmManager;
    protected LogMath logMath;
    private boolean binary;
    private String location;
    private boolean swap;

    protected final static String DENSITY_FILE_VERSION = "1.0";
    protected final static String MIXW_FILE_VERSION = "1.0";
    protected final static String TMAT_FILE_VERSION = "1.0";

    @S4String(defaultValue = ".")
    public static final String SAVE_LOCATION = "saveLocation";

    @S4String(mandatory = false, defaultValue = "")
    public final static String DATA_LOCATION = "dataLocation";
    private String dataDir;

    @S4String(mandatory = false, defaultValue = "")
    public final static  String DEF_FILE = "definitionFile";

    public boolean useCDUnits;

    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();

        location = ps.getString(SAVE_LOCATION);
        dataDir = ps.getString(DATA_LOCATION);

        sparseForm = ps.getBoolean(PROP_SPARSE_FORM);
        useCDUnits = ps.getBoolean(PROP_USE_CD_UNITS);
        logMath = LogMath.getInstance();

        // extract the feature vector length
        vectorLength = ps.getInt(PROP_VECTOR_LENGTH);

        Loader loader = (Loader) ps.getComponent(LOADER);
        hmmManager = loader.getHMMManager();
        meansPool = loader.getMeansPool();
        variancePool = loader.getVariancePool();
        mixtureWeightsPool = loader.getMixtureWeightPool();
        matrixPool = loader.getTransitionMatrixPool();
        senonePool = loader.getSenonePool();
        contextIndependentUnits = new LinkedHashMap<String, Unit> ();

        // TODO: read checksum from props;
        checksum = "no";
        doCheckSum = (checksum != null && checksum.equals("yes"));
        swap = false;
    }

    /**
     * Return the checksum string.
     *
     * @return the checksum
     */
    protected String getCheckSum() {
        return checksum;
    }

    /**
     * Return whether to do the dochecksum. If true, checksum is performed.
     *
     * @return the dochecksum
     */
    protected boolean getDoCheckSum() {
        return doCheckSum;
    }

    /**
     * Return the location.
     *
     * @return the location
     */
    protected String getLocation() {
        return location;
    }

    /**
     * Saves the AcousticModel from a directory in the file system.
     *
     * @param modelName the name of the acoustic model; if null we just save from the default location
     */
    public void save(String modelName, boolean b) throws IOException {

        logger.info("Saving acoustic model: " + modelName);
        logger.info("    Path      : " + location);
        logger.info("    modellName: " + modelName);
        logger.info("    dataDir   : " + dataDir);

        // save the acoustic properties file (am.props), 
        // create a different URL depending on the data format
        
        if (binary) {
            // First, overwrite the previous file
            saveDensityFileBinary(meansPool, dataDir + "means", true);
            // From now on, append to previous file
            saveDensityFileBinary(variancePool, dataDir + "variances", true);
            saveMixtureWeightsBinary(mixtureWeightsPool, dataDir + "mixture_weights", true);
            saveTransitionMatricesBinary(matrixPool, dataDir + "transition_matrices", true);

        } else {
            saveDensityFileAscii(meansPool, dataDir + "means.ascii", true);
            saveDensityFileAscii(variancePool, dataDir + "variances.ascii", true);
            saveMixtureWeightsAscii(mixtureWeightsPool, dataDir + "mixture_weights.ascii", true);
            saveTransitionMatricesAscii(matrixPool, dataDir + "transition_matrices.ascii", true);
        }

        // save the HMM model file

        saveHMMPool(useCDUnits, StreamFactory.getOutputStream(location, "mdef", true), location + File.separator + "mdef");
    }

    public Map<String, Unit> getContextIndependentUnits() {
        return contextIndependentUnits;
    }


    /**
     * Saves the sphinx3 density file, a set of density arrays are created and placed in the given pool.
     *
     * @param pool   the pool to be saved
     * @param path   the name of the data
     * @param append is true, the file will be appended, useful if saving to a ZIP or JAR file
     * @throws FileNotFoundException if a file cannot be found
     * @throws IOException           if an error occurs while saving the data
     */
    private void saveDensityFileAscii(Pool<float[]> pool, String path, boolean append)
            throws FileNotFoundException, IOException {
        logger.info("Saving density file to: ");
        logger.info(path);

        OutputStream outputStream = StreamFactory.getOutputStream(location, path, append);

        if (outputStream == null) {
            throw new IOException("Error trying to write file " + location + path);
        }
        PrintWriter pw = new PrintWriter(outputStream, true);

        pw.print("param ");
        int numStates = pool.getFeature(NUM_SENONES, -1);
        pw.print(numStates + " ");
        int numStreams = pool.getFeature(NUM_STREAMS, -1);
        pw.print(numStreams + " ");
        int numGaussiansPerState = pool.getFeature(NUM_GAUSSIANS_PER_STATE, -1);
        pw.println(numGaussiansPerState);

        for (int i = 0; i < numStates; i++) {
            pw.println("mgau " + i);
            pw.println("feat " + 0);
            for (int j = 0; j < numGaussiansPerState; j++) {

                pw.print("density" + " \t" + j);

                int id = i * numGaussiansPerState + j;
                float[] density = pool.get(id);
                for (int k = 0; k < vectorLength; k++) {
                    pw.print(" " + density[k]);
                    //   System.out.println(" " + i + " " + j + " " + k +
                    //          " " + density[k]);
                }
                pw.println();
            }
        }
        outputStream.close();
    }

    /**
     * Saves the sphinx3 density file, a set of density arrays are created and placed in the given pool.
     *
     * @param pool   the pool to be saved
     * @param path   the name of the data
     * @param append is true, the file will be appended, useful if saving to a ZIP or JAR file
     * @throws FileNotFoundException if a file cannot be found
     * @throws IOException           if an error occurs while saving the data
     */
    private void saveDensityFileBinary(Pool<float[]> pool, String path, boolean append)
            throws FileNotFoundException, IOException {
        Properties props = new Properties();
        int checkSum = 0;

        logger.info("Saving density file to: ");
        logger.info(path);

        props.setProperty("version", DENSITY_FILE_VERSION);
        props.setProperty("chksum0", checksum);

        DataOutputStream dos = writeS3BinaryHeader(location, path, props, append);

        int numStates = pool.getFeature(NUM_SENONES, -1);
        int numStreams = pool.getFeature(NUM_STREAMS, -1);
        int numGaussiansPerState = pool.getFeature(NUM_GAUSSIANS_PER_STATE, -1);

        writeInt(dos, numStates);
        writeInt(dos, numStreams);
        writeInt(dos, numGaussiansPerState);

        int rawLength = 0;
        int[] vectorLength = new int[numStreams];
        for (int i = 0; i < numStreams; i++) {
            vectorLength[i] = this.vectorLength;
            writeInt(dos, vectorLength[i]);
            rawLength += numGaussiansPerState * numStates * vectorLength[i];
        }

        assert numStreams == 1;
        assert rawLength == numGaussiansPerState * numStates * this.vectorLength;
        writeInt(dos, rawLength);

        //System.out.println("Nstates " + numStates);
        //System.out.println("Nstreams " + numStreams);
        //System.out.println("NgaussiansPerState " + numGaussiansPerState);
        //System.out.println("vectorLength " + vectorLength.length);
        //System.out.println("rawLength " + rawLength);

        for (int i = 0; i < numStates; i++) {
            for (int j = 0; j < numStreams; j++) {
                for (int k = 0; k < numGaussiansPerState; k++) {
                    int id = i * numStreams * numGaussiansPerState +
                            j * numGaussiansPerState + k;
                    float[] density = pool.get(id);
                    // Do checksum here?
                    writeFloatArray(dos, density);
                }
            }
        }
        if (doCheckSum) {
            assert doCheckSum = false : "Checksum not supported";
        }
        // S3 requires some number here....
        writeInt(dos, checkSum);
        // BUG: not checking the check sum yet.
        dos.close();
    }

    /**
     * Writes the S3 binary header to the given location+path.
     *
     * @param location the location of the file
     * @param path     the name of the file
     * @param props    the properties
     * @param append   is true, the file will be appended, useful if saving to a ZIP or JAR file
     * @return the output stream positioned after the header
     * @throws IOException on error
     */
    protected DataOutputStream writeS3BinaryHeader(String location, String path, Properties props, boolean append)
            throws IOException {

        OutputStream outputStream = StreamFactory.getOutputStream(location, path, append);
        if (doCheckSum) {
            assert false : "Checksum not supported";
        }
        DataOutputStream dos = new DataOutputStream(new BufferedOutputStream(outputStream));

        writeWord(dos, "s3\n");

        for (Enumeration<Object> e = props.keys(); e.hasMoreElements();) {
            String name = (String) e.nextElement();
            String value = props.getProperty(name);
            writeWord(dos, name + ' ' + value + '\n');
        }
        writeWord(dos, "endhdr\n");

        writeInt(dos, BYTE_ORDER_MAGIC);

        return dos;
    }

    /**
     * Writes the next word (without surrounding white spaces) to the given stream.
     *
     * @param dos  the output stream
     * @param word the next word
     * @throws IOException on error
     */
    void writeWord(DataOutputStream dos, String word) throws IOException {
        dos.writeBytes(word);
    }

    
    /**
     * Writes an integer to the output stream, byte-swapping as necessary
     *
     * @param dos the output stream
     * @param val an integer value
     * @throws IOException on error
     */
    protected void writeInt(DataOutputStream dos, int val) throws IOException {
        if (swap) {
            dos.writeInt(Utilities.swapInteger(val));
        } else {
            dos.writeInt(val);
        }
    }

    /**
     * Writes a float to the output stream, byte-swapping as necessary
     *
     * @param dos the input stream
     * @param val a float value
     * @throws IOException on error
     */
    protected void writeFloat(DataOutputStream dos, float val)
            throws IOException {
        if (swap) {
            dos.writeFloat(Utilities.swapFloat(val));
        } else {
            dos.writeFloat(val);
        }
    }


    /**
     * Writes the given number of floats from an array of floats to a stream.
     *
     * @param dos  the stream to write the data to
     * @param data the array of floats to write to the stream
     * @throws IOException if an exception occurs
     */
    protected void writeFloatArray(DataOutputStream dos, float[] data)
            throws IOException {
        for (float val : data) {
            writeFloat(dos, val);
        }
    }

    /**
     * Saves the sphinx3 density file, a set of density arrays are created and placed in the given pool.
     *
     * @param useCDUnits   if true, uses context dependent units
     * @param outputStream the open output stream to use
     * @param path         the path to a density file
     * @throws FileNotFoundException if a file cannot be found
     * @throws IOException           if an error occurs while saving the data
     */
    private void saveHMMPool(boolean useCDUnits, OutputStream outputStream, String path)
            throws FileNotFoundException, IOException {
        logger.info("Saving HMM file to: ");
        logger.info(path);

        if (outputStream == null) {
            throw new IOException("Error trying to write file "
                    + location + path);
        }
        PrintWriter pw = new PrintWriter(outputStream, true);

        /*
      ExtendedStreamTokenizer est = new ExtendedStreamTokenizer
              (outputStream, '#', false);
          Pool pool = new Pool(path);
      */

        // First, count the HMMs
        int numBase = 0;
        int numTri = 0;
        int numContextIndependentTiedState = 0;
        int numStateMap = 0;
        for (HMM hmm : hmmManager) {
            numStateMap += hmm.getOrder() + 1;
            if (((SenoneHMM)hmm).isContextDependent()) {
                numTri++;
            } else {
                numBase++;
                numContextIndependentTiedState += hmm.getOrder();
            }
        }
        pw.println(MODEL_VERSION);
        pw.println(numBase + " n_base");
        pw.println(numTri + " n_tri");
        pw.println(numStateMap + " n_state_map");
        int numTiedState = mixtureWeightsPool.getFeature(NUM_SENONES, 0);
        pw.println(numTiedState + " n_tied_state");
        pw.println(numContextIndependentTiedState + " n_tied_ci_state");
        int numTiedTransitionMatrices = numBase;
        assert numTiedTransitionMatrices == matrixPool.size();
        pw.println(numTiedTransitionMatrices + " n_tied_tmat");

        pw.println("#");
        pw.println("# Columns definitions");
        pw.println("#base lft  rt p attrib tmat      ... state id's ...");

        // Save the base phones
        for (HMM hmm0 : hmmManager) {
            SenoneHMM hmm = (SenoneHMM)hmm0;
            if (hmm.isContextDependent()) {
                continue;
            }

            Unit unit = hmm.getUnit();

            String name = unit.getName();
            pw.print(name + '\t');
            String left = "-";
            pw.print(left + "   ");
            String right = "-";
            pw.print(right + ' ');
            String position = hmm.getPosition().toString();
            pw.print(position + '\t');
            String attribute = unit.isFiller() ? FILLER : "n/a";
            pw.print(attribute + '\t');
            int tmat = matrixPool.indexOf(hmm.getTransitionMatrix());
            assert tmat < numTiedTransitionMatrices;
            pw.print(tmat + "\t");

            SenoneSequence ss = hmm.getSenoneSequence();
            Senone[] senones = ss.getSenones();
            for (Senone senone : senones) {
                int index = senonePool.indexOf(senone);
                assert index >= 0 && index < numContextIndependentTiedState;
                pw.print(index + "\t");
            }
            pw.println("N");

            if (logger.isLoggable(Level.FINE)) {
                logger.fine("Saved " + unit);
            }

        }

        // Save the context dependent phones.

        for (HMM hmm0 : hmmManager) {
            SenoneHMM hmm = (SenoneHMM)hmm0;
            if (!hmm.isContextDependent()) {
                continue;
            }

            Unit unit = hmm.getUnit();
            LeftRightContext context = (LeftRightContext)unit.getContext();
            Unit[] leftContext = context.getLeftContext();
            Unit[] rightContext = context.getRightContext();
            assert leftContext.length == 1 && rightContext.length == 1;

            String name = unit.getName();
            pw.print(name + '\t');
            String left = leftContext[0].getName();
            pw.print(left + "   ");
            String right = rightContext[0].getName();
            pw.print(right + ' ');
            String position = hmm.getPosition().toString();
            pw.print(position + '\t');
            String attribute = unit.isFiller() ? FILLER : "n/a";
            assert attribute.equals("n/a");
            pw.print(attribute + '\t');
            int tmat = matrixPool.indexOf(hmm.getTransitionMatrix());
            assert tmat < numTiedTransitionMatrices;
            pw.print(tmat + "\t");

            SenoneSequence ss = hmm.getSenoneSequence();
            Senone[] senones = ss.getSenones();
            for (Senone senone : senones) {
                int index = senonePool.indexOf(senone);
                assert index >= 0 && index < numTiedState;
                pw.print(index + "\t");
            }
            pw.println("N");

            if (logger.isLoggable(Level.FINE)) {
                logger.fine("Saved " + unit);
            }

        }

        outputStream.close();
    }


    /**
     * Saves the mixture weights
     *
     * @param pool   the mixture weight pool
     * @param path   the path to the mixture weight file
     * @param append is true, the file will be appended, useful if saving to a ZIP or JAR file
     * @throws FileNotFoundException if a file cannot be found
     * @throws IOException           if an error occurs while saving the data
     */
    private void saveMixtureWeightsAscii(Pool<float[]> pool, String path, boolean append)
            throws FileNotFoundException, IOException {
        logger.info("Saving mixture weights to: ");
        logger.info(path);

        OutputStream outputStream = StreamFactory.getOutputStream(location, path, append);
        if (outputStream == null) {
            throw new IOException("Error trying to write file " + location + path);
        }
        PrintWriter pw = new PrintWriter(outputStream, true);

        pw.print("mixw ");
        int numStates = pool.getFeature(NUM_SENONES, -1);
        pw.print(numStates + " ");
        int numStreams = pool.getFeature(NUM_STREAMS, -1);
        pw.print(numStreams + " ");
        int numGaussiansPerState = pool.getFeature(NUM_GAUSSIANS_PER_STATE, -1);
        pw.println(numGaussiansPerState);

        for (int i = 0; i < numStates; i++) {
            pw.print("mixw [" + i + " 0] ");
            float[] mixtureWeight = new float[numGaussiansPerState];
            float[] logMixtureWeight = pool.get(i);
            logMath.logToLinear(logMixtureWeight, mixtureWeight);

            float sum = 0.0f;
            for (int j = 0; j < numGaussiansPerState; j++) {
                sum += mixtureWeight[j];
            }
            pw.println(sum);
            pw.print("\n\t");
            for (int j = 0; j < numGaussiansPerState; j++) {
                pw.print(" " + mixtureWeight[j]);
            }
            pw.println();
        }
        outputStream.close();
    }

    /**
     * Saves the mixture weights (Binary)
     *
     * @param pool   the mixture weight pool
     * @param path   the path to the mixture weight file
     * @param append is true, the file will be appended, useful if saving to a ZIP or JAR file
     * @return a pool of mixture weights
     * @throws FileNotFoundException if a file cannot be found
     * @throws IOException           if an error occurs while saving the data
     */
    private void saveMixtureWeightsBinary(Pool<float[]> pool, String path, boolean append)
            throws FileNotFoundException, IOException {
        logger.info("Saving mixture weights to: ");
        logger.info(path);

        Properties props = new Properties();

        props.setProperty("version", MIXW_FILE_VERSION);
        if (doCheckSum) {
            props.setProperty("chksum0", checksum);
        }

        DataOutputStream dos = writeS3BinaryHeader(location, path, props, append);

        int numStates = pool.getFeature(NUM_SENONES, -1);
        int numStreams = pool.getFeature(NUM_STREAMS, -1);
        int numGaussiansPerState = pool.getFeature(NUM_GAUSSIANS_PER_STATE, -1);

        writeInt(dos, numStates);
        writeInt(dos, numStreams);
        writeInt(dos, numGaussiansPerState);

        assert numStreams == 1;

        int rawLength = numGaussiansPerState * numStates * numStreams;
        writeInt(dos, rawLength);

        for (int i = 0; i < numStates; i++) {
            float[] mixtureWeight = new float[numGaussiansPerState];
            float[] logMixtureWeight = pool.get(i);
            logMath.logToLinear(logMixtureWeight, mixtureWeight);

            writeFloatArray(dos, mixtureWeight);
        }
        if (doCheckSum) {
            assert doCheckSum = false : "Checksum not supported";
            // writeInt(dos, checkSum);
        }

        dos.close();
    }

    /**
     * Saves the transition matrices
     *
     * @param pool   the transition matrices pool
     * @param path   the path to the transitions matrices
     * @param append is true, the file will be appended, useful if saving to a ZIP or JAR file
     * @throws FileNotFoundException if a file cannot be found
     * @throws IOException           if an error occurs while saving the data
     */
    protected void saveTransitionMatricesAscii(Pool<float[][]> pool, String path, boolean append)
            throws FileNotFoundException, IOException {
        OutputStream outputStream = StreamFactory.getOutputStream(location, path, append);
        if (outputStream == null) {
            throw new IOException("Error trying to write file " + location + path);
        }
        PrintWriter pw = new PrintWriter(outputStream, true);

        logger.info("Saving transition matrices to: ");
        logger.info(path);
        int numMatrices = pool.size();

        assert numMatrices > 0;
        float[][] tmat = pool.get(0);
        int numStates = tmat[0].length;

        pw.println("tmat " + numMatrices + ' ' + numStates);

        for (int i = 0; i < numMatrices; i++) {
            pw.println("tmat [" + i + ']');

            tmat = pool.get(i);
            for (int j = 0; j < numStates; j++) {
                for (int k = 0; k < numStates; k++) {

                    // the last row is just zeros, so we just do
                    // the first (numStates - 1) rows

                    if (j < numStates - 1) {
                        if (sparseForm) {
                            if (k < j) {
                                pw.print("\t");
                            }
                            if (k == j || k == j + 1) {
                                pw.print((float)
                                        logMath.logToLinear(tmat[j][k]));
                            }
                        } else {
                            pw.print((float) logMath.logToLinear(tmat[j][k]));
                        }
                        if (numStates - 1 == k) {
                            pw.println();
                        } else {
                            pw.print(" ");
                        }

                    }

                    if (logger.isLoggable(Level.FINE)) {
                        logger.fine("tmat j " + j + " k "
                                + k + " tm " + tmat[j][k]);
                    }
                }
            }
        }
        outputStream.close();
    }

    /**
     * Saves the transition matrices (Binary)
     *
     * @param pool   the transition matrices pool
     * @param path   the path to the transitions matrices
     * @param append is true, the file will be appended, useful if saving to a ZIP or JAR file
     * @return a pool of transition matrices
     * @throws FileNotFoundException if a file cannot be found
     * @throws IOException           if an error occurs while saving the data
     */
    protected void saveTransitionMatricesBinary(Pool<float[][]> pool, String path, boolean append)
            throws IOException {

        logger.info("Saving transition matrices to: ");
        logger.info(path);
        Properties props = new Properties();

        props.setProperty("version", TMAT_FILE_VERSION);
        if (doCheckSum) {
            props.setProperty("chksum0", checksum);
        }

        DataOutputStream dos = writeS3BinaryHeader(location, path, props, append);

        int numMatrices = pool.size();
        assert numMatrices > 0;
        writeInt(dos, numMatrices);

        float[][] tmat = pool.get(0);
        int numStates = tmat[0].length;
        int numRows = numStates - 1;

        writeInt(dos, numRows);
        writeInt(dos, numStates);
        int numValues = numStates * numRows * numMatrices;
        writeInt(dos, numValues);

        for (int i = 0; i < numMatrices; i++) {
            tmat = pool.get(i);

            // Last row should be all zeroes
            float[] logTmatRow = tmat[numStates - 1];
            float[] tmatRow = new float[logTmatRow.length];

            for (int j = 0; j < numStates; j++) {
                assert tmatRow[j] == 0.0f;
            }

            for (int j = 0; j < numRows; j++) {
                logTmatRow = tmat[j];
                tmatRow = new float[logTmatRow.length];
                logMath.logToLinear(logTmatRow, tmatRow);
                writeFloatArray(dos, tmatRow);
            }
        }
        if (doCheckSum) {
            assert doCheckSum = false : "Checksum not supported";
            // writeInt(dos, checkSum);
        }
        dos.close();
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
        logger.info("Sphinx3Saver");
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
}

package edu.cmu.sphinx.frontend;

import java.util.Random;
import java.util.logging.Logger;

import org.testng.Assert;
import org.testng.annotations.Test;

import edu.cmu.sphinx.frontend.BaseDataProcessor;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DataProcessingException;
import edu.cmu.sphinx.frontend.DataStartSignal;
import edu.cmu.sphinx.frontend.DoubleData;
import edu.cmu.sphinx.frontend.FloatData;
import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Boolean;
import edu.cmu.sphinx.util.props.S4Double;
import edu.cmu.sphinx.util.props.S4Integer;

/**
 * A DataProcessor which inserts short speech snippets with variable length into a speech stream. The snippets are takes
 * from the local neighborhood of the insertion point. The insertion behavior satisfies a uniform distribution with a
 * user defined width. Additionally a insertion of can be enforced at the beginning of the stream.
 *
 * The purpose of this component is to randomize data to blow a small set of data up to a larger one.
 * <p/>
 * This component should be put into the processing chain AFTER the SpeechClassifier or BEFORE a DataBlocker. The reason
 * for that is, that the current implementation generates blocks with different lengths which would affect the results
 * of the SpeechClassifier.
 *
 * @author Holger Brandl
 */
public class RandomSampleRepeater extends BaseDataProcessor {

    /** The maximal value which could be added to the signal */
    @S4Boolean(defaultValue = false)
    public static final String PROP_RAND_STREAM_START = "forceRandStreamStart";
    private boolean randStreamStart;


    /**
     * The maximal number of milliseconds which can be repeated using this randomizer. This number has to smaller than
     * the length of the incoming data-blocks, because currently this class does not support data-buffering.
     */
    @S4Double(defaultValue = 0.0)
    public static final String PROP_MAX_REAPTED_MS = "maxRepeatedMs";
    private double maxRepeatedMs;
    private long maxRepeatedSamples = -1;


    /**
     * The property about the width of the uniform distribution which determines the distance between
     * different repeated-sample-insertion-points.
     */
    @S4Double(defaultValue = 100.0)
    public static final String PROP_UNIFORM_DIST_WIDTH = "uDistWidthSec";
    private double uDistWidthSec;
    private long uDistWidthSamples = -1;


    /**
     * The property about using random seed or not for the randomization process. if not the stream
     * will be randomized every time in the same manner.
     */
    @S4Boolean(defaultValue = true)
    public static final String PROP_USE_RANDSEED = "useRandSeed";
    private boolean useRandSeed;


    /** The property for the sample rate. */
    @S4Integer(defaultValue = 16000)
    public static final String PROP_SAMPLE_RATE = "sampleRate";
    private int sampleRate;


    /** The random generator used to compute the insertion points. */
    private Random r;
    private Logger logger;

    private long nextInsertionPoint = 0;
    private long numInsertedSamples = 0;


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.frontend.DataProcessor#initialize(edu.cmu.sphinx.frontend.CommonConfig)
    */
    @Override
    public void initialize() {
        super.initialize();

        if (useRandSeed)
            r = new Random();
        else
            r = new Random(12345);

        uDistWidthSamples = RandomDataProcessor.ms2samples((int) (uDistWidthSec * 1000), sampleRate);
        maxRepeatedSamples = RandomDataProcessor.ms2samples((int) (maxRepeatedMs), sampleRate);
    }


    private long computeNextInsertionPoint() {
        // compute the next number in samples

        int stepWidth = r.nextInt((int) uDistWidthSamples);
        return stepWidth + nextInsertionPoint;
    }


    public void prepareForNewStream() {
        if (randStreamStart)
            nextInsertionPoint = 0;
        else
            nextInsertionPoint = computeNextInsertionPoint();

        // reset the number of inserted samples
        numInsertedSamples = 0;
    }


    /**
     * Returns the next DoubleData object, which is a dithered version of the input
     *
     * @return the next available DoubleData object, or null if no Data is available
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     *          if a data processing error occurred
     */
    @Override
    public Data getData() throws DataProcessingException {
        Data input = getPredecessor().getData(); // get the spectrum

        if (input instanceof DataStartSignal)
            prepareForNewStream();

        getTimer().start();

        if (input != null && maxRepeatedMs != 0) {
            if (input instanceof DoubleData || input instanceof FloatData) {
                input = process(input);
            }
        }

        getTimer().stop();
        return input;
    }


    /**
     * Process data, adding dither
     *
     * @param input a MelSpectrum frame
     * @return a mel Cepstrum frame
     */
    private DoubleData process(Data input) throws IllegalArgumentException {
        assert input instanceof DoubleData;

        double[] inFeatures;

        DoubleData doubleData = (DoubleData) input;
        assert sampleRate == doubleData.getSampleRate();

        inFeatures = doubleData.getValues();

        // check wether we reached the next insertion point and repeat some parts of the input part if necessary
        double[] extFeatures;
        long firstSampleNumber = doubleData.getFirstSampleNumber() + numInsertedSamples;
        long lastSampleNumber = doubleData.getFirstSampleNumber() + doubleData.getValues().length - 1;

        if (nextInsertionPoint >= doubleData.getFirstSampleNumber() && nextInsertionPoint <= lastSampleNumber) {
            int insertLength = Math.min(r.nextInt((int) maxRepeatedSamples) + 1, inFeatures.length);

            // make sure that after insertion the block-length does not exceed 160 samples because with more SpeechClassifierNT will fail
            assert doubleData.getValues().length + insertLength <= 160 : "block too large for next SpeechClassifier";


            extFeatures = new double[insertLength + inFeatures.length];

            logger.fine("RSR: repeat snippet with length " + insertLength + " at position " + nextInsertionPoint);

            // copy the existing block into the new array and replicate the desired snippet inbetween
            int startIndex = (int) (nextInsertionPoint - doubleData.getFirstSampleNumber());

            System.arraycopy(inFeatures, 0, extFeatures, 0, startIndex);
            System.arraycopy(inFeatures, 0, extFeatures, startIndex, insertLength);
            System.arraycopy(inFeatures, startIndex, extFeatures, startIndex + insertLength, inFeatures.length - startIndex);
            numInsertedSamples += insertLength;
            nextInsertionPoint = computeNextInsertionPoint();
        } else {
            extFeatures = inFeatures;
        }


        DoubleData extendedData = new DoubleData(extFeatures, doubleData.getSampleRate(),
                firstSampleNumber);

        return extendedData;
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        logger = ps.getLogger();
        maxRepeatedMs = ps.getDouble(RandomSampleRepeater.PROP_MAX_REAPTED_MS);
        uDistWidthSec = ps.getDouble(RandomSampleRepeater.PROP_UNIFORM_DIST_WIDTH);
        useRandSeed = ps.getBoolean(RandomSampleRepeater.PROP_USE_RANDSEED);
        randStreamStart = ps.getBoolean(RandomSampleRepeater.PROP_RAND_STREAM_START);
        sampleRate = ps.getInt(RandomSampleRepeater.PROP_SAMPLE_RATE);

        initialize();
    }


    @Test
    public void testInsertAtZero() {
        RandomSampleRepeater rsr = ConfigurationManager.getInstance(RandomSampleRepeater.class);
        rsr.randStreamStart = true;
        rsr.useRandSeed = false;

        rsr.initialize();
        rsr.maxRepeatedSamples = 5;
        rsr.numInsertedSamples = 20;
        rsr.nextInsertionPoint = 5;

        // create a dummy DoubleData
        double[] data = new double[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        DoubleData dd = new DoubleData(data, 16000, 0);

        DoubleData extData = rsr.process(dd);

        double[] expectedData = new double[]{1, 2, 3, 4, 5, 1, 2, 6, 7, 8, 9, 10};
        Assert.assertEquals(extData.getFirstSampleNumber(), 20, 0);
        Assert.assertEquals(extData.getValues().length, expectedData.length);

        for (int i = 0; i < expectedData.length; i++) {
            Assert.assertEquals(extData.getValues()[i], expectedData[i], 0);
        }
    }


    @Test
    public void testInsertAt4() {
        RandomSampleRepeater rsr = ConfigurationManager.getInstance(RandomSampleRepeater.class);
        rsr.randStreamStart = true;
        rsr.useRandSeed = false;

        rsr.initialize();
        rsr.maxRepeatedSamples = 3;
        rsr.numInsertedSamples = 20;
        rsr.nextInsertionPoint = 5;

        // create a dummy DoubleData
        double[] data = new double[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        DoubleData dd = new DoubleData(data, 16000, 0);

        DoubleData extData = rsr.process(dd);

        double[] expectedData = new double[]{1, 2, 3, 4, 5, 1, 2, 6, 7, 8, 9, 10};
        Assert.assertEquals(extData.getFirstSampleNumber(), 20, 0);
        Assert.assertEquals(extData.getValues().length, expectedData.length);

        for (int i = 0; i < expectedData.length; i++) {
            Assert.assertEquals(extData.getValues()[i], expectedData[i], 0);
        }
    }


    @Test
    public void testInsertAtBlockEnd() {
        RandomSampleRepeater rsr = ConfigurationManager.getInstance(RandomSampleRepeater.class);
        rsr.randStreamStart = true;
        rsr.useRandSeed = false;

        rsr.initialize();
        rsr.maxRepeatedSamples = 3;
        rsr.numInsertedSamples = 20;
        rsr.nextInsertionPoint = 9;

        // create a dummy DoubleData
        double[] data = new double[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        DoubleData dd = new DoubleData(data, 16000, 0);

        DoubleData extData = rsr.process(dd);

        double[] expectedData = new double[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 10};
        Assert.assertEquals(extData.getFirstSampleNumber(), 20, 0);
        Assert.assertEquals(extData.getValues().length, expectedData.length);

        for (int i = 0; i < expectedData.length; i++) {
            Assert.assertEquals(extData.getValues()[i], expectedData[i], 0);
        }
    }


}

package edu.cmu.sphinx.frontend;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import org.testng.annotations.BeforeMethod;

import edu.cmu.sphinx.frontend.BaseDataProcessor;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DataProcessingException;
import edu.cmu.sphinx.frontend.DoubleData;

/**
 * A DataProcessor implemenation which can be used to setup simple unit-tests for other DataProcessors. Addtionally some
 * static utility methods which should ease unit-testing of DataProcessors are provided by this class.
 *
 * @author Holger Brandl
 */
public abstract class RandomDataProcessor extends BaseDataProcessor {

    public static Random r = new Random(123);

    protected List<Data> input;


    @BeforeMethod
    public void setUp() {
        input = new ArrayList<Data>();
    }


    @Override
    public Data getData() throws DataProcessingException {
        return input.isEmpty() ? null : input.remove(0);
    }


    public List<Data> collectOutput(BaseDataProcessor dataProc) throws DataProcessingException {
        dataProc.setPredecessor(this);

        List<Data> output = new ArrayList<Data>();

        Data d;
        while ((d = dataProc.getData()) != null) {
            output.add(d);
        }

        return output;
    }


    public static List<DoubleData> createFeatVectors(double lengthSec, int sampleRate, long startSample, int featDim, double shiftMs) {
        int numFrames = (int) Math.ceil((lengthSec * 1000) / shiftMs);
        List<DoubleData> datas = new ArrayList<DoubleData>(numFrames);

        long curStartSample = startSample;
        long shiftSamples = ms2samples((int) shiftMs, sampleRate);
        for (int i = 0; i < numFrames; i++) {
            double[] values = createRandFeatureVector(featDim, null, null);
            datas.add(new DoubleData(values, sampleRate, curStartSample));

            curStartSample += shiftSamples;
        }

        return datas;
    }


    public static double[] createRandFeatureVector(int featDim, double[] mean, double[] variance) {
        if (mean == null) {
            mean = new double[featDim];
        }

        if (variance == null) {
            variance = new double[featDim];
            for (int i = 0; i < variance.length; i++) {
                variance[i] = 1;
            }
        }

        assert featDim == mean.length && featDim == variance.length;

        double[] updBlock = new double[featDim];

        for (int i = 0; i < updBlock.length; i++) {
            updBlock[i] = mean[i] + variance[i] * r.nextDouble(); // *10 to get better debuggable (sprich: merkbarer) values
        }

        return updBlock;
    }


    public static long ms2samples(double ms, int sampleRate) {
        return Math.round(sampleRate * ms / 1000);
    }
}

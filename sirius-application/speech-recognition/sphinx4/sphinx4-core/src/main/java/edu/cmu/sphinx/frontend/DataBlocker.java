package edu.cmu.sphinx.frontend;

import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Double;

import java.util.LinkedList;

/**
 * A <code>DataProcessor</code> which wraps incoming <code>DoubleData</code>-objects into equally size blocks of defined
 * length.
 */
public class DataBlocker extends BaseDataProcessor {

    /** The property for the block size of generated data-blocks in milliseconds. */
    @S4Double(defaultValue = 10)
    public static final String PROP_BLOCK_SIZE_MS = "blockSizeMs";

    private double blockSizeMs;
    private int blockSizeSamples = Integer.MAX_VALUE;

    private int curFirstSamplePos;
    private int sampleRate = -1;

    private final LinkedList<DoubleData> inBuffer = new LinkedList<DoubleData>();

    private int curInBufferSize;


    public DataBlocker() {
    }

    /**
     * 
     * @param blockSizeMs
     */
    public DataBlocker(double blockSizeMs) {
        initLogger();
        this.blockSizeMs = blockSizeMs;
    }

    @Override
    public void newProperties(PropertySheet propertySheet) throws PropertyException {
        super.newProperties(propertySheet);
        blockSizeMs = propertySheet.getDouble(PROP_BLOCK_SIZE_MS);
    }


    public double getBlockSizeMs() {
        return blockSizeMs;
    }


    @Override
    public Data getData() throws DataProcessingException {
        while (curInBufferSize < blockSizeSamples || curInBufferSize == 0) {
            Data data = getPredecessor().getData();

            if (data instanceof DataStartSignal) {
                sampleRate = ((DataStartSignal) data).getSampleRate();
                blockSizeSamples = (int) Math.round(sampleRate * blockSizeMs / 1000);

                curInBufferSize = 0;
                curFirstSamplePos = 0;
                
                inBuffer.clear();
            }

            if (!(data instanceof DoubleData)) {
                return data;
            }

            DoubleData dd = (DoubleData) data;

            inBuffer.add(dd);
            curInBufferSize += dd.getValues().length;
        }

        // now we are ready to merge all data blocks into one
        double[] newSampleBlock = new double[blockSizeSamples];

        int copiedSamples = 0;

        long firstSample = inBuffer.get(0).getFirstSampleNumber() + curFirstSamplePos;

        while (!inBuffer.isEmpty()) {
            DoubleData dd = inBuffer.remove(0);
            double[] values = dd.getValues();
            int copyLength = Math.min(blockSizeSamples - copiedSamples, values.length - curFirstSamplePos);

            System.arraycopy(values, curFirstSamplePos, newSampleBlock, copiedSamples, copyLength);

            // does the current data-object contains more samples than necessary? -> keep the rest for the next block
            if (copyLength < (values.length - curFirstSamplePos)) {
                assert inBuffer.isEmpty();

                curFirstSamplePos += copyLength;
                inBuffer.add(0, dd);
                break;
            } else {
                copiedSamples += copyLength;
                curFirstSamplePos = 0;
            }
        }

        curInBufferSize = inBuffer.isEmpty() ? 0 : inBuffer.get(0).getValues().length - curFirstSamplePos;

//        for (int i = 0; i < newSampleBlock.length; i++) {
//            newSampleBlock[i] *= 10;
//        }
        return new DoubleData(newSampleBlock, sampleRate, firstSample);
    }

}

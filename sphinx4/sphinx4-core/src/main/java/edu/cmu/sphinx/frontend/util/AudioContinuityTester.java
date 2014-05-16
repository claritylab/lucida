package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.frontend.*;

/**
 * {@code FrontEnd} element that asserts the audio-stream to be continuous. This is often a mandatory property for
 * frontend setups. The component operates on the acoustic data level and needs to plugged into the frontend
 * before the actual feature extraction starts.
 * <p/>
 * This component can help to debug new VAD implementations, where it has been shown that data-blocks easily get lost.
 *
 * @author Holger Brandl
 */
public class AudioContinuityTester extends BaseDataProcessor {

    long lastSampleNum = -1;

    public AudioContinuityTester() {
        initLogger();
    }

    @Override
    public Data getData() throws DataProcessingException {
        Data d = getPredecessor().getData();

        assert isAudioStreamContinuous(d) : "audio stream is not continuous";

        return d;
    }


    private boolean isAudioStreamContinuous(Data input) {
        if (input instanceof DoubleData) {
            DoubleData d = (DoubleData) input;
            if (lastSampleNum != -1 && lastSampleNum != d.getFirstSampleNumber()) {
                return false;
            }

            lastSampleNum = d.getFirstSampleNumber() + d.getValues().length;

        } else if (input instanceof DataStartSignal)
            lastSampleNum = -1;

        return true;
    }
}

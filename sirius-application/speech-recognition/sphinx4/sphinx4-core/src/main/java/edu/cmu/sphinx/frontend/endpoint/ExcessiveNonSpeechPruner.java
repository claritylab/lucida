package edu.cmu.sphinx.frontend.endpoint;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.util.props.*;

/**
 * Removes excessive non-speech-segments from a speech stream. Compared with <code>NonSpeechDatatFilter</code> this
 * component does not remove all non-speech frames. It just reduces the non-speech parts to a user defined length.
 *
 * @see SpeechMarker
 * @see NonSpeechDataFilter
 */
public class ExcessiveNonSpeechPruner extends BaseDataProcessor {

    /**
     * The property for the maximum amount of (subsequent) none-speech time (in ms) to be preserved in the speech
     * stream.
     */
    @S4Integer(defaultValue = Integer.MAX_VALUE)
    public static final String PROP_MAX_NON_SPEECH_TIME_MS = "maxNonSpeechTimeMs";

    private int maxNonSpeechTime;
    private boolean inSpeech;
    private int nonSpeechCounter;


    public ExcessiveNonSpeechPruner(int maxNonSpeechTime) {
        this.maxNonSpeechTime = maxNonSpeechTime;
    }

    public ExcessiveNonSpeechPruner() {
    }
      
    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        maxNonSpeechTime = ps.getInt(PROP_MAX_NON_SPEECH_TIME_MS);
    }

    /**
     * Returns the processed Data output.
     *
     * @return an Data object that has been processed by this DataProcessor
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     *          if a data processor error occurs
     */
    @Override
    public Data getData() throws DataProcessingException {
        Data data = getPredecessor().getData();

        if (data instanceof SpeechEndSignal || data instanceof DataStartSignal) {
            inSpeech = false;
            nonSpeechCounter = 0;
        } else if (data instanceof SpeechStartSignal) {
            inSpeech = true;
        } else if (data instanceof DoubleData || data instanceof FloatData) {
            if (!inSpeech) {
                nonSpeechCounter += getAudioTime(data);
                if (nonSpeechCounter >= maxNonSpeechTime)
                    data = getData();
            }
        }

        return data;
    }


    /**
     * Returns the amount of audio data in milliseconds in the given SpeechClassifiedData object.
     *
     * @param data the SpeechClassifiedData object
     * @return the amount of audio data in milliseconds
     */
    public int getAudioTime(Data data) {
        if (data instanceof DoubleData) {
            DoubleData audio = (DoubleData) data;
            return (int) ((audio.getValues().length * 1000.0f / audio.getSampleRate()));
        } else if (data instanceof FloatData) {
            FloatData audio = (FloatData) data;
            return (int) ((audio.getValues().length * 1000.0f / audio.getSampleRate()));
        }

        assert false;
        return -1;
    }

}

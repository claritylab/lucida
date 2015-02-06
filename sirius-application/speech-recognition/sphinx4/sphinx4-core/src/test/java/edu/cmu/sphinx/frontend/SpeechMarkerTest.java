package edu.cmu.sphinx.frontend;

import static edu.cmu.sphinx.util.props.ConfigurationManager.getInstance;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.instanceOf;

import java.util.ArrayList;
import java.util.List;

import org.hamcrest.Matchers;
import org.testng.annotations.Test;

import edu.cmu.sphinx.frontend.endpoint.*;
import edu.cmu.sphinx.util.props.PropertyException;


/**
 * DOCUMENT ME!
 * 
 * @author Holger Brandl
 */
public class SpeechMarkerTest extends RandomDataProcessor {

    public BaseDataProcessor createDataFilter(boolean mergeSpeechSegments) {
        try {
            SpeechMarker speechMarker = getInstance(SpeechMarker.class);
            speechMarker.initialize();

            return speechMarker;

        } catch (PropertyException e) {
            e.printStackTrace();
        }

        return null;
    }

    /**
     * Test whether the speech marker is able to handle cases in which an
     * DataEndSignal occurs somewhere after a SpeechStartSignal. This is might
     * occur if the microphone is stopped while someone is speaking.
     */
    @Test
    public void testEndWithoutSilence() throws DataProcessingException {
        int sampleRate = 1000;
        input.add(new DataStartSignal(sampleRate));
        input.addAll(createClassifiedSpeech(sampleRate, 2., true));
        input.add(new DataEndSignal(-2));

        List<Data> results = collectOutput(createDataFilter(false));

        assertThat(results, Matchers.hasSize(104));
        assertThat(results.get(0), instanceOf(DataStartSignal.class));
        assertThat(results.get(1), instanceOf(SpeechStartSignal.class));
        assertThat(results.get(102), instanceOf(SpeechEndSignal.class));
        assertThat(results.get(103), instanceOf(DataEndSignal.class));
    }

    private List<SpeechClassifiedData> createClassifiedSpeech(int sampleRate,
                                                              double lengthSec,
                                                              boolean isSpeech) {
        List<SpeechClassifiedData> datas = new ArrayList<SpeechClassifiedData>();
        List<DoubleData> featVecs = createFeatVectors(1, sampleRate, 0, 10, 10);

        for (DoubleData featVec : featVecs)
            datas.add(new SpeechClassifiedData(featVec, isSpeech));

        return datas;
    }
}

package edu.cmu.sphinx.frontend;

import java.util.List;

import org.testng.Assert;
import org.testng.annotations.Test;

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DataEndSignal;
import edu.cmu.sphinx.frontend.DataProcessingException;
import edu.cmu.sphinx.frontend.DataStartSignal;
import edu.cmu.sphinx.frontend.endpoint.NonSpeechDataFilter;
import edu.cmu.sphinx.frontend.endpoint.SpeechEndSignal;
import edu.cmu.sphinx.frontend.endpoint.SpeechStartSignal;
import edu.cmu.sphinx.util.props.ConfigurationManager;

/**
 * Some tests to ensure that the NonSpeechDataFilter filters non-speech in the specified manner.
 *
 * @author Holger Brandl
 */
public class NonSpeechDataFilterTest extends RandomDataProcessor {


    @Test
    public void testOneSpeechRegion() throws DataProcessingException {
        int sampleRate = 1000;

        input.add(new DataStartSignal(sampleRate));

        input.addAll(createFeatVectors(1, sampleRate, 0, 10, 10)); // create one second of data sampled with 1kHz
        input.add(new SpeechStartSignal(-1));
        input.addAll(createFeatVectors(0.5, sampleRate, 0, 10, 10));
        input.add(new SpeechEndSignal(-1));
        input.addAll(createFeatVectors(1, sampleRate, 0, 10, 10));

        input.add(new DataEndSignal(0));

        List<Data> result = collectOutput(ConfigurationManager.getInstance(NonSpeechDataFilter.class));

        Assert.assertTrue(result.size() == 54);
        Assert.assertTrue(result.get(0) instanceof DataStartSignal);
        Assert.assertTrue(result.get(1) instanceof SpeechStartSignal);
        Assert.assertTrue(result.get(52) instanceof SpeechEndSignal);
        Assert.assertTrue(result.get(53) instanceof DataEndSignal);
    }


    @Test
    public void testMultipleSpeechRegionWithoutMerging() throws DataProcessingException {
        int sampleRate = 1000;

        input.add(new DataStartSignal(sampleRate));

        input.addAll(createFeatVectors(0.1, sampleRate, 0, 10, 10)); // create one second of data sampled with 1kHz

        input.add(new SpeechStartSignal(-1));
        input.addAll(createFeatVectors(0.1, sampleRate, 0, 10, 10));
        input.add(new SpeechEndSignal(-1));

        input.addAll(createFeatVectors(0.1, sampleRate, 0, 10, 10));

        input.add(new SpeechStartSignal(-1));
        input.addAll(createFeatVectors(0.1, sampleRate, 0, 10, 10));
        input.add(new SpeechEndSignal(-1));

        input.addAll(createFeatVectors(0.1, sampleRate, 0, 10, 10));

        input.add(new DataEndSignal(0));

        NonSpeechDataFilter nonSpeechDataFilter = ConfigurationManager.getInstance(NonSpeechDataFilter.class);
        List<Data> result = collectOutput(nonSpeechDataFilter);

        Assert.assertTrue(result.size() == 26);

        Assert.assertTrue(result.get(0) instanceof DataStartSignal);
        Assert.assertTrue(result.get(1) instanceof SpeechStartSignal);

        Assert.assertTrue(result.get(12) instanceof SpeechEndSignal);
        Assert.assertTrue(result.get(13) instanceof SpeechStartSignal);

        Assert.assertTrue(result.get(24) instanceof SpeechEndSignal);
        Assert.assertTrue(result.get(25) instanceof DataEndSignal);
    }


    @Test
    public void testMultipleEmptyAndNonemptySegments() throws DataProcessingException {
        int sampleRate = 1000;

        // process an empty segment
        input.add(new DataStartSignal(sampleRate));
        input.add(new DataEndSignal(sampleRate));

        // process a segment which contains a speech segment which is empty
        input.add(new DataStartSignal(sampleRate));
        input.add(new SpeechStartSignal(-1));
        input.add(new SpeechEndSignal(-1));
        input.add(new DataEndSignal(sampleRate));

        // process a segment which contains a speech segment which is empty but has some non-speech data around it
        input.add(new DataStartSignal(sampleRate));
        input.addAll(createFeatVectors(1, sampleRate, 0, 10, 10)); // create one second of data sampled with 1kHz
        input.add(new SpeechStartSignal(-1));
        input.add(new SpeechEndSignal(-1));
        input.addAll(createFeatVectors(1, sampleRate, 0, 10, 10)); // create one second of data sampled with 1kHz
        input.add(new DataEndSignal(sampleRate));

        // and now a some real segments
        input.add(new DataStartSignal(sampleRate));
        input.add(new SpeechStartSignal(-1));
        input.addAll(createFeatVectors(0.1, sampleRate, 0, 10, 10));
        input.add(new SpeechEndSignal(-1));
        input.add(new DataEndSignal(sampleRate));


        List<Data> result = collectOutput(ConfigurationManager.getInstance(NonSpeechDataFilter.class));

        Assert.assertTrue(result.size() == 24);

        Assert.assertTrue(result.get(0) instanceof DataStartSignal);
        Assert.assertTrue(result.get(1) instanceof DataEndSignal);

        Assert.assertTrue(result.get(2) instanceof DataStartSignal);
        Assert.assertTrue(result.get(3) instanceof SpeechStartSignal);
        Assert.assertTrue(result.get(4) instanceof SpeechEndSignal);
        Assert.assertTrue(result.get(5) instanceof DataEndSignal);

        Assert.assertTrue(result.get(6) instanceof DataStartSignal);
        Assert.assertTrue(result.get(7) instanceof SpeechStartSignal);

        Assert.assertTrue(result.get(22) instanceof SpeechEndSignal);
        Assert.assertTrue(result.get(23) instanceof DataEndSignal);
    }
}

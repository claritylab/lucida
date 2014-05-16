package edu.cmu.sphinx.decoder.scorer;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.testng.Assert;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.databranch.DataBufferProcessor;
import edu.cmu.sphinx.frontend.endpoint.SpeechEndSignal;
import edu.cmu.sphinx.frontend.endpoint.SpeechStartSignal;
import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.ConfigurationManagerUtils;

/**
 * Unit tests for the processing logic of the scorer implementations
 *
 * @author Holger Brandl
 */
public class ScorerTests {

    @SuppressWarnings("serial")
    Scoreable testToken = new Token(null, 0.f, 0.f, 0.f, 0.f) {

        @Override
        public float calculateScore(Data feature) {
            return -1;
        }
    };


    @BeforeClass
    public static void configureLogger() {
        Logger.getLogger(ScorerTests.class.getSimpleName()).setLevel(Level.FINER);
    }


    @Test
    public void waitUntilSpeechStart() {
        List<Class<? extends SimpleAcousticScorer>> scorerClasses = new ArrayList<Class<? extends SimpleAcousticScorer>>();
        scorerClasses.add(SimpleAcousticScorer.class);
        scorerClasses.add(ThreadedAcousticScorer.class);

        for (Class<? extends SimpleAcousticScorer> scorerClass : scorerClasses) {
            System.err.println("testing: " + scorerClass.getSimpleName());
            DataBufferProcessor dummyFrontEnd = createDummyFrontEnd();

            Map<String, Object> props = new HashMap<String, Object>();
            props.put(SimpleAcousticScorer.FEATURE_FRONTEND, dummyFrontEnd);
            AcousticScorer scorer = ConfigurationManager.getInstance(scorerClass, props);

            int startBufferSize = dummyFrontEnd.getBufferSize();

            scorer.allocate();
            scorer.startRecognition();

//          Assert.assertTrue(dummyFrontEnd.getBufferSize() < (startBufferSize - 100));

            List<Scoreable> dummyTokens = Arrays.asList(testToken);

            scorer.calculateScores(dummyTokens);
            Assert.assertTrue(dummyFrontEnd.getBufferSize() < (startBufferSize - 100));

            scorer.stopRecognition();
            scorer.deallocate();
        }
    }


    private DataBufferProcessor createDummyFrontEnd() {
        DataBufferProcessor bufferProc = ConfigurationManager.getInstance(DataBufferProcessor.class);
        bufferProc.processDataFrame(new DataStartSignal(16000, true));

        for (DoubleData doubleData : RandomDataProcessor.createFeatVectors(5, 16000, 0, 39, 10))
            bufferProc.processDataFrame(doubleData);

        bufferProc.processDataFrame(new SpeechStartSignal());
        for (DoubleData doubleData : RandomDataProcessor.createFeatVectors(3, 16000, 1000, 39, 10))
            bufferProc.processDataFrame(doubleData);

        bufferProc.processDataFrame(new SpeechEndSignal());
        for (DoubleData doubleData : RandomDataProcessor.createFeatVectors(5, 16000, 2000, 39, 10))
            bufferProc.processDataFrame(doubleData);

        bufferProc.processDataFrame(new DataEndSignal(123));

        return bufferProc;
    }


    @Test
    public void testThreadedScorerDeallocation() throws InterruptedException {
        Map<String, Object> props = new HashMap<String, Object>();
        DataBufferProcessor dummyFrontEnd = createDummyFrontEnd();

        props.put(SimpleAcousticScorer.FEATURE_FRONTEND, dummyFrontEnd);
        props.put(ThreadedAcousticScorer.PROP_NUM_THREADS, 4);
        props.put(ConfigurationManagerUtils.GLOBAL_COMMON_LOGLEVEL, "FINEST");
        AcousticScorer scorer = ConfigurationManager.getInstance(ThreadedAcousticScorer.class, props);

        scorer.allocate();
        scorer.startRecognition();

        List<Scoreable> dummyTokens = Arrays.asList(testToken);

        // score around a little
        scorer.calculateScores(dummyTokens);

        scorer.stopRecognition();
        scorer.deallocate();

        Thread.sleep(1000);
        
        // ensure that all scoring threads have died
    }
}

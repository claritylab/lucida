package edu.cmu.sphinx.decoder.scorer;

import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.endpoint.SpeechEndSignal;
import edu.cmu.sphinx.frontend.endpoint.SpeechStartSignal;
import edu.cmu.sphinx.frontend.util.DataUtil;
import edu.cmu.sphinx.util.props.ConfigurableAdapter;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Implements some basic scorer functionality, including a simple default
 * acoustic scoring implementation which scores within the current thread,
 * that can be changed by overriding the {@link #doScoring} method.
 *
 * <p>Note that all scores are maintained in LogMath log base.
 *
 * @author Holger Brandl
 */
public class SimpleAcousticScorer extends ConfigurableAdapter implements AcousticScorer {

    /** Property the defines the frontend to retrieve features from for scoring */
    @S4Component(type = BaseDataProcessor.class)
    public final static String FEATURE_FRONTEND = "frontend";
    protected BaseDataProcessor frontEnd;

    /**
     * An optional post-processor for computed scores that will normalize scores. If not set, no normalization will
     * applied and the token scores will be returned unchanged.
     */
    @S4Component(type = ScoreNormalizer.class, mandatory = false)
    public final static String SCORE_NORMALIZER = "scoreNormalizer";
    private ScoreNormalizer scoreNormalizer;

    private Boolean useSpeechSignals;

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        this.frontEnd = (BaseDataProcessor) ps.getComponent(FEATURE_FRONTEND);
        this.scoreNormalizer = (ScoreNormalizer) ps.getComponent(SCORE_NORMALIZER);
    }

    /**
     * @param frontEnd the frontend to retrieve features from for scoring
     * @param scoreNormalizer optional post-processor for computed scores that will normalize scores. If not set, no normalization will
     * applied and the token scores will be returned unchanged.
     */
    public SimpleAcousticScorer(BaseDataProcessor frontEnd, ScoreNormalizer scoreNormalizer) {
        initLogger();
        this.frontEnd = frontEnd;
        this.scoreNormalizer = scoreNormalizer;
    }

    public SimpleAcousticScorer() {
    }

    /**
     * Scores the given set of states.
     *
     * @param scoreableList A list containing scoreable objects to be scored
     * @return The best scoring scoreable, or <code>null</code> if there are no more features to score
     */
    public Data calculateScores(List<? extends Scoreable> scoreableList) {
    	try {
            Data data;
            while ((data = getNextData()) instanceof Signal) {
                if (data instanceof SpeechEndSignal || data instanceof DataEndSignal) {
                    return data;
                }
            }

            if (data == null || scoreableList.isEmpty())
            	return null;
            
            // convert the data to FloatData if not yet done
            if (data instanceof DoubleData)
                data = DataUtil.DoubleData2FloatData((DoubleData) data);

            Scoreable bestToken = doScoring(scoreableList, data);

            // apply optional score normalization
            if (scoreNormalizer != null && bestToken instanceof Token)
                bestToken = scoreNormalizer.normalize(scoreableList, bestToken);

            return bestToken;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    private Data getNextData() {
        Data data = frontEnd.getData();

        // reconfigure the scorer for the coming data stream
        if (data instanceof DataStartSignal)
            handleDataStartSignal((DataStartSignal)data);
        if (data instanceof DataEndSignal)
            handleDataEndSignal((DataEndSignal)data);

        return data;
    }

    /** Handles the first element in a feature-stream.
     * @param dataStartSignal*/
    protected void handleDataStartSignal(DataStartSignal dataStartSignal) {
        Map<String, Object> dataProps = dataStartSignal.getProps();

        useSpeechSignals = dataProps.containsKey(DataStartSignal.SPEECH_TAGGED_FEATURE_STREAM) && (Boolean) dataProps.get(DataStartSignal.SPEECH_TAGGED_FEATURE_STREAM);
    }

    /** Handles the last element in a feature-stream.
     * @param dataEndSignal*/
    protected void handleDataEndSignal(DataEndSignal dataEndSignal) {
        // we don't treat the end-signal here, but extending classes might do
    }

    public void startRecognition() {
        if (useSpeechSignals == null) {
            Data firstData = getNextData();
            if (firstData == null)
        	    return;
            
            assert firstData instanceof DataStartSignal :
                    "The first element in an sphinx4-feature stream must be a DataStartSignal but was a " + firstData.getClass().getSimpleName();
        }

        if (!useSpeechSignals)
            return;

        Data data;
        while (!((data = getNextData()) instanceof SpeechStartSignal)) {
            if (data == null) {
                break;
            }
        }
    }

    public void stopRecognition() {
        // nothing needs to be done here
    }

    /**
     * Scores a a list of <code>Scoreable</code>s given a <code>Data</code>-object.
     *
     * @param scoreableList The list of Scoreables to be scored
     * @param data          The <code>Data</code>-object to be used for scoring.
     * @return the best scoring <code>Scoreable</code> or <code>null</code> if the list of scoreables was empty.
     * @throws Exception 
     */
    protected <T extends Scoreable> T doScoring(List<T> scoreableList, Data data) throws Exception {
        Iterator<T> i = scoreableList.iterator();
        T best = i.next();
        best.calculateScore(data);
        while (i.hasNext()) {
            T scoreable = i.next();
            if (scoreable.calculateScore(data) > best.getScore())
                best = scoreable;
        }
        return best;
    }

    // Even if we don't do any meaningful allocation here, we implement the methods because
    // most extending scorers do need them either.
    
    public void allocate() {
    }

    public void deallocate() {
    }
}

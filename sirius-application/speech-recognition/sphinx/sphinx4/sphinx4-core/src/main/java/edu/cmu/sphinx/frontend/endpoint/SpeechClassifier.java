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


package edu.cmu.sphinx.frontend.endpoint;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Double;
import edu.cmu.sphinx.util.props.S4Integer;

import java.util.logging.Level;

/**
 * Implements a level tracking endpointer invented by Bent Schmidt Nielsen.
 * <p/>
 * <p>This endpointer is composed of three main steps. <ol> <li>classification of audio into speech and non-speech
 * <li>inserting SPEECH_START and SPEECH_END signals around speech <li>removing non-speech regions </ol>
 * <p/>
 * <p>The first step, classification of audio into speech and non-speech, uses Bent Schmidt Nielsen's algorithm. Each
 * time audio comes in, the average signal level and the background noise level are updated, using the signal level of
 * the current audio. If the average signal level is greater than the background noise level by a certain threshold
 * value (configurable), then the current audio is marked as speech. Otherwise, it is marked as non-speech.
 * <p/>
 * <p>The second and third step of this endpointer are documented in the classes {@link SpeechMarker SpeechMarker} and
 * {@link NonSpeechDataFilter NonSpeechDataFilter}.
 *
 * @see SpeechMarker
 */
public class SpeechClassifier extends AbstractVoiceActivityDetector {

    /** The property specifying the endpointing frame length in milliseconds. */
    @S4Integer(defaultValue = 10)
    public static final String PROP_FRAME_LENGTH_MS = "frameLengthInMs";

    /** The property specifying the minimum signal level used to update the background signal level. */
    @S4Double(defaultValue = 0)
    public static final String PROP_MIN_SIGNAL = "minSignal";

    /**
     * The property specifying the threshold. If the current signal level is greater than the background level by
     * this threshold, then the current signal is marked as speech. Therefore, a lower threshold will make the
     * endpointer more sensitive, that is, mark more audio as speech. A higher threshold will make the endpointer less
     * sensitive, that is, mark less audio as speech.
     */
    @S4Double(defaultValue = 10)
    public static final String PROP_THRESHOLD = "threshold";

    /** The property specifying the adjustment. */
    @S4Double(defaultValue = 0.003)
    public static final String PROP_ADJUSTMENT = "adjustment";

    protected final double averageNumber = 1;
    protected double adjustment;
    protected double level;               // average signal level
    protected double background;          // background signal level
    protected double minSignal;           // minimum valid signal level
    protected double threshold;
    protected float frameLengthSec;
    protected boolean isSpeech;

    /* Statistics */
    protected long speechFrames;
    protected long backgroundFrames;
    protected double totalBackgroundLevel;
    protected double totalSpeechLevel;
    
    public SpeechClassifier(int frameLengthMs, double adjustment, double threshold, double minSignal ) {
        initLogger();
        this.frameLengthSec = frameLengthMs / 1000.f;

        this.adjustment = adjustment;
        this.threshold = threshold;
        this.minSignal = minSignal;

        initialize();
    }

    public SpeechClassifier() {
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        int frameLengthMs = ps.getInt(PROP_FRAME_LENGTH_MS);
        frameLengthSec = frameLengthMs / 1000.f;

        adjustment = ps.getDouble(PROP_ADJUSTMENT);
        threshold = ps.getDouble(PROP_THRESHOLD);
        minSignal = ps.getDouble(PROP_MIN_SIGNAL);

        logger = ps.getLogger();
        //logger.setLevel(Level.FINEST);
 
        initialize();
    }


    /** Initializes this LevelTracker endpointer and DataProcessor predecessor. */
    @Override
    public void initialize() {
        super.initialize();
        reset();
    }


    /** Resets this LevelTracker to a starting state. */
    protected void reset() {
        level = 0;
        background = 300;
        resetStats();
    }


    /**
     * Returns the logarithm base 10 of the root mean square of the given samples.
     *
     * @param samples the samples
     * @return the calculated log root mean square in log 10
     */
    public static double logRootMeanSquare(double[] samples) {
        assert samples.length > 0;
        double sumOfSquares = 0.0f;
        for (double sample : samples) {
            sumOfSquares += sample * sample;
        }
        double rootMeanSquare = Math.sqrt
                (sumOfSquares / samples.length);
        rootMeanSquare = Math.max(rootMeanSquare, 1);
        return (LogMath.log10((float) rootMeanSquare) * 20);
    }


    /**
     * Classifies the given audio frame as speech or not, and updates the endpointing parameters.
     *
     * @param audio the audio frame
     */
    protected SpeechClassifiedData classify(DoubleData audio) {
        double current = logRootMeanSquare(audio.getValues());
        // System.out.println("current: " + current);
        isSpeech = false;
        if (current >= minSignal) {
            level = ((level * averageNumber) + current) / (averageNumber + 1);
            if (current < background) {
                background = current;
            } else {
                background += (current - background) * adjustment;
            }
            if (level < background) {
                level = background;
            }
            isSpeech = (level - background > threshold);
        }

        SpeechClassifiedData labeledAudio = new SpeechClassifiedData(audio, isSpeech);

        if (logger.isLoggable(Level.FINEST)) {
            String speech = "";
            if (labeledAudio.isSpeech())
                speech = "*";

            logger.finest("Bkg: " + background + ", level: " + level +
                    ", current: " + current + ' ' + speech);
        }

        collectStats (isSpeech);
        
        return labeledAudio;
    }

    /**
     * Reset statistics
     */
    private void resetStats () {
        backgroundFrames = 1;
        speechFrames = 1;
        totalSpeechLevel = 0;
        totalBackgroundLevel = 0;
    }
    
    /**
     * Collects the statistics to provide information about signal to noise ratio in channel
     * 
     * @param isSpeech if the current frame is classified as speech
     */
    private void collectStats(boolean isSpeech) {
        if (isSpeech) {
            totalSpeechLevel = totalSpeechLevel + level;
            speechFrames = speechFrames + 1;
        } else {
            totalBackgroundLevel = totalBackgroundLevel + background;
            backgroundFrames = backgroundFrames + 1;
        }        
    }

    /**
     * Returns the next Data object.
     *
     * @return the next Data object, or null if none available
     * @throws DataProcessingException if a data processing error occurs
     */
    @Override
    public Data getData() throws DataProcessingException {
        Data audio = getPredecessor().getData();

        if (audio instanceof DataStartSignal)
            reset();

        if (audio instanceof DoubleData) {
            DoubleData data = (DoubleData) audio;
            audio = classify(data);
        }
        return audio;
    }
    
    /**
     * Method that returns if current returned frame contains speech. 
     * It could be used by noise filter for example to adjust noise 
     * spectrum estimation.
     * 
     * @return if current frame is speech 
     */
    @Override
    public boolean isSpeech() {
    	return isSpeech;
    }
    
    /** 
     * Retrieves accumulated signal to noise ratio in dbScale 
     * 
     * @return signal to noise ratio
     */
    public double getSNR () {
        double snr = (totalBackgroundLevel / backgroundFrames - totalSpeechLevel / speechFrames);
        logger.fine ("Background " + totalBackgroundLevel / backgroundFrames);
        logger.fine ("Speech " + totalSpeechLevel / speechFrames);
        logger.fine ("SNR is " + snr);
        return snr;
    }
 
    /** 
     * Return the estimation if input data was noisy enough to break
     * recognition. The audio is counted noisy if signal to noise ratio
     * is less then -20dB.
     * 
     * @return estimation of data being noisy
     */
    public boolean getNoisy () {
        return (getSNR() > -20);
    }
}

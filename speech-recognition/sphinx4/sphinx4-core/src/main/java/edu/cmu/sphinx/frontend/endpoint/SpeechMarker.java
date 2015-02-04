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
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Double;
import edu.cmu.sphinx.util.props.S4Integer;

import java.util.ArrayList;
import java.util.List;
import java.util.ListIterator;

/**
 * Converts a stream of SpeechClassifiedData objects, marked as speech and
 * non-speech, and mark out the regions that are considered speech. This is done
 * by inserting SPEECH_START and SPEECH_END signals into the stream.
 * <p/>
 * <p>
 * The algorithm for inserting the two signals is as follows.
 * <p/>
 * <p>
 * The algorithm is always in one of two states: 'in-speech' and
 * 'out-of-speech'. If 'out-of-speech', it will read in audio until we hit audio
 * that is speech. If we have read more than 'startSpeech' amount of
 * <i>continuous</i> speech, we consider that speech has started, and insert a
 * SPEECH_START at 'speechLeader' time before speech first started. The state of
 * the algorithm changes to 'in-speech'.
 * <p/>
 * <p>
 * Now consider the case when the algorithm is in 'in-speech' state. If it read
 * an audio that is speech, it is scheduled for output. If the audio is non-speech, we read
 * ahead until we have 'endSilence' amount of <i>continuous</i> non-speech. At
 * the point we consider that speech has ended. A SPEECH_END signal is inserted
 * at 'speechTrailer' time after the first non-speech audio. The algorithm
 * returns to 'out-of-speech' state. If any speech audio is encountered
 * in-between, the accounting starts all over again.
 * 
 * While speech audio is processed delay is lowered to some minimal amount. This helps
 * to segment both slow speech with visible delays and fast speech when delays are minimal.
 */
public class SpeechMarker extends BaseDataProcessor {

    /**
     * The property for the minimum amount of time in speech (in milliseconds) to be considered
     * as utterance start.
     */
    @S4Integer(defaultValue = 200)
    public static final String PROP_START_SPEECH = "startSpeech";
    private int startSpeechTime;


    /**
     * The property for the amount of time in silence (in milliseconds) to be
     * considered as utterance end.
     */
    @S4Integer(defaultValue = 500)
    public static final String PROP_END_SILENCE = "endSilence";
    private int endSilenceTime;


    /**
     * The property for the amount of time (in milliseconds) before speech start
     * to be included as speech data.
     */
    @S4Integer(defaultValue = 50)
    public static final String PROP_SPEECH_LEADER = "speechLeader";
    private int speechLeader;

    /**
     * The property for number of frames to keep in buffer. Should be enough to let
     * insert the SpeechStartSignal.
     */
    @S4Integer(defaultValue = 30)
    public static final String PROP_SPEECH_LEADER_FRAMES = "speechLeaderFrames";
    private int speechLeaderFrames;

    /**
     * The property for the amount of time (in milliseconds) after speech ends to be
     * included as speech data.
     */
    @S4Integer(defaultValue = 50)
    public static final String PROP_SPEECH_TRAILER = "speechTrailer";
    private int speechTrailer;
    
    /**
     * The property to decrease end silence while we are reading speech. This
     * allows marker to adapt to the fast speech with small pauses. This 
     * is relative decrease to speechTrailer per second of speech, so that 
     * utterance shouldn't be longer than this amount of seconds.
     */    
    @S4Double(defaultValue = 15.0)
    public static final String PROP_END_SILENCE_DECAY = "endSilenceDecay";
    private double endSilenceDecay;

    private List<Data> outputQueue;  // Audio objects are added to the end
    private boolean inSpeech;
    private int frameCount;
    private int initialEndSilenceTime;

    public SpeechMarker(int startSpeechTime, int endSilenceTime, int speechLeader, int speechLeaderFrames, int speechTrailer, double endSilenceDecay) {
        initLogger();
        this.startSpeechTime = startSpeechTime;
        this.endSilenceTime = endSilenceTime;
        this.speechLeader = speechLeader;
        this.speechLeaderFrames = speechLeaderFrames;
        this.speechTrailer = speechTrailer;
        this.endSilenceDecay = endSilenceDecay;       
        this.initialEndSilenceTime = endSilenceTime;
    }

    public SpeechMarker() {
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        startSpeechTime = ps.getInt(PROP_START_SPEECH);
        endSilenceTime = ps.getInt(PROP_END_SILENCE);
        speechLeader = ps.getInt(PROP_SPEECH_LEADER);
        speechLeaderFrames = ps.getInt(PROP_SPEECH_LEADER_FRAMES);
        speechTrailer = ps.getInt(PROP_SPEECH_TRAILER);
        endSilenceDecay = ps.getDouble(PROP_END_SILENCE_DECAY);
        
        initialEndSilenceTime = endSilenceTime;
    }


    /**
     * Initializes this SpeechMarker
     */
    @Override
    public void initialize() {
        super.initialize();
        reset();
    }


    /**
     * Resets this SpeechMarker to a starting state.
     */
    private void reset() {
        inSpeech = false;
        frameCount = 0;
        this.outputQueue = new ArrayList<Data>();
    }


    /**
     * Returns the next Data object.
     *
     * @return the next Data object, or null if none available
     * @throws DataProcessingException if a data processing error occurs
     */
    @Override
    public Data getData() throws DataProcessingException {
        while (outputQueue.size() < speechLeaderFrames) {
            Data audio = readData();
            
            if (audio != null) {
                if (!inSpeech) {

                    if (audio instanceof SpeechClassifiedData) {
                        SpeechClassifiedData data = (SpeechClassifiedData) audio;

                        sendToQueue(audio);

                        if (data.isSpeech()) {
                            boolean speechStarted = handleFirstSpeech(data);
                            if (speechStarted) {
                                addSpeechStart();
                                inSpeech = true;
                            	startCountingFrames();
                            }
                        }
                    } else if (audio instanceof DataStartSignal) {
                        reset();
                        sendToQueue(audio);
                    } else {
                        sendToQueue(audio);
                    }
                } else {
                    if (audio instanceof SpeechClassifiedData) {
                        SpeechClassifiedData data =
                                (SpeechClassifiedData) audio;
                        sendToQueue(data);
                        if (!data.isSpeech()) {
                            inSpeech = !(readEndFrames(data));
                        } else {
                        	countSpeechFrame();
                        }
                    } else if (audio instanceof DataEndSignal) {
                        sendToQueue(new SpeechEndSignal(((Signal) audio).getTime()));
                        sendToQueue(audio);
                        inSpeech = false;
                    } else if (audio instanceof DataStartSignal) {
                        reset();
                        sendToQueue(audio);
                    }
                }
            } else {
                break;
            }
        }

        if (!outputQueue.isEmpty()) {
            Data audio = outputQueue.remove(0);
            if (audio instanceof SpeechClassifiedData) {
                SpeechClassifiedData data = (SpeechClassifiedData) audio;
                audio = data.getDoubleData();
            }

            if (audio instanceof DataStartSignal)
                DataStartSignal.tagAsVadStream((DataStartSignal) audio);

            return audio;
        } else {
            return null;
        }
    }


    private void countSpeechFrame() {
    	frameCount++;
    	int minTime = speechLeader + speechTrailer;
    	
   		endSilenceTime = (int) (initialEndSilenceTime - 
   						       ((float)initialEndSilenceTime - minTime) / endSilenceDecay * 
   						       (frameCount / 100.0));
    	
   		if (endSilenceTime <= minTime)
    		endSilenceTime = minTime;
	}

	private void startCountingFrames() {
    	frameCount = 0;
    	endSilenceTime = initialEndSilenceTime;
	}

	private Data readData() throws DataProcessingException {
        return getPredecessor().getData();
    }

    private void sendToQueue(Data audio) {
        outputQueue.add(audio);
    }

    /**
     * Returns the amount of audio data in milliseconds in the given SpeechClassifiedData object.
     *
     * @param audio the SpeechClassifiedData object
     * @return the amount of audio data in milliseconds
     */
    public int getAudioTime(SpeechClassifiedData audio) {
        return (int)
                (audio.getValues().length * 1000.0f / audio.getSampleRate());
    }


    /**
     * Handles an SpeechClassifiedData object that can possibly be the first in an utterance.
     *
     * @param audio the SpeechClassifiedData to handle
     * @return true if utterance/speech has started for real, false otherwise
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     *
     */
    private boolean handleFirstSpeech(SpeechClassifiedData audio)
            throws DataProcessingException {
        int speechTime = getAudioTime(audio);

        // System.out.println("Entering handleFirstSpeech()");
        // try to read more that 'startSpeechTime' amount of
        // audio that is labeled as speech (the condition for speech start)

        while (speechTime < startSpeechTime) {
            Data next = readData();

            if (next == null) {
                return false;
            }

            sendToQueue(next);

            if (next instanceof SpeechClassifiedData) {
                if (!((SpeechClassifiedData) next).isSpeech()) {
                    return false;
                } else {
                    speechTime += getAudioTime(audio);
                }
            }
        }
        return true;
    }


    /**
     * Backtrack from the current position to add a SPEECH_START Signal to the outputQueue.
     */
    private void addSpeechStart() {
        long lastCollectTime = 0;
        int silenceLength = 0, initalSpeechLength = 0;
        ListIterator<Data> i = outputQueue.listIterator(outputQueue.size());

        // backtrack until we have 'speechLeader' amount of non-speech
        while ((silenceLength < speechLeader || initalSpeechLength < startSpeechTime) && i.hasPrevious()) {
            Data current = i.previous();
            if (current instanceof SpeechClassifiedData) {
                SpeechClassifiedData data = (SpeechClassifiedData) current;
                if (data.isSpeech()) {
                    initalSpeechLength += getAudioTime(data);
                } else {
                    silenceLength += getAudioTime(data);
                }
                lastCollectTime = data.getCollectTime();
            } else if (current instanceof DataStartSignal || current instanceof SpeechEndSignal) {
                i.next(); // put the SPEECH_START after the UTTERANCE_START
                break;
            } else if (current instanceof DataEndSignal) {
                throw new Error("Illegal signal " + current);
            }
        }

        // Add the SPEECH_START.
        i.add(new SpeechStartSignal(lastCollectTime));
    }


    /**
     * Given a non-speech frame, try to read more non-speech frames until we think its the end of utterance.
     *
     * @param audio a non-speech frame
     * @return true if speech has really ended, false if speech has not ended
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     *
     */
    private boolean readEndFrames(SpeechClassifiedData audio) throws
            DataProcessingException {

        boolean speechEndAdded = false;
        boolean readTrailer = true;
        int originalLast = outputQueue.size() - 1;
        int silenceLength = getAudioTime(audio);

        // read ahead until we have 'endSilenceTime' amount of silence
        while (silenceLength < endSilenceTime) {
            Data next = readData();
            if (next instanceof SpeechClassifiedData) {
                SpeechClassifiedData data = (SpeechClassifiedData) next;
                sendToQueue(data);
                if (data.isSpeech()) {
                    // if speech is detected again, we're still in
                    // an utterance
                    return false;
                } else {
                    // it is non-speech
                    silenceLength += getAudioTime(data);
                }
            } else if (next instanceof DataEndSignal) {
                sendToQueue(next);
                readTrailer = false;
                break;
            } else if (next instanceof Signal) {
                throw new Error("Illegal signal: " + next);
            }
        }

        if (readTrailer) {
            // read ahead until we have 'speechTrailer' amount of silence
            while (!speechEndAdded && silenceLength < speechTrailer) {
                Data next = readData();
                if (next instanceof SpeechClassifiedData) {
                    SpeechClassifiedData data = (SpeechClassifiedData) next;
                    if (data.isSpeech()) {
                        // if we have hit speech again, then the current
                        // speech should end
                        sendToQueue(new SpeechEndSignal(data.getCollectTime()));
                        sendToQueue(data);
                        speechEndAdded = true;
                        break;
                    } else {
                        silenceLength += getAudioTime(data);
                        sendToQueue(data);
                    }
                } else if (next instanceof DataEndSignal) {
                    sendToQueue(new SpeechEndSignal(((Signal) next).getTime()));
                    sendToQueue(next);
                    speechEndAdded = true;
                } else {
                    throw new Error("Illegal signal: " + next);
                }
            }
        }

        if (!speechEndAdded) {
            // iterate from the end of speech and read till we
            // have 'speechTrailer' amount of non-speech, and
            // then add an SPEECH_END
            ListIterator<Data> i = outputQueue.listIterator(originalLast);
            long nextCollectTime = 0;

            // the 'firstSampleNumber' of SPEECH_END actually contains
            // the last sample number of the segment
            long lastSampleNumber = 0;
            silenceLength = 0;

            while (silenceLength < speechTrailer && i.hasNext()) {
                Data next = i.next();
                if (next instanceof DataEndSignal) {
                    i.previous();
                    break;
                } else if (next instanceof SpeechClassifiedData) {
                    SpeechClassifiedData data = (SpeechClassifiedData) next;
                    nextCollectTime = data.getCollectTime();
                    assert !data.isSpeech();
                    silenceLength += getAudioTime(data);
                    lastSampleNumber = data.getFirstSampleNumber() +
                            data.getValues().length - 1;
                }
            }
            if (speechTrailer > 0) {
                assert nextCollectTime != 0 && lastSampleNumber != 0;
            }
            i.add(new SpeechEndSignal(nextCollectTime));
        }
        return true;
    }

    public boolean inSpeech() {
        return inSpeech;
    }
}

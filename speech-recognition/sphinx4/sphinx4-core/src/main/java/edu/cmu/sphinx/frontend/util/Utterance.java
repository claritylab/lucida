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

package edu.cmu.sphinx.frontend.util;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;


/** Represents the complete audio data of an utterance. */
public class Utterance {

    private final String name;
    private final ByteArrayOutputStream audioBuffer;
    private final AudioFormat audioFormat;


    /**
     * Constructs a default Utterance object.
     *
     * @param name   the name of this Utterance, e.g., it can be the name of the audio file where the audio came from
     * @param format the audio format of this Utterance
     */
    public Utterance(String name, AudioFormat format) {
        this.name = name;
        this.audioFormat = format;
        this.audioBuffer = new ByteArrayOutputStream();
    }


    /**
     * Returns the name of this Utterance.
     *
     * @return the name of this Utterance
     */
    public String getName() {
        return name;
    }


    /**
     * Returns the audio format of this Utterance.
     *
     * @return the audio format
     */
    public AudioFormat getAudioFormat() {
        return audioFormat;
    }


    /**
     * Adds an audio frame into this Utterance.
     *
     * @param audio the audio frame to add
     */
    public void add(byte[] audio) {
        synchronized (audioBuffer) {
            audioBuffer.write(audio, 0, audio.length);
        }
    }


    /**
     * Returns the complete audio stream of this utterance.
     *
     * @return the complete audio stream
     */
    public byte[] getAudio() {
        return audioBuffer.toByteArray();
    }


    /**
     * Returns the amount of time (in seconds) this Utterance lasts.
     *
     * @return how long is this utterance
     */
    public float getAudioTime() {
        return audioBuffer.size() /
                (audioFormat.getSampleRate() *
                        audioFormat.getSampleSizeInBits() / 8);
    }


    /**
     * Save the audio as a WAV file in the given file.
     *
     * @param fileName   the name of the audio file
     * @param fileFormat the audio file format
     * @throws java.io.IOException
     */
    public void save(String fileName, AudioFileFormat.Type fileFormat)
            throws IOException {
        File file = new File(fileName);
        byte[] audio = getAudio();
        AudioInputStream ais = new AudioInputStream
                ((new ByteArrayInputStream(audio)), getAudioFormat(), audio.length);
        AudioSystem.write(ais, fileFormat, file);
    }
}

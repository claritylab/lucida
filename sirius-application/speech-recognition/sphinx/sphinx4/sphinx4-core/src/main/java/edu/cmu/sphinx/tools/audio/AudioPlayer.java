/*
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2002-2004 Sun Microsystems, Inc.  
 * Portions Copyright 2002-2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.tools.audio;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.SourceDataLine;

/** Plays an AudioData in a separate thread. */
public class AudioPlayer extends Thread {

    private final AudioData audio;
    private SourceDataLine line;
    private int selectionStart;
    private int selectionEnd;


    /** Creates a new AudioPlayer for the given AudioData. */
    public AudioPlayer(AudioData audio) {
        this.audio = audio;
        selectionStart = 0;
        selectionEnd = audio.getAudioData().length;
    }


    /** Notifies the AudioPlayer thread to play the audio. */
    public void play(int selectionStart, int selectionEnd) {
        synchronized (audio) {
            this.selectionStart = selectionStart;
            this.selectionEnd = selectionEnd;
            audio.notify();
        }
    }


    /** Plays the AudioData in a separate thread. */
    @Override
    public void run() {
        while (true) {
            try {
                synchronized (audio) {
                    audio.wait();
                    AudioFormat format = audio.getAudioFormat();
                    short[] data = audio.getAudioData();
                    int start = Math.max(0, selectionStart);
                    int end = selectionEnd;
                    if (end == -1) {
                        end = data.length;
                    }

                    DataLine.Info info =
                            new DataLine.Info(SourceDataLine.class,
                                    format);
                    line = (SourceDataLine) AudioSystem.getLine(info);
                    line.open(format);
                    line.start();

                    byte[] frame = new byte[2];
                    for (int i = start;
                         i < end && i < data.length; i++) {
                        Utils.toBytes(data[i], frame, false);
                        line.write(frame, 0, frame.length);
                    }

                    line.drain();
                    line.close();
                    line = null;
                }
            } catch (Exception e) {
                e.printStackTrace();
                break;
            }
        }
    }
}

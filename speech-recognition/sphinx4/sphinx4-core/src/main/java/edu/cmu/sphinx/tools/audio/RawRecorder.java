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

import javax.sound.sampled.*;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

/** Grabs audio from the microphone and returns an array of samples. */
public class RawRecorder {

    final Object lock = new Object();
    RecordThread recorder;
    AudioFormat inFormat;
    final AudioFormat outFormat;
    TargetDataLine microphone;
    boolean downsample;


    /**
     * Create a new RawRecorder.
     *
     * @param audioFormat the desired output
     * @throws LineUnavailableException if the audioFormat is not supported
     */
    public RawRecorder(AudioFormat audioFormat)
            throws LineUnavailableException {

        inFormat = audioFormat;
        outFormat = audioFormat;

        /* Some machines, such as my Mac OS X PowerBook, don't support
         * a wide range of input formats.  So...we may need to read
         * data in using a different format and then resample to the
         * desired format.  Here, I'm just going to go for 44.1kHz
         * 16-bit signed little endian data if the given audio format
         * is not supported.
         */
        DataLine.Info info = new DataLine.Info(TargetDataLine.class,
                inFormat);

        if (!AudioSystem.isLineSupported(info)) {
            downsample = true;
            inFormat = new AudioFormat(44100.0f, // sample rate
                    16,       // sample size
                    1,        // channels (1 == mono)
                    true,     // signed
                    false);    // little endian
            info = new DataLine.Info(TargetDataLine.class,
                    inFormat);
            if (!AudioSystem.isLineSupported(info)) {
                throw new LineUnavailableException(
                        "Unsupported format: " + audioFormat);
            }
        }

        microphone = (TargetDataLine) AudioSystem.getLine(info);
        microphone.open(audioFormat, microphone.getBufferSize());
    }


    /**
     * Start recording.  The stop method will give us the clip.
     *
     * @see #stop
     */
    public void start() {
        synchronized (lock) {
            if (recorder != null) {
                recorder.stopRecording();
            }
            recorder = new RecordThread();
            recorder.start();
        }
    }


    /**
     * Stop recording and give us the clip.
     *
     * @return the clip that was recorded since the last time start was called
     * @see #start
     */
    public short[] stop() {
        synchronized (lock) {
            if (recorder == null) {
                return new short[0];
            }
            ByteArrayOutputStream out = recorder.stopRecording();
            microphone.close();
            recorder = null;
            byte audioBytes[] = out.toByteArray();
            ByteArrayInputStream in = new ByteArrayInputStream(audioBytes);
            try {
                short[] samples = RawReader.readAudioData(in, inFormat);
                if (downsample) {
                    samples = Downsampler.downsample(
                            samples,
                            (int) (inFormat.getSampleRate() / 1000.0f),
                            (int) (outFormat.getSampleRate() / 1000.0f));
                }
                return samples;
            } catch (IOException e) {
                e.printStackTrace();
                return new short[0];
            }
        }
    }


    class RecordThread extends Thread {

        boolean done;
        final Object lock = new Object();
        ByteArrayOutputStream out;


        public ByteArrayOutputStream stopRecording() {
            try {
                synchronized (lock) {
                    done = true;
                    lock.wait();
                }
            } catch (InterruptedException e) {
            }
            return out;
        }


        @Override
        public void run() {
            byte[] data = new byte[microphone.getBufferSize()];
            out = new ByteArrayOutputStream();

            try {
                microphone.flush();
                microphone.start();
                while (!done) {
                    int numBytesRead = microphone.read(data, 0, data.length);
                    if (numBytesRead != -1) {
                        out.write(data, 0, numBytesRead);
                    } else {
                        break;
                    }
                }
                microphone.stop();
                out.flush();
            } catch (IOException e) {
                e.printStackTrace();
            }
            synchronized (lock) {
                lock.notify();
            }
        }
    }
}

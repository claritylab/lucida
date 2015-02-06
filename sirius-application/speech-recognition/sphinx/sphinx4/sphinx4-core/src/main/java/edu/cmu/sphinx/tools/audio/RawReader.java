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
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

/**
 * Reads a raw audio file (i.e., a file that is just plain raw samples and nothing else) and converts it to signed
 * data.
 */
public class RawReader {

    /**
     * Reads raw bytes from the given audioStream and returns samples based on the audioFormat.  
     *
     * @param audioStream the stream containing the raw bytes
     * @param audioFormat a hint of what to expect from the stream
     * @return samples, one sample per array element
     */
    public static short[] readAudioData(InputStream audioStream,
                                        AudioFormat audioFormat)
            throws IOException {

        /* Initialize ourselves based on the input data format.
         */
        int bytesPerSample;
        boolean signedData = true;
        boolean bigEndian;
        AudioFormat.Encoding encoding = audioFormat.getEncoding();
        bytesPerSample = audioFormat.getSampleSizeInBits() / 8;
        if (encoding == AudioFormat.Encoding.PCM_SIGNED) {
            signedData = true;
        } else if (encoding == AudioFormat.Encoding.PCM_UNSIGNED) {
            signedData = false;
        } else {
            System.err.println("Unsupported audio encoding: " + encoding);
            System.exit(-1);
        }
        bigEndian = audioFormat.isBigEndian();

        /* Now read in the data, saving the samples in an array list.
         * Along the way, convert each sample to little endian signed
         * data.
         */
        byte[] buffer = new byte[bytesPerSample];
        ArrayList<Short> samples = new ArrayList<Short>();
        int read = 0;
        int totalRead = 0;
        boolean done = false;
        while (!done) {
            totalRead = read = audioStream.read(buffer, 0, bytesPerSample);
            while (totalRead < bytesPerSample) {
                if (read == -1) {
                    done = true;
                    break;
                } else {
                    read = audioStream.read(buffer,
                            totalRead,
                            bytesPerSample - totalRead);
                    totalRead += read;
                }
            }
            if (!done) {
                int val = 0;
                if (bigEndian) {
                    val = buffer[0];
                    if (!signedData) {
                        val &= 0xff;
                    }
                    for (int i = 1; i < bytesPerSample; i++) {
                        int temp = buffer[i] & 0xff;
                        val = (val << 8) + temp;
                    }
                } else {
                    val = buffer[bytesPerSample - 1];
                    if (!signedData) {
                        val &= 0xff;
                    }
                    for (int i = bytesPerSample - 2; i >= 0; i--) {
                        int temp = buffer[i] & 0xff;
                        val = (val << 8) + temp;
                    }
                }

                /* We'll always give signed data.  So, if the input
                 * is unsigned, convert it to signed.
                 */
                if (!signedData) {
                    val = val - (1 << ((bytesPerSample * 8) - 1));
                }
                samples.add((short) val);
            }
        }

        /* Convert the array list to an array of shorts and return.
        */
        short[] audioData = new short[samples.size()];
        for (int i = 0; i < audioData.length; i++) {
            audioData[i] = samples.get(i);
        }
        return audioData;
    }
}

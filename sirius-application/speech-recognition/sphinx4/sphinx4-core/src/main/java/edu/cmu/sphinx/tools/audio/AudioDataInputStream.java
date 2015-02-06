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

/** Converts an AudioData into an InputStream. */
public class AudioDataInputStream extends InputStream {

    AudioFormat format;
    int currentIndex;
    int markIndex;
    final short[] shorts;
    final byte[] bytes;


    /**
     * Creates a new AudioDataInputStream for the given AudioData.
     *
     * @param audio the AudioData to turn into an AudioDataInputStream
     */
    public AudioDataInputStream(AudioData audio) {
        shorts = audio.getAudioData();
        bytes = new byte[2 * shorts.length];

        byte[] sample = new byte[2];
        for (int i = 0; i < shorts.length; i++) {
            Utils.toBytes(shorts[i], sample, true);
            bytes[i * 2 + 1] = sample[0];
            bytes[i * 2] = sample[1];
        }
    }


    // inherited from InputStream
    //
    @Override
    public int read() throws IOException {
        if (currentIndex >= bytes.length) {
            return -1;
        } else {
            return bytes[currentIndex++];
        }
    }


    // inherited from InputStream
    //
    @Override
    public int read(byte[] buf) throws IOException {
        int count = 0;
        for (int i = 0; i < buf.length; i++) {
            if (currentIndex >= bytes.length) {
                break;
            } else {
                buf[i] = bytes[currentIndex++];
                count++;
            }
        }
        return (count == 0) ? -1 : count;
    }


    // inherited from InputStream
    //
    @Override
    public int read(byte[] buf, int off, int len) throws IOException {
        int count = 0;
        for (int i = 0; (i < len) && ((i + off) < buf.length); i++) {
            if (currentIndex >= bytes.length) {
                break;
            } else {
                buf[i + off] = bytes[currentIndex++];
                count++;
            }
        }
        return (count == 0) ? -1 : count;
    }


    // inherited from InputStream
    //
    public long skip(int n) throws IOException {
        int actual = n;
        if ((currentIndex + n) > bytes.length) {
            actual = bytes.length - currentIndex;
        }
        currentIndex += actual;
        return actual;
    }


    // inherited from InputStream
    //
    @Override
    public int available() throws IOException {
        return bytes.length - currentIndex;
    }


    // inherited from InputStream
    //
    @Override
    public void close() throws IOException {
        super.close();
    }


    // inherited from InputStream
    //
    @Override
    public void mark(int readLimit) {
        markIndex = currentIndex;
    }


    // inherited from AudioInputStream
    //
    @Override
    public boolean markSupported() {
        return true;
    }


    // inherited from AudioInputStream
    //
    @Override
    public void reset() throws IOException {
        currentIndex = markIndex;
    }
}

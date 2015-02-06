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
import javax.sound.sampled.AudioFormat.Encoding;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

public class Utils {

    /** Index = ulaw value, entry = signed 16 bit value. */
    static final private short[] ulawTable = {
            32760, 31608, 30584, 29560, 28536, 27512, 26488, 25464, 24440,
            23416, 22392, 21368, 20344, 19320, 18296, 17272, 16248, 15736,
            15224, 14712, 14200, 13688, 13176, 12664, 12152, 11640, 11128,
            10616, 10104, 9592, 9080, 8568, 8056, 7800, 7544, 7288, 7032,
            6776, 6520, 6264, 6008, 5752, 5496, 5240, 4984, 4728, 4472,
            4216, 3960, 3832, 3704, 3576, 3448, 3320, 3192, 3064, 2936,
            2808, 2680, 2552, 2424, 2296, 2168, 2040, 1912, 1848, 1784,
            1720, 1656, 1592, 1528, 1464, 1400, 1336, 1272, 1208, 1144,
            1080, 1016, 952, 888, 856, 824, 792, 760, 728, 696, 664, 632,
            600, 568, 536, 504, 472, 440, 408, 376, 360, 344, 328, 312,
            296, 280, 264, 248, 232, 216, 200, 184, 168, 152, 136, 120,
            112, 104, 96, 88, 80, 72, 64, 56, 48, 40, 32, 24, 16, 8, 0,
            -32760, -31608, -30584, -29560, -28536, -27512, -26488, -25464,
            -24440, -23416, -22392, -21368, -20344, -19320, -18296, -17272,
            -16248, -15736, -15224, -14712, -14200, -13688, -13176, -12664,
            -12152, -11640, -11128, -10616, -10104, -9592, -9080, -8568,
            -8056, -7800, -7544, -7288, -7032, -6776, -6520, -6264, -6008,
            -5752, -5496, -5240, -4984, -4728, -4472, -4216, -3960, -3832,
            -3704, -3576, -3448, -3320, -3192, -3064, -2936, -2808, -2680,
            -2552, -2424, -2296, -2168, -2040, -1912, -1848, -1784, -1720,
            -1656, -1592, -1528, -1464, -1400, -1336, -1272, -1208, -1144,
            -1080, -1016, -952, -888, -856, -824, -792, -760, -728, -696,
            -664, -632, -600, -568, -536, -504, -472, -440, -408, -376,
            -360, -344, -328, -312, -296, -280, -264, -248, -232, -216,
            -200, -184, -168, -152, -136, -120, -112, -104, -96, -88, -80,
            -72, -64, -56, -48, -40, -32, -24, -16, -8, 0};


    /** Uninstantiable class. */
    private Utils() {
    }


    /** Converts a byte array to a signed short value. */
    static public short toShort(byte[] bytes, boolean bigEndian) {
        if (bytes.length == 1) {
            return bytes[0];
        } else if (bigEndian) {
            return (short) ((bytes[0] << 8) | (0xff & bytes[1]));
        } else {
            return (short) ((bytes[1] << 8) | (0xff & bytes[0]));
        }
    }


    /** Converts a byte array into an unsigned short. */
    static public int toUnsignedShort(byte[] bytes, boolean bigEndian) {
        if (bytes.length == 1) {
            return 0xff & bytes[0];
        } else if (bigEndian) {
            return ((bytes[0] & 0xff) << 8) | (0xff & bytes[1]);
        } else {
            return ((bytes[1] & 0xff) << 8) | (0xff & bytes[0]);
        }
    }


    /** Converts a short into a byte array. */
    public static void toBytes(short sVal, byte[] bytes, boolean bigEndian) {
        if (bigEndian) {
            bytes[0] = (byte) (sVal >> 8);
            bytes[1] = (byte) (sVal & 0xff);
        } else {
            bytes[0] = (byte) (sVal & 0xff);
            bytes[1] = (byte) (sVal >> 8);
        }
    }


    /**
     * Convert the bytes starting at the given offset to a signed short based upon the AudioFormat.  If the frame size
     * is 1, then the value is doubled to make it match a frame size of 2.
     *
     * @param format    the audio format
     * @param byteArray the byte array
     * @return a short
     * @throws java.lang.ArrayIndexOutOfBoundsException
     *
     */
    public static short bytesToShort(AudioFormat format,
                                     byte[] byteArray) {
        short result = 0;
        Encoding encoding = format.getEncoding();
        int frameSize = format.getFrameSize();

        if (encoding == Encoding.PCM_SIGNED) {
            result = toShort(byteArray, format.isBigEndian());
            if (frameSize == 1) {
                result = (short) (result << 8);
            }
        } else if (encoding == Encoding.PCM_UNSIGNED) {
            int tmp = toUnsignedShort(byteArray, format.isBigEndian());
            if (frameSize == 1) {
                tmp = tmp << 8;
            }
            result = (short) (tmp - (2 << 14));
        } else if (encoding == Encoding.ULAW) {
            result = ulawTable[byteArray[0] + 128];
        } else {
            System.out.println("Unknown encoding: " + encoding);
        }
        return result;
    }


    /**
     * Turns the AudioInputStream into a 16bit, SIGNED_PCM, little endian audio stream that preserves the original sample
     * rate of the AudioInputStream.  NOTE:  this assumes the frame size can be only 1 or 2 bytes.  The AudioInputStream
     * is left in a state of having all of its data being read.
     */
    static public short[] toSignedPCM(AudioInputStream ais)
            throws IOException {
        AudioFormat aisFormat = ais.getFormat();

        short[] shorts = new short[ais.available() / aisFormat.getFrameSize()];
        byte[] frame = new byte[aisFormat.getFrameSize()];

        int pos = 0;
        while (ais.read(frame) != -1) {
            shorts[pos++] = bytesToShort(aisFormat, frame);
        }

        return shorts;
    }


    /**
     * Attempts to read an audio file using the Java Sound APIs.  If this file isn't a typical audio file, then this
     * returns a null.  Otherwise, it converts the data into a 8kHz 16-bit signed PCM little endian clip.
     *
     * @param filename the file containing audio data
     * @return the audio data or null if the audio cannot be parsed
     */
    static public AudioData readAudioFile(String filename) throws IOException {
        try {
            BufferedInputStream stream = new BufferedInputStream(
                    new FileInputStream(filename));
            AudioInputStream ais = AudioSystem.getAudioInputStream(stream);
            AudioData audioData = new AudioData(ais);
            stream.close();
            return audioData;
        } catch (UnsupportedAudioFileException e) {
            return null;
        }
    }


    /**
     * Reads the given stream in as 8kHz 16-bit signed PCM little endian audio data and returns an audio clip.
     *
     * @param filename the file containing audio data
     * @return the audio data or null if the audio cannot be parsed
     */
    static public AudioData readRawFile(String filename)
            throws IOException {
        FileInputStream stream = new FileInputStream(filename);
        AudioFormat format = new AudioFormat(8000.0f, // sample rate
                16,       // sample size
                1,        // channels (1 == mono)
                true,     // signed
                false);    // little endian
        short[] audioData = RawReader.readAudioData(stream, format);
        stream.close();
        return new AudioData(audioData, 8000.0f);
    }


    /** Writes the given 8kHz 16-bit signed PCM audio clip to the given file as raw little endian data. */
    static public void writeRawFile(AudioData audio, String filename)
            throws IOException {

        FileOutputStream outputStream = new FileOutputStream(filename);
        AudioFormat format = new AudioFormat(
                8000.0f, // sample rate
                16,       // sample size
                1,        // channels (1 == mono)
                true,     // signed
                false);    // little endian
        RawWriter writer = new RawWriter(outputStream, format);
        short[] samples = audio.getAudioData();
        for (short sample : samples) {
            writer.writeSample(sample);
        }
        outputStream.flush();
        outputStream.close();
    }
}

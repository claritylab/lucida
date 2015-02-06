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

import edu.cmu.sphinx.frontend.DoubleData;
import edu.cmu.sphinx.frontend.FloatData;
import edu.cmu.sphinx.util.Utilities;

import javax.sound.sampled.*;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.DecimalFormat;


/** Defines utility methods for manipulating data values. */
public class DataUtil {

    private static final int HEXADECIMAL = 1;
    private static final int SCIENTIFIC = 2;
    private static final int DECIMAL = 3;


    /** DecimalFormat object to be used by all the methods. */
    private static final DecimalFormat format = new DecimalFormat();


    private static final int decimalIntegerDigits = 10;
    private static final int decimalFractionDigits = 5;

    private static final int floatScientificFractionDigits = 8;
    private static final int doubleScientificFractionDigits = 8;


    /** The number format to be used by *ArrayToString() methods. The default is scientific. */
    private static int dumpFormat = SCIENTIFIC;

    /**
     * Static initialization of dumpFormat
     */
    static {
        String formatProperty = System.getProperty("frontend.util.dumpformat",
                "SCIENTIFIC");
        if (formatProperty.compareToIgnoreCase("DECIMAL") == 0) {
            dumpFormat = DECIMAL;
        } else if (formatProperty.compareToIgnoreCase("HEXADECIMAL") == 0) {
            dumpFormat = HEXADECIMAL;
        } else if (formatProperty.compareToIgnoreCase("SCIENTIFIC") == 0) {
            dumpFormat = SCIENTIFIC;
        }
    }


    /** Uninstantiable class. */
    private DataUtil() {
    }


    /**
     * Converts a byte array into a short array. Since a byte is 8-bits, and a short is 16-bits, the returned short
     * array will be half in length than the byte array. If the length of the byte array is odd, the length of the short
     * array will be <code>(byteArray.length - 1)/2</code>, i.e., the last byte is discarded.
     *
     * @param byteArray a byte array
     * @param offset    which byte to start from
     * @param length    how many bytes to convert
     * @return a short array, or <code>null</code> if byteArray is of zero length
     * @throws java.lang.ArrayIndexOutOfBoundsException
     *
     */
    public static short[] byteToShortArray
            (byte[] byteArray, int offset, int length)
            throws ArrayIndexOutOfBoundsException {

        if (0 < length && (offset + length) <= byteArray.length) {
            int shortLength = length / 2;
            short[] shortArray = new short[shortLength];
            int temp;
            for (int i = offset, j = 0; j < shortLength;
                 j++, temp = 0x00000000) {
                temp = byteArray[i++] << 8;
                temp |= 0x000000FF & byteArray[i++];
                shortArray[j] = (short) temp;
            }
            return shortArray;
        } else {
            throw new ArrayIndexOutOfBoundsException
                    ("offset: " + offset + ", length: " + length
                            + ", array length: " + byteArray.length);
        }
    }


    /**
     * Converts a big-endian byte array into an array of doubles. Each consecutive bytes in the byte array are converted
     * into a double, and becomes the next element in the double array. The size of the returned array is
     * (length/bytesPerValue). Currently, only 1 byte (8-bit) or 2 bytes (16-bit) samples are supported.
     *
     * @param byteArray     a byte array
     * @param offset        which byte to start from
     * @param length        how many bytes to convert
     * @param bytesPerValue the number of bytes per value
     * @param signedData    whether the data is signed
     * @return a double array, or <code>null</code> if byteArray is of zero length
     * @throws java.lang.ArrayIndexOutOfBoundsException
     *
     */
    public static double[] bytesToValues(byte[] byteArray,
                                               int offset,
                                               int length,
                                               int bytesPerValue,
                                               boolean signedData)
            throws ArrayIndexOutOfBoundsException {

        if (0 < length && (offset + length) <= byteArray.length) {
            assert (length % bytesPerValue == 0);
            double[] doubleArray = new double[length / bytesPerValue];

            int i = offset;

            for (int j = 0; j < doubleArray.length; j++) {
                int val = byteArray[i++];
                if (!signedData) {
                    val &= 0xff; // remove the sign extension
                }
                for (int c = 1; c < bytesPerValue; c++) {
                    int temp = byteArray[i++] & 0xff;
                    val = (val << 8) + temp;
                }

                doubleArray[j] = val;
            }

            return doubleArray;
        } else {
            throw new ArrayIndexOutOfBoundsException
                    ("offset: " + offset + ", length: " + length
                            + ", array length: " + byteArray.length);
        }
    }


    /**
     * Converts a little-endian byte array into an array of doubles. Each consecutive bytes of a float are converted
     * into a double, and becomes the next element in the double array. The number of bytes in the double is specified
     * as an argument. The size of the returned array is (data.length/bytesPerValue).
     *
     * @param data          a byte array
     * @param offset        which byte to start from
     * @param length        how many bytes to convert
     * @param bytesPerValue the number of bytes per value
     * @param signedData    whether the data is signed
     * @return a double array, or <code>null</code> if byteArray is of zero length
     * @throws java.lang.ArrayIndexOutOfBoundsException
     *
     */
    public static double[] littleEndianBytesToValues(byte[] data,
                                                           int offset,
                                                           int length,
                                                           int bytesPerValue,
                                                           boolean signedData)
            throws ArrayIndexOutOfBoundsException {

        if (0 < length && (offset + length) <= data.length) {
            assert (length % bytesPerValue == 0);
            double[] doubleArray = new double[length / bytesPerValue];

            int i = offset + bytesPerValue - 1;

            for (int j = 0; j < doubleArray.length; j++) {
                int val = data[i--];
                if (!signedData) {
                    val &= 0xff; // remove the sign extension
                }
                for (int c = 1; c < bytesPerValue; c++) {
                    int temp = data[i--] & 0xff;
                    val = (val << 8) + temp;
                }

                // advance 'i' to the last byte of the next value
                i += (bytesPerValue * 2);

                doubleArray[j] = val;
            }

            return doubleArray;

        } else {
            throw new ArrayIndexOutOfBoundsException
                    ("offset: " + offset + ", length: " + length
                            + ", array length: " + data.length);
        }
    }


    /**
     * Convert the two bytes starting at the given offset to a short.
     *
     * @param byteArray the byte array
     * @param offset    where to start
     * @return a short
     * @throws java.lang.ArrayIndexOutOfBoundsException
     *
     */
    public static short bytesToShort(byte[] byteArray, int offset)
            throws ArrayIndexOutOfBoundsException {
        short result = (short)
                ((byteArray[offset++] << 8) |
                        (0x000000FF & byteArray[offset]));
        return result;
    }


    /**
     * Returns the string representation of the given short array. The string will be in the form:
     * <pre>data.length data[0] data[1] ... data[data.length-1]</pre>
     *
     * @param data the short array to convert
     * @return a string representation of the short array
     */
    public static String shortArrayToString(short[] data) {
        StringBuilder dump = new StringBuilder().append(data.length);
        for (short val : data) {
            dump.append(' ').append(val);
        }
        return dump.toString();
    }


    /**
     * Returns the given double array as a string. The string will be in the form:
     * <pre>data.length data[0] data[1] ... data[data.length-1]</pre>where
     * <code>data[i]</code>.
     * <p/>
     * The doubles can be written as decimal, hexadecimal, or scientific notation. In decimal notation, it is formatted
     * by the method <code>Util.formatDouble(data[i], 10, 5)</code>. Use the System property
     * <code>"frontend.util.dumpformat"</code> to control the dump format (permitted values are "decimal",
     * "hexadecimal", and "scientific".
     *
     * @param data the double array to dump
     * @return a string representation of the double array
     */
    public static String doubleArrayToString(double[] data) {
        return doubleArrayToString(data, dumpFormat);
    }


    /**
     * Returns the given double array as a string. The dump will be in the form:
     * <pre>data.length data[0] data[1] ... data[data.length-1]</pre>where
     * <code>data[i]</code> is formatted by the method <code>Util.formatDouble(data[i], 10, 5)</code>.
     *
     * @param data   the double array to dump
     * @param format either HEXADECIMAL, SCIENTIFIC or DECIMAL
     * @return a string representation of the double array
     */
    private static String doubleArrayToString(double[] data, int format) {
        StringBuilder dump = new StringBuilder().append(data.length);

        for (double val : data) {
            if (format == DECIMAL) {
                dump.append(' ').append(formatDouble
                    (val, decimalIntegerDigits,
                        decimalFractionDigits));
            } else if (format == HEXADECIMAL) {
                long binary = Double.doubleToRawLongBits(val);
                dump.append(" 0x").append(Long.toHexString(binary));
            } else if (format == SCIENTIFIC) {
                dump.append(' ').append(Utilities.doubleToScientificString
                    (val, doubleScientificFractionDigits));
            }
        }
        return dump.toString();
    }


    /**
     * Returns the given float array as a string. The string is of the form:
     * <pre>data.length data[0] data[1] ... data[data.length-1]</pre>
     * <p/>
     * The floats can be written as decimal, hexadecimal, or scientific notation. In decimal notation, it is formatted
     * by the method <code>Util.formatDouble(data[i], 10, 5)</code>. Use the System property
     * <code>"frontend.util.dumpformat"</code> to control the dump format (permitted values are "decimal",
     * "hexadecimal", and "scientific".
     *
     * @param data the float array to dump
     * @return a string of the given float array
     */
    public static String floatArrayToString(float[] data) {
        return floatArrayToString(data, dumpFormat);
    }


    /**
     * Returns the given float array as a string. The string is of the form:
     * <pre>data.length data[0] data[1] ... data[data.length-1]</pre>
     *
     * @param data   the float array to dump
     * @param format either DECIMAL, HEXADECIMAL or SCIENTIFIC
     * @return a string of the given float array
     */
    private static String floatArrayToString(float[] data, int format) {
        StringBuilder dump = new StringBuilder().append(data.length);

        for (float val : data) {
            if (format == DECIMAL) {
                dump.append(' ').append(formatDouble
                    (val,
                        decimalIntegerDigits, decimalFractionDigits));
            } else if (format == HEXADECIMAL) {
                int binary = Float.floatToRawIntBits(val);
                dump.append(" 0x").append(Integer.toHexString(binary));
            } else if (format == SCIENTIFIC) {
                dump.append(' ').append(Utilities.doubleToScientificString
                    (val, floatScientificFractionDigits));
            }
        }
        return dump.toString();
    }


    /**
     * Returns a formatted string of the given number, with the given numbers of digit space for the integer and
     * fraction parts. If the integer part has less than <code>integerDigits</code> digits, spaces will be prepended to
     * it. If the fraction part has less than <code>fractionDigits</code>, spaces will be appended to it. Therefore,
     * <code>formatDouble(12345.6789, 6, 6)</code> will give
     * the string <pre>" 12345.6789  "</pre> (one space before 1, two spaces
     * after 9).
     *
     * @param number         the number to format
     * @param integerDigits  the length of the integer part
     * @param fractionDigits the length of the fraction part
     * @return a formatted number
     */
    public static String formatDouble(double number, int integerDigits,
                                      int fractionDigits) {
        StringBuilder formatter = new StringBuilder(2 + fractionDigits).append("0.");
        for (int i = 0; i < fractionDigits; i++) {
            formatter.append('0');
        }

        format.applyPattern(formatter.toString());
        String formatted = format.format(number);

        // pad preceding spaces before the number
        int dotIndex = formatted.indexOf('.');
        if (dotIndex == -1) {
            formatted += ".";
            dotIndex = formatted.length() - 1;
        }
        StringBuilder result = new StringBuilder();
        for (int i = dotIndex; i < integerDigits; i++) {
            result.append(' ');
        }
        result.append(formatted);
        return result.toString();
    }


    /**
     * Returns the number of samples per window given the sample rate (in Hertz) and window size (in milliseconds).
     *
     * @param sampleRate     the sample rate in Hertz (i.e., frequency per seconds)
     * @param windowSizeInMs the window size in milliseconds
     * @return the number of samples per window
     */
    public static int getSamplesPerWindow(int sampleRate,
                                          float windowSizeInMs) {
        return (int) (sampleRate * windowSizeInMs / 1000);
    }


    /**
     * Returns the number of samples in a window shift given the sample rate (in Hertz) and the window shift (in
     * milliseconds).
     *
     * @param sampleRate      the sample rate in Hertz (i.e., frequency per seconds)
     * @param windowShiftInMs the window shift in milliseconds
     * @return the number of samples in a window shift
     */
    public static int getSamplesPerShift(int sampleRate,
                                         float windowShiftInMs) {
        return (int) (sampleRate * windowShiftInMs / 1000);
    }


    /**
     * Saves the given bytes to the given binary file.
     *
     * @param data     the bytes to save
     * @param filename the binary file name
     * @throws IOException if an I/O error occurs
     */
    public static void bytesToFile(byte[] data, String filename)
            throws IOException {
        FileOutputStream file = new FileOutputStream(filename);
        file.write(data);
        file.close();
    }


    /**
     * Returns a native audio format that has the same encoding, endianness and sample size as the given format, and a
     * sample rate that is larger than the given sample rate.
     *
     * @param format
     * @return a suitable native audio format
     */
    public static AudioFormat getNativeAudioFormat(AudioFormat format) {
        return getNativeAudioFormat(format, null);
    }


    /**
     * Returns a native audio format that has the same encoding, endianness and sample size as the given format, and a
     * sample rate that is greater than or equal to the given sample rate.
     *
     * @param format the desired format
     * @param mixer  if non-null, use this Mixer; otherwise use AudioSystem
     * @return a suitable native audio format
     */
    public static AudioFormat getNativeAudioFormat(AudioFormat format,
                                                   Mixer mixer) {
        Line.Info[] lineInfos;

        if (mixer != null) {
            lineInfos = mixer.getTargetLineInfo
                    (new Line.Info(TargetDataLine.class));
        } else {
            lineInfos = AudioSystem.getTargetLineInfo
                    (new Line.Info(TargetDataLine.class));
        }

        AudioFormat nativeFormat = null;

        // find a usable target line
        for (Line.Info info : lineInfos) {

            AudioFormat[] formats = ((TargetDataLine.Info)info).getFormats();

            for (AudioFormat thisFormat : formats) {

                // for now, just accept downsampling, not checking frame
                // size/rate (encoding assumed to be PCM)

                if (thisFormat.getEncoding() == format.getEncoding()
                    && thisFormat.isBigEndian() == format.isBigEndian()
                    && thisFormat.getSampleSizeInBits() ==
                    format.getSampleSizeInBits()
                    && thisFormat.getSampleRate() >= format.getSampleRate()) {
                    nativeFormat = thisFormat;
                    break;
                }
            }
            if (nativeFormat != null) {
                //no need to look through remaining lineinfos
                break;
            }
        }
        return nativeFormat;
    }


    /** Converts DoubleData object to FloatDatas.
     * @param data
     */
    public static DoubleData FloatData2DoubleData(FloatData data) {
        int numSamples = data.getValues().length;

        double[] doubleData = new double[numSamples];
        float[] values = data.getValues();
        for (int i = 0; i < values.length; i++) {
            doubleData[i] = values[i];
        }
//      System.arraycopy(data.getValues(), 0, doubleData, 0, numSamples); 

        return new DoubleData(doubleData, data.getSampleRate(), data.getFirstSampleNumber());
    }


    /** Converts FloatData object to DoubleData.
     *  @param data
     */
    public static FloatData DoubleData2FloatData(DoubleData data) {
        int numSamples = data.getValues().length;

        float[] floatData = new float[numSamples];
        double[] values = data.getValues();
        for (int i = 0; i < values.length; i++) {
            floatData[i] = (float) values[i];
        }
//        System.arraycopy(data.getValues(), 0, floatData, 0, numSamples);

        return new FloatData(floatData, data.getSampleRate(), data.getFirstSampleNumber());
    }
}

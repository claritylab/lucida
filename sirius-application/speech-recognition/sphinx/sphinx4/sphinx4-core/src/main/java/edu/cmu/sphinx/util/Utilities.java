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

package edu.cmu.sphinx.util;

import java.io.*;
import java.text.DecimalFormat;


/** Provides a set of generic utilities */
public class Utilities {

    private final static boolean TRACKING_OBJECTS = false;


    // Unconstructable.
    private Utilities() {
    }


    /**
     * Returns a string with the given number of spaces.
     *
     * @param padding the number of spaces in the string
     * @return a string of length 'padding' containing only the SPACE char.
     */
    public static String pad(int padding) {
        if (padding > 0) {
            StringBuilder sb = new StringBuilder(padding);
            for (int i = 0; i < padding; i++) {
                sb.append(' ');
            }
            return sb.toString();
        } else {
            return "";
        }
    }


    /**
     * Pads with spaces or truncates the given string to guarantee that it is exactly the desired length.
     *
     * @param string    the string to be padded
     * @param minLength the desired length of the string
     * @return a string of length containing string padded with whitespace or truncated
     */
    public static String pad(String string, int minLength) {
        String result = string;
        int pad = minLength - string.length();
        if (pad > 0) {
            result = string + pad(minLength - string.length());
        } else if (pad < 0) {
            result = string.substring(0, minLength);
        }
        return result;
    }


    /**
     * Pads with spaces or truncates the given int to guarantee that it is exactly the desired length.
     *
     * @param val       the value to be padded
     * @param minLength the desired length of the string
     * @return a string of length containing string padded with whitespace or truncated
     */
    public static String pad(int val, int minLength) {
        return pad(String.valueOf(val), minLength);
    }


    /**
     * Pads with spaces or truncates the given double to guarantee that it is exactly the desired length.
     *
     * @param val       the value to be padded
     * @param minLength the desired length of the string
     * @return a string of length containing string padded with whitespace or truncated
     */
    public static String pad(double val, int minLength) {
        return pad(String.valueOf(val), minLength);
    }

    /**
     * Converts class name into readable string
     * @param name  input name
     * @return readable class name
     */
    public static String getReadable(String name) {
        StringBuilder builder = new StringBuilder();
        
        for (int i = 0; i < name.length(); i++) {
            char c = name.charAt(i);
            if (i == 0) {
                builder.append(Character.toUpperCase(c));
            } else if (Character.isUpperCase(c) && !Character.isUpperCase(name.charAt(i - 1))) {
                builder.append(' ');
                builder.append(c);
            } else {
                builder.append(c);
            }
        }
        return builder.toString();
    }

    /**
     * Dumps padded text. This is a simple tool for helping dump text with padding to a Writer.
     *
     * @param pw      the stream to send the output
     * @param padding the number of spaces in the string
     * @param string  the string to output
     */
    public static void dump(PrintWriter pw, int padding, String string) {
        pw.print(pad(padding));
        pw.println(string);
    }


    /**
     * utility method for tracking object counts
     *
     * @param name  the name of the object
     * @param count the count of objects
     */
    public static void objectTracker(String name, int count) {
        if (TRACKING_OBJECTS) {
            if (count % 1000 == 0) {
                System.out.println("OT: " + name + ' ' + count);
            }
        }
    }


    static long maxUsed;


    /**
     * Dumps  out memory information
     *
     * @param msg addditional text for the dump
     */

    public static void dumpMemoryInfo(String msg) {
        Runtime rt = Runtime.getRuntime();
        long free = rt.freeMemory();
        rt.gc();
        long reclaimedMemory = (rt.freeMemory() - free)
                / (1024 * 1024);
        long freeMemory = rt.freeMemory() / (1024 * 1024);
        long totalMemory = rt.totalMemory() / (1024 * 1024);
        long usedMemory = rt.totalMemory() - rt.freeMemory();

        if (usedMemory > maxUsed) {
            maxUsed = usedMemory;
        }

        System.out.println("Memory (mb) "
                + " total: " + totalMemory
                + " reclaimed: " + reclaimedMemory
                + " free: " + freeMemory
                + " Max Used: " + (maxUsed / (1024 * 1024))
                + " -- " + msg);
    }


    /**
     * Returns the string representation of the given double value in normalized scientific notation. The
     * <code>fractionDigits</code> argument gives the number of decimal digits in the fraction portion. For example, if
     * <code>fractionDigits</code> is 4, then the 123450 will be "1.2345e+05". There will always be two digits in the
     * exponent portion, and a plus or minus sign before the exponent.
     *
     * @param number         the double to convert
     * @param fractionDigits the number of digits in the fraction part, e.g., 4 in "1.2345e+05".
     * @return the string representation of the double in scientific notation
     */
    public static String doubleToScientificString(double number,
                                                  int fractionDigits) {
        DecimalFormat format = new DecimalFormat();

        StringBuilder formatter = new StringBuilder(5 + fractionDigits).append("0.");
        for (int i = 0; i < fractionDigits; i++) {
            formatter.append('0');
        }
        formatter.append("E00");

        format.applyPattern(formatter.toString());
        String formatted = format.format(number);

        int index = formatted.indexOf('E');
        if (formatted.charAt(index + 1) != '-') {
            return formatted.substring(0, index + 1) + '+' +
                    formatted.substring(index + 1);
        } else {
            return formatted;
        }
    }


    /**
     * Returns true if the given binary cepstra file is in big-endian format. It assumes that the first 4 bytes of the
     * file tells you how many 4-byte floating point cepstra values are in the file.
     *
     * @param filename the cepstra file name
     * @return true if the given binary cepstra file is big-endian
     */
    public static boolean isCepstraFileBigEndian(String filename)
            throws IOException {
        File cepstraFile = new File(filename);
        int fileSize = (int) cepstraFile.length();
        DataInputStream stream =
                new DataInputStream(new FileInputStream(filename));
        int numberBytes = stream.readInt() * 4 + 4;
        stream.close();
        return (fileSize == numberBytes);
    }


    /**
     * Reads the next float from the given DataInputStream, where the data is in little endian.
     *
     * @param dataStream the DataInputStream to read from
     * @return a float
     */
    public static float readLittleEndianFloat(DataInputStream dataStream)
            throws IOException {
        return Float.intBitsToFloat(readLittleEndianInt(dataStream));
    }


    /**
     * Reads the next little-endian integer from the given DataInputStream.
     *
     * @param dataStream the DataInputStream to read from
     * @return an integer
     */
    public static int readLittleEndianInt(DataInputStream dataStream)
            throws IOException {
        return dataStream.readUnsignedByte() | dataStream.readUnsignedByte() << 8 |
               dataStream.readUnsignedByte() << 16 | dataStream.readUnsignedByte() << 24;
    }


    /**
     * Byte-swaps the given integer to the other endian. That is, if this integer is big-endian, it becomes
     * little-endian, and vice-versa.
     *
     * @param integer the integer to swap
     */
    public static int swapInteger(int integer) {
        return (((0x000000ff & integer) << 24) |
                ((0x0000ff00 & integer) << 8) |
                ((0x00ff0000 & integer) >> 8) |
                ((0xff000000 & integer) >> 24));
    }


    /**
     * Byte-swaps the given float to the other endian. That is, if this float is big-endian, it becomes little-endian,
     * and vice-versa.
     *
     * @param floatValue the float to swap
     */
    public static float swapFloat(float floatValue) {
        return Float.intBitsToFloat
                (swapInteger(Float.floatToRawIntBits(floatValue)));
    }
    

    /**
     * If a data point is below 'floor' make it equal to floor.
     *
     * @param data  the data to floor
     * @param floor the floored value
     */
    public static void floorData(float[] data, float floor) {
        for (int i = 0; i < data.length; i++) {
            if (data[i] < floor) {
                data[i] = floor;
            }
        }
    }
    /**
     * If a data point is non-zero and below 'floor' make it equal to floor
     * (don't floor zero values though).
     * 
     * @param data the data to floor
     * @param floor the floored value
     */
    public static void nonZeroFloor(float[] data, float floor) {
        for (int i = 0; i < data.length; i++) {
            if (data[i] != 0.0 && data[i] < floor) {
                data[i] = floor;
            }
        }
    }
    

    /**
     * Normalize the given data.
     * 
     * @param data the data to normalize
     */
    public static void normalize(float[] data) {
        float sum = 0;
        for (float val : data) {
            sum += val;
        }
        if (sum != 0.0f) {
            for (int i = 0; i < data.length; i++) {
                data[i] = data[i] / sum;
            }
        }
    }
}

  

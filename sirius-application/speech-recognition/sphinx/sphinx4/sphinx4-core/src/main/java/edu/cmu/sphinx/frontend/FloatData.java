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


package edu.cmu.sphinx.frontend;

import edu.cmu.sphinx.util.MatrixUtils;

/**
 * A Data object that holds data of primitive type float.
 *
 * @see Data
 */
@SuppressWarnings("serial")
public class FloatData implements Data, Cloneable {

    private final float[] values;
    private final int sampleRate;
    private final long firstSampleNumber;
    private final long collectTime;


    /**
     * Constructs a Data object with the given values, sample rate, collect time, and first sample number.
     *
     * @param values            the data values
     * @param sampleRate        the sample rate of the data
     * @param collectTime       the time at which this data is collected
     * @param firstSampleNumber the position of the first sample in the original data
     */
    public FloatData(float[] values, int sampleRate,
                     long firstSampleNumber) {
        this.values = values;
        this.sampleRate = sampleRate;
        this.collectTime = firstSampleNumber * 1000 / sampleRate;
        this.firstSampleNumber = firstSampleNumber;
    }

    /**
     * Constructs a Data object with the given values, sample rate, collect time, and first sample number.
     *
     * @param values            the data values
     * @param sampleRate        the sample rate of the data
     * @param collectTime       the time at which this data is collected
     * @param firstSampleNumber the position of the first sample in the original data
     */
    public FloatData(float[] values, int sampleRate,
                     long collectTime, long firstSampleNumber) {
        this.values = values;
        this.sampleRate = sampleRate;
        this.collectTime = collectTime;
        this.firstSampleNumber = firstSampleNumber;
    }


    /**
     * @return the values of this data.
     */
    public float[] getValues() {
        return values;
    }


    /**
     * @return the sample rate of this data.
     */
    public int getSampleRate() {
        return sampleRate;
    }


    /**
     * @return the position of the first sample in the original data. The very first sample number is zero.
     */
    public long getFirstSampleNumber() {
        return firstSampleNumber;
    }


    /**
     * Returns the time in milliseconds at which the audio data is collected.
     *
     * @return the difference, in milliseconds, between the time the audio data is collected and midnight, January 1,
     *         1970
     */
    public long getCollectTime() {
        return collectTime;
    }

    @Override
    public FloatData clone() throws CloneNotSupportedException {
        try {
            FloatData data = (FloatData)super.clone();
            return data;
        } catch (CloneNotSupportedException e) {
            throw new InternalError(e.toString());
        }
    }


    /** Converts a given Data-object into a <code>FloatData</code> if possible.
     *  @param data
     */
    public static FloatData toFloatData(Data data) {
        FloatData convertData;
        if (data instanceof FloatData)
            convertData = (FloatData) data;
        else if (data instanceof DoubleData) {
            DoubleData dd = (DoubleData) data;
            convertData = new FloatData(MatrixUtils.double2float(dd.getValues()), dd.getSampleRate(),
                    dd.getFirstSampleNumber());
        } else
            throw new IllegalArgumentException("data type '" + data.getClass() + "' is not supported");

        return convertData;
    }
}

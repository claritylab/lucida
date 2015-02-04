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

import edu.cmu.sphinx.util.machlearn.OVector;

/** A Data object that holds data of primitive type double. */
@SuppressWarnings("serial")
public class DoubleData extends OVector implements Data {

    private int sampleRate;
    private long firstSampleNumber;
    private long collectTime;


    /**
     * Constructs a new <code>Data</code> object with values only. All other internal fields like
     * sampling rate etc. are initialized to -1.
     * @param values
     */
    public DoubleData(double[] values) {
        super(values);
    }


    /**
     * Constructs a Data object with the given values, collect time, and first sample number.
     *
     * @param values            the data values
     * @param sampleRate        the sample rate of the data
     * @param firstSampleNumber the position of the first sample in the original data
     */
    public DoubleData(double[] values, int sampleRate,
                      long firstSampleNumber) {
        super(values);

        this.sampleRate = sampleRate;
        this.collectTime = firstSampleNumber * 1000 / sampleRate;
        this.firstSampleNumber = firstSampleNumber;
    }

    /**
     * Constructs a Data object with the given values, collect time, and first sample number.
     *
     * @param values            the data values
     * @param sampleRate        the sample rate of the data
     * @param collectTime       the time at which this data is collected
     * @param firstSampleNumber the position of the first sample in the original data
     */
    public DoubleData(double[] values, int sampleRate,
                      long collectTime, long firstSampleNumber) {
        super(values);

        this.sampleRate = sampleRate;
        this.collectTime = collectTime;
        this.firstSampleNumber = firstSampleNumber;
    }

    /**
     * @return a string that describes the data.
     */
    @Override
    public String toString() {
        return ("DoubleData: " + sampleRate + "Hz, first sample #: " +
                firstSampleNumber + ", collect time: " + collectTime);
    }


    /**
     * @return the sample rate of the data.
     */
    public int getSampleRate() {
        return sampleRate;
    }


    /**
     * @return the position of the first sample in the original data. The very first sample number
     * is zero.
     */
    public long getFirstSampleNumber() {
        return firstSampleNumber;
    }


    /**
     * Returns the time in milliseconds at which the audio data is collected.
     *
     * @return the difference, in milliseconds, between the time the audio data is collected and
     *         midnight, January 1, 1970
     */
    public long getCollectTime() {
        return collectTime;
    }

    @Override
    public DoubleData clone() throws CloneNotSupportedException {
        try {
            DoubleData data = (DoubleData)super.clone();
            data.sampleRate = sampleRate;
            data.collectTime = collectTime;
            data.firstSampleNumber = firstSampleNumber;
            return data;
        } catch (CloneNotSupportedException e) {
            throw new InternalError(e.toString());
        }
    }
}

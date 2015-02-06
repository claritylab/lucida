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

package edu.cmu.sphinx.linguist.acoustic.tiedstate.trainer;

import edu.cmu.sphinx.util.LogMath;

/** Used to accumulate data for updating of models. */
class Buffer {

    private double[] numerator;
    private double denominator;
    private boolean wasUsed;
    // Maybe isLog should be used otherwise: one single, say,
    // accumulate(), which would be directed according to isLog. But
    // then again having accumulate() and logAccumulate() makes it
    // clearer if we're dealing with log scale or not...
    private boolean isLog;
    private int id;


    /**
     * Creates a new buffer. If the values will be in log, the buffer is initialized to all
     * <code>LogMath.LOG_ZERO</code>.
     *
     * @param size  the number of elements in this buffer
     * @param isLog if true, the values in the buffer will be in log
     */
    Buffer(int size, boolean isLog, int id) {
        this.id = id;
        this.isLog = isLog;
        wasUsed = false;
        numerator = new double[size];
        if (isLog) {
            denominator = LogMath.LOG_ZERO;
            for (int i = 0; i < size; i++) {
                numerator[i] = LogMath.LOG_ZERO;
            }
        }
    }


    /**
     * Accumulates data to this buffer. Data are accumulated to a given numerator buffer and to the denominator buffer.
     *
     * @param data  the data to be added
     * @param entry the numerator entry to be accumulated to
     */
    void accumulate(float data, int entry) {
        // Not needed anymore??
        assert false;
        assert numerator != null;
        assert !isLog;
        numerator[entry] += data;
        denominator += data;
        wasUsed = true;
    }


    /**
     * Accumulates data to this buffer. Data are accumulated to a given numerator buffer and to the denominator buffer.
     *
     * @param data    the data to be added
     * @param entry   the numerator entry to be accumulated to
     * @param logMath the logMath to use
     */
    void logAccumulate(float data, int entry, LogMath logMath) {
        assert numerator != null;
        assert isLog;
        numerator[entry] = logMath.addAsLinear((float) numerator[entry], data);
        denominator = logMath.addAsLinear((float) denominator, data);
        wasUsed = true;
    }


    /**
     * Accumulates data to this buffer. Data are accumulated to a given numerator buffer and to the denominator buffer.
     *
     * @param numeratorData   the data to be added to the numerator
     * @param denominatorData the data to be added to the denominator
     */
    void accumulate(double[] numeratorData, double denominatorData) {
        assert numerator != null;
        assert numeratorData != null;
        assert numerator.length == numeratorData.length;
        assert !isLog;
        for (int i = 0; i < numerator.length; i++) {
            numerator[i] += numeratorData[i];
        }
        denominator += denominatorData;
        wasUsed = true;
    }


    /**
     * Accumulates data to this buffer. Data are accumulated to a given numerator buffer and to the denominator buffer.
     *
     * @param logNumeratorData   the data to be added to the numerator
     * @param logDenominatorData the data to be added to the denominator
     * @param logMath            the LogMath instance to be used
     */
    void logAccumulate(float[] logNumeratorData, float logDenominatorData,
                       LogMath logMath) {
        assert numerator != null;
        assert logNumeratorData != null;
        assert numerator.length == logNumeratorData.length;
        assert isLog;
        for (int i = 0; i < numerator.length; i++) {
            numerator[i] =
                    logMath.addAsLinear((float) numerator[i], logNumeratorData[i]);
        }
        denominator = logMath.addAsLinear((float) denominator, logDenominatorData);
        wasUsed = true;
    }


    /**
     * Normalize the buffer. This method divides the numerator by the denominator, storing the result in the numerator,
     * and setting denominator to 1.
     */
    void normalize() {
        assert !isLog;
        if (denominator == 0) {
            System.out.println("Empty denominator: " + id);
            // dump();
            // assert false;
            wasUsed = false;
            return;
        }

        double invDenominator = (1.0 / denominator);
        for (int i = 0; i < numerator.length; i++) {
            numerator[i] *= invDenominator;
        }
        denominator = 1.0;
    }


    /**
     * Normalize the buffer in log scale. This method divides the numerator by the denominator, storing the result in
     * the numerator, and setting denominator to log(1) = 0.
     */
    void logNormalize() {
        assert isLog;
        for (int i = 0; i < numerator.length; i++) {
            numerator[i] -= denominator;
        }
        denominator = 0.0f;
    }


    /**
     * Normalize the non-zero elements in a buffer in log scale. This method divides the numerator by the denominator,
     * storing the result in the numerator, and setting denominator to log(1) = 0. A mask is used to tell whether a
     * component should be normalized (non-zero) or not (zero).
     *
     * @param mask a vector containing zero/non-zero values.
     */
    void logNormalizeNonZero(float[] mask) {
        assert isLog;
        assert mask.length == numerator.length;
        for (int i = 0; i < numerator.length; i++) {
            if (mask[i] != LogMath.LOG_ZERO) {
                numerator[i] -= denominator;
            }
        }
        denominator = 0.0;
    }


    /** Normalize the buffer. The normalization is done so that the summation of elements in the buffer is 1. */
    void normalizeToSum() {
        assert !isLog;
        float den = 0.0f;
        for (double val : numerator) {
            den += val;
        }
        float invDenominator = (float) (1.0 / den);
        for (int i = 0; i < numerator.length; i++) {
            numerator[i] *= invDenominator;
        }
        denominator = 1.0;
    }


    /**
     * Normalize the buffer in log scale. The normalization is done so that the summation of elements in the buffer is
     * log(1) = 0. In this, we assume that if an element has a value of zero, it won't be updated.
     *
     * @param logMath the logMath to use
     */
    void logNormalizeToSum(LogMath logMath) {
        assert isLog;
        float logZero = LogMath.LOG_ZERO;
        float den = logZero;
        for (double val : numerator) {
            if (val != logZero) {
                den = logMath.addAsLinear(den, (float)val);
            }
        }
        for (int i = 0; i < numerator.length; i++) {
            if (numerator[i] != logZero) {
                numerator[i] -= den;
            }
        }
        denominator = 0.0;
    }


    /**
     * Floor the buffer.
     *
     * @param floor the floor for this buffer
     * @return if true, the buffer was modified
     */
    protected boolean floor(float floor) {
        assert !isLog;
        boolean wasModified = false;
        for (int i = 0; i < numerator.length; i++) {
            if (numerator[i] < floor) {
                wasModified = true;
                numerator[i] = floor;
            }
        }
        return wasModified;
    }


    /**
     * Floor the buffer in log scale.
     *
     * @param logFloor the floor for this buffer, in log scale
     * @return if true, the buffer was modified
     */
    protected boolean logFloor(float logFloor) {
        assert isLog;
        boolean wasModified = false;
        for (int i = 0; i < numerator.length; i++) {
            if (numerator[i] < logFloor) {
                wasModified = true;
                numerator[i] = logFloor;
            }
        }
        return wasModified;
    }


    /**
     * Retrieves a value from this buffer. Make sure you normalize the buffer first.
     *
     * @param entry the index into the buffer
     * @return the value
     */
    protected float getValue(int entry) {
        return (float) numerator[entry];
    }


    /**
     * Set the entry in this buffer to a value.
     *
     * @param entry the index into the buffer
     * @param value the value
     */
    protected void setValue(int entry, float value) {
        numerator[entry] = value;
    }


    /**
     * Retrieves a vector from this buffer. Make sure you normalize the buffer first.
     *
     * @return the value
     */
    protected float[] getValues() {
        float[] returnVector = new float[numerator.length];
        for (int i = 0; i < numerator.length; i++) {
            returnVector[i] = (float) numerator[i];
        }
        return returnVector;
    }


    /**
     * Returns whether the buffer was used.
     *
     * @return if true, the buffer was used
     */
    protected boolean wasUsed() {
        return wasUsed;
    }


    /** Dump info about this buffer. */
    public void dump() {
        System.out.println("Denominator= " + denominator);
        System.out.println("Numerators= ");
        for (int i = 0; i < numerator.length; i++) {
            System.out.println("[" + i + "]= " + numerator[i]);
        }
    }

}


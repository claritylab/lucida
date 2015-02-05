/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */

package edu.cmu.sphinx.util;

import static edu.cmu.sphinx.util.Preconditions.checkState;

/**
 * Provides a set of methods for performing simple math in the log domain.
 *
 * The logarithmic base can be set by the
 * property: <code>edu.cmu.sphinx.util.LogMath.logBase</code>
 */
public final class LogMath {

    public static final float LOG_ZERO = -Float.MAX_VALUE;
    public static final float LOG_ONE = 0.f;

    private static final String IMMUTABLE_INSTANCE_ERROR =
        "Parameters must be set before the class instance is obtained";

    // Singeleton instance.
    private static LogMath instance;
    private static float logBase = 1.0001f;
    private static boolean useTable = true;

    private float naturalLogBase;
    private float inverseNaturalLogBase;

    private float theAddTable[];
    private float maxLogValue;
    private float minLogValue;

    private LogMath() {
        naturalLogBase = (float) Math.log(logBase);
        inverseNaturalLogBase = 1.0f / naturalLogBase;
        // When converting a number from/to linear, we need to make
        // sure it's within certain limits to prevent it from
        // underflowing/overflowing.
        // We compute the max value by computing the log of the max
        // value that a float can contain.
        maxLogValue = linearToLog(Double.MAX_VALUE);
        // We compute the min value by computing the log of the min
        // (absolute) value that a float can hold.
        minLogValue = linearToLog(Double.MIN_VALUE);
        if (useTable) {
            // Now create the addTable table.
            // summation needed in the loop
            float innerSummation;
            // First decide number of elements.
            int entriesInTheAddTable;
            final int veryLargeNumberOfEntries = 150000;
            final int verySmallNumberOfEntries = 0;
            // To decide size of table, take into account that a base
            // of 1.0001 or 1.0003 converts probabilities, which are
            // numbers less than 1, into integers. Therefore, a good
            // approximation for the smallest number in the table,
            // therefore the value with the highest index, is an
            // index that maps into 0.5: indices higher than that, if
            // they were present, would map to less values less than
            // 0.5, therefore they would be mapped to 0 as
            // integers. Since the table implements the expression:
            //
            // log(1.0 + base^(-index)))
            //
            // then the highest index would be:
            //
            // topIndex = - log(logBase^(0.5) - 1)
            //
            // where log is the log in the appropriate base.
            //
            // Added -Math.rint(...) to round to nearest
            // integer. Added the negation to match the preceding
            // documentation
            entriesInTheAddTable = (int) -Math
                .rint(linearToLog(logToLinear(0.5f) - 1));
            // We reach this max if the log base is 1.00007. The
            // closer you get to 1, the higher the number of entries
            // in the table.
            if (entriesInTheAddTable > veryLargeNumberOfEntries) {
                entriesInTheAddTable = veryLargeNumberOfEntries;
            }
            if (entriesInTheAddTable <= verySmallNumberOfEntries) {
                throw new IllegalArgumentException("The log base " + logBase
                        + " yields a very small addTable. "
                        + "Either choose not to use the addTable, "
                        + "or choose a logBase closer to 1.0");
            }
            // PBL added this just to see how many entries really are
            // in the table
            theAddTable = new float[entriesInTheAddTable];
            for (int index = 0; index < entriesInTheAddTable; ++index) {
                // This loop implements the expression:
                //
                // log( 1.0 + power(base, index))
                //
                // needed to add two numbers in the log domain.
                innerSummation = (float) logToLinear(-index);
                innerSummation += 1.0f;
                theAddTable[index] = linearToLog(innerSummation);
            }
        }
    }

    public static LogMath getInstance() {
        if (null == instance) {
            synchronized(LogMath.class) {
                if (null == instance)
                    instance = new LogMath();
            }
        }

        return instance;
    }

    /**
     * Sets log base.
     *
     * According to forum discussions a value between 1.00001 and 1.0004 should
     * be used for speech recognition. Going above 1.0005 will probably hurt.
     *
     * @param logbase Log base
     *
     * @throws IllegalStateException if LogMath instance has been already got
     */
    public static void setLogBase(float logBase) {
        synchronized(LogMath.class) {
            checkState(useTable, IMMUTABLE_INSTANCE_ERROR);
            LogMath.logBase = logBase;
        }
    }

    /**
     * The property that controls whether we use the old, slow (but correct)
     * method of performing the LogMath.add by doing the actual computation.
     *
     * @throws IllegalStateException if LogMath instance has been already got
     */
    public static void setUseTable(boolean useTable) {
        synchronized(LogMath.class) {
            checkState(null == instance, IMMUTABLE_INSTANCE_ERROR);
            LogMath.useTable = useTable;
        }
    }

    /**
     * Returns the summation of two numbers when the arguments and the result are in log. <p/> <p/> That is, it returns
     * log(a + b) given log(a) and log(b) </p> <p/> <p/> This method makes use of the equality: </p> <p/> <p/> <b>log(a
     * + b) = log(a) + log (1 + exp(log(b) - log(a))) </b> </p> <p/> <p/> which is derived from: </p> <p/> <p/> <b>a + b
     * = a * (1 + (b / a)) </b> </p> <p/> <p/> which in turns makes use of: </p> <p/> <p/> <b>b / a = exp (log(b) -
     * log(a)) </b> </p> <p/> <p/> Important to notice that <code>subtractAsLinear(a, b)</code> is *not* the same as
     * <code>addAsLinear(a, -b)</code>, since we're in the log domain, and -b is in fact the inverse. </p> <p/> <p/> No
     * underflow/overflow check is performed. </p>
     *
     * @param logVal1 value in log domain (i.e. log(val1)) to add
     * @param logVal2 value in log domain (i.e. log(val2)) to add
     * @return sum of val1 and val2 in the log domain
     */
    public final float addAsLinear(float logVal1, float logVal2) {
        float logHighestValue = logVal1;
        float logDifference = logVal1 - logVal2;
        /*
         * [ EBG: maybe we should also have a function to add many numbers, *
         * say, return the summation of all terms in a given vector, if *
         * efficiency becomes an issue.
         */
        // difference is always a positive number
        if (logDifference < 0) {
            logHighestValue = logVal2;
            logDifference = -logDifference;
        }
        return logHighestValue + addTable(logDifference);
    }

    /**
     * Method used by add() internally. It returns the difference between the highest number and the total summation of
     * two numbers. <p/> Considering the expression (in which we assume natural log) <p/> <p/> <b>log(a + b) = log(a) +
     * log(1 + exp(log(b) - log(a))) </b> </p>
     * <p/>
     * the current function returns the second term of the right hand side of the equality above, generalized for the
     * case of any log base. This function can be constructed as a table, if table lookup is faster than actual
     * computation.
     *
     * @param index the index into the addTable
     * @return the value pointed to by index
     */
    private float addTableActualComputation(float index) {
        double logInnerSummation;
        // Negate index, since the derivation of this formula implies
        // the smallest number as a numerator, therefore the log of the
        // ratio is negative
        logInnerSummation = logToLinear(-index);
        logInnerSummation += 1.0;
        return linearToLog(logInnerSummation);
    }

    /**
     * Method used by add() internally. It returns the difference between the highest number and the total summation of
     * two numbers. <p/> Considering the expression (in which we assume natural log) <p/> <p/> <b>log(a + b) = log(a) +
     * log(1 + exp(log(b) - log(a))) </b> </p>
     * <p/>
     * the current function returns the second term of the right hand side of the equality above, generalized for the
     * case of any log base. This function is constructed as a table lookup.
     *
     * @param index the index into the addTable
     * @return the value pointed to by index
     * @throws IllegalArgumentException
     */
    private float addTable(float index) throws IllegalArgumentException {
        if (useTable) {
            // int intIndex = (int) Math.rint(index);
            int intIndex = (int) (index + 0.5);
            // When adding two numbers, the highest one should be
            // preserved, and therefore the difference should always
            // be positive.
            if (0 <= intIndex) {
                if (intIndex < theAddTable.length) {
                    return theAddTable[intIndex];
                } else {
                    return 0.0f;
                }
            } else {
                throw new IllegalArgumentException("addTable index has "
                        + "to be negative");
            }
        } else {
            return addTableActualComputation(index);
        }
    }

    /**
     * Returns the difference between two numbers when the arguments and the result are in log. <p/> <p/> That is, it
     * returns log(a - b) given log(a) and log(b) </p> <p/> <p/> Implementation is less efficient than add(), since
     * we're less likely to use this function, provided for completeness. Notice however that the result only makes
     * sense if the minuend is higher than the subtrahend. Otherwise, we should return the log of a negative number.
     * </p> <p/> <p/> It implements the subtraction as: </p> <p/> <p/> <b>log(a - b) = log(a) + log(1 - exp(log(b) -
     * log(a))) </b> </p> <p/> <p/> No need to check for underflow/overflow. </p>
     *
     * @param logMinuend    value in log domain (i.e. log(minuend)) to be subtracted from
     * @param logSubtrahend value in log domain (i.e. log(subtrahend)) that is being subtracted
     * @return difference between minuend and the subtrahend in the log domain
     * @throws IllegalArgumentException <p/> This is a very slow way to do this, but this method should rarely be used.
     *                                  </p>
     */
    public final float subtractAsLinear(float logMinuend, float logSubtrahend)
            throws IllegalArgumentException {
        double logInnerSummation;
        if (logMinuend < logSubtrahend) {
            throw new IllegalArgumentException("Subtraction results in log "
                    + "of a negative number: " + logMinuend + " - "
                    + logSubtrahend);
        }
        logInnerSummation = 1.0;
        logInnerSummation -= logToLinear(logSubtrahend - logMinuend);
        return logMinuend + linearToLog(logInnerSummation);
    }

    /**
     * Converts the source, which is assumed to be a log value whose base is sourceBase, to a log value whose base is
     * resultBase. Possible values for both the source and result bases include Math.E, 10.0, LogMath.getLogBase(). If a
     * source or result base is not supported, an IllegalArgumentException will be thrown. <p/> <p/> It takes advantage
     * of the relation: </p> <p/> <p/> <b>log_a(b) = log_c(b) / lob_c(a) </b> </p> <p/> <p/> or: </p> <p/> <p/>
     * <b>log_a(b) = log_c(b) * lob_a(c) </b> </p> <p/> <p/> where <b>log_a(b) </b> is logarithm of <b>b </b> base <b>a
     * </b> etc. </p>
     *
     * @param logSource  log value whose base is sourceBase
     * @param sourceBase the base of the log the source
     * @param resultBase the base to convert the source log to
     * @throws IllegalArgumentException
     */
    //  [[[ TODO: This is slow, but it probably doesn't need
    //  to be too fast ]]]
    // [ EBG: it can be made more efficient if one of the bases is
    // Math.E. So maybe we should consider two functions logToLn and
    // lnToLog instead of a generic function like this??
    //
    public static float logToLog(float logSource, float sourceBase,
                                 float resultBase) throws IllegalArgumentException {
        if ((sourceBase <= 0) || (resultBase <= 0)) {
            throw new IllegalArgumentException("Trying to take log of "
                    + " non-positive number: " + sourceBase + " or "
                    + resultBase);
        }
        if (logSource == LOG_ZERO) {
            return LOG_ZERO;
        }
        float lnSourceBase = (float) Math.log(sourceBase);
        float lnResultBase = (float) Math.log(resultBase);
        return (logSource * lnSourceBase / lnResultBase);
    }

    /**
     * Converts the source, which is a number in base Math.E, to a log value which base is the LogBase of this LogMath.
     *
     * @param logSource the number in base Math.E to convert
     */
    public final float lnToLog(float logSource) {
        if (logSource == LOG_ZERO) {
            return LOG_ZERO;
        }
        return (logSource * inverseNaturalLogBase);
    }

    /**
     * Converts the source, which is a number in base 10, to a log value which base is the LogBase of this LogMath.
     *
     * @param logSource the number in base Math.E to convert
     */
    public final float log10ToLog(float logSource) {
        if (logSource == LOG_ZERO) {
            return LOG_ZERO;
        }
        return logToLog(logSource, 10.0f, logBase);
    }

    /**
     * Converts the source, whose base is the LogBase of this LogMath, to a log value which is a number in base Math.E.
     *
     * @param logSource the number to convert to base Math.E
     */
    public final float logToLn(float logSource) {
        if (logSource == LOG_ZERO) {
            return LOG_ZERO;
        }
        return logSource * naturalLogBase;
    }

    /**
     * Converts the value from linear scale to log scale. The log scale numbers are limited by the range of the type
     * float. The linear scale numbers can be any double value.
     *
     * @param linearValue the value to be converted to log scale
     * @return the value in log scale
     * @throws IllegalArgumentException
     */
    public final float linearToLog(double linearValue)
            throws IllegalArgumentException {
        double returnValue;
        if (linearValue < 0.0) {
            throw new IllegalArgumentException(
                    "linearToLog: param must be >= 0: " + linearValue);
        } else if (linearValue == 0.0) {
            // [EBG] Shouldn't the comparison above be something like
            // linearValue < "epsilon"? Is it ever going to be 0.0?
            return LOG_ZERO;
        } else {
            returnValue = Math.log(linearValue) * inverseNaturalLogBase;
            if (returnValue > Float.MAX_VALUE) {
                return Float.MAX_VALUE;
            } else {
                if (returnValue < -Float.MAX_VALUE) {
                    return -Float.MAX_VALUE;
                } else {
                    return (float) returnValue;
                }
            }
        }
    }

    /**
     * Converts the value from log scale to linear scale.
     *
     * @param logValue the value to be converted to the linear scale
     * @return the value in the linear scale
     */
    public final double logToLinear(float logValue) {
        // return Math.pow(logBase, logValue);
        double returnValue;
        if (logValue < minLogValue) {
            returnValue = 0.0;
        } else if (logValue > maxLogValue) {
            returnValue = Double.MAX_VALUE;
        } else {
            returnValue = Math.exp(logToLn(logValue));
        }
        return returnValue;
    }

    /** Returns the actual log base. */
    public final float getLogBase() {
        return logBase;
    }

    public boolean isUseTable() {
        return useTable;
    }

    /**
     * Returns the log (base 10) of value
     *
     * @param value the value to take the log of
     * @return the log (base 10) of value
     */
    // [ EBG: Shouldn't we be using something like logToLog(value, base, 10)
    // for this? ]
    public static float log10(float value) {
        return (float) (0.4342944819 * java.lang.Math.log(value));
        // If you want to get rid of the constant:
        // return ((1.0f / Math.log(10.0f)) * Math.log(value));
    }

    /** Converts a vector from linear domain to log domain using a given <code>LogMath</code>-instance for conversion. */
    public void linearToLog(float[] vector) {
        int nbGaussians = vector.length;
        for (int i = 0; i < nbGaussians; i++) {
            vector[i] = linearToLog(vector[i]);
        }
    }

    
    /** Converts a vector from log to linear domain using a given <code>LogMath</code>-instance for conversion. */
    public void logToLinear(float[] vector, float[] out) {
        for (int i = 0; i < vector.length; i++) {
            out[i] = (float)logToLinear(vector[i]);
        }
    }
}

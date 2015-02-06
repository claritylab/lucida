package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DoubleData;

/**
 * Copyright 1999-2006 Carnegie Mellon University. Portions Copyright 2002 Sun Microsystems, Inc. Portions Copyright
 * 2002 Mitsubishi Electric Research Laboratories. All Rights Reserved.  Use is subject to license terms.
 * <p/>
 * See the file "license.terms" for information on usage and redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 * <p/>
 * User: Peter Wolf Date: Mar 9, 2006 Time: 9:30:52 PM
 */
public class VUMeter {

    private double rms;
    private double average;
    private double peak;

    private static final double log10 = Math.log(10.0);
    private static final double maxDB = Math.max(0.0, 20.0 * Math.log(Short.MAX_VALUE) / log10);

    private final int peakHoldTime = 1000;
    private long then = System.currentTimeMillis();

    private final float a2 = -1.9556f;
    private final float a3 = 0.9565f;

    private final float b1 = 0.9780f;
    private final float b2 = -1.9561f;
    private final float b3 = 0.9780f;


    public final synchronized double getRmsDB() {
        return Math.max(0.0, 20.0 * Math.log(rms) / log10);
    }


    public final synchronized double getAverageDB() {
        return Math.max(0.0, 20.0 * Math.log(average) / log10);
    }


    public final synchronized double getPeakDB() {
        return Math.max(0.0, 20.0 * Math.log(peak) / log10);
    }


    public final synchronized boolean getIsClipping() {
        return (Short.MAX_VALUE) < (2 * peak);
    }


    public final synchronized double getMaxDB() {
        return maxDB;
    }


    public void calculateVULevels(Data data) {

        if (data instanceof DoubleData) {

            double[] samples = ((DoubleData) data).getValues();

            calculateVULevels(samples);
        }
    }


    public void calculateVULevels(byte[] data, int offset, int cnt) {
        short[] samples = new short[cnt / 2];
        for (int i = 0; i < cnt / 2; i++) {
            int o = offset + (2 * i);
            samples[i] = (short) ((data[o] << 8) | (0x000000FF & data[o + 1]));
            //System.out.print(data[2*i] + "+" +data[(2*i)+1] + "=" + samples[i] + " ");
        }
        calculateVULevels(samples);
    }


    private synchronized void calculateVULevels(double[] samples) {
        double energy = 0.0;
        average = 0.0;

        double y1 = 0.0f;
        double y2 = 0.0f;


        for (int i = 0; i < samples.length; i++) {

            // remove the DC offset with a filter

            //System.out.print(samples[i] + " ");

            double i1 = samples[i];
            double j = 0;
            double k = 0;

            if (i > 0) {
                j = samples[i - 1];
            }
            if (i > 1) {
                k = samples[i - 2];
            }

            double y = b1 * i1 + b2 * j + b3 * k - a2 * y1 - a3 * y2;

            y2 = y1;
            y1 = y;

            double v2 = Math.abs(y);

            long now = System.currentTimeMillis();

            energy += v2 * v2;
            average += v2;

            if (v2 > peak) {
                peak = v2;
            } else if ((now - then) > peakHoldTime) {
                peak = v2;
                then = now;
            }

        }

        rms = energy / samples.length;
        rms = Math.sqrt(rms);
        average /= samples.length;
    }


    private synchronized void calculateVULevels(short[] samples) {

        double energy = 0.0;
        average = 0.0;

        double y1 = 0.0f;
        double y2 = 0.0f;


        for (int i = 0; i < samples.length; i++) {

            // remove the DC offset with a filter

            //System.out.print(samples[i] + " ");

            short i1 = samples[i];
            double j = 0;
            double k = 0;

            if (i > 0) {
                j = samples[i - 1];
            }
            if (i > 1) {
                k = samples[i - 2];
            }

            double y = b1 * i1 + b2 * j + b3 * k - a2 * y1 - a3 * y2;

            y2 = y1;
            y1 = y;

            double v2 = Math.abs(y);

            long now = System.currentTimeMillis();

            energy += v2 * v2;
            average += v2;

            if (v2 > peak) {
                peak = v2;
            } else if ((now - then) > peakHoldTime) {
                peak = v2;
                then = now;
            }

        }

        rms = energy / samples.length;
        rms = Math.sqrt(rms);
        average /= samples.length;
    }


}

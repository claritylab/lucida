/*
 * Copyright 1999-2013 Carnegie Mellon University.  
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */

package edu.cmu.sphinx.tools.bandwidth;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Scanner;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.frequencywarp.MelFrequencyFilterBank;
import edu.cmu.sphinx.frontend.transform.DiscreteFourierTransform;
import edu.cmu.sphinx.frontend.util.AudioFileDataSource;
import edu.cmu.sphinx.frontend.window.RaisedCosineWindower;

/**
 * A simple energy-based detector for upsampled audio. Could be used to detect
 * bandwidth issues leading to the accuracy issues.
 * 
 * The detector simply looks for energies in different mel bands and using the
 * threshold it decides if we have cut of the frequencies signal. On every frame
 * we find the maximum energy band, then we just control that energy doesn't
 * fall too fast in upper bands.
 * 
 * A paper on the subject is "DETECTING BANDLIMITED AUDIO IN BROADCAST TELEVISION SHOWS"
 * by by Mark C. Fuhs, Qin Jin and Tanja Schultz where spline approximation is proposed
 * for detection. However, the paper seems to contain a fundamental flaw. The
 * decision is made on average spectrum, not per-frame. This probably leads
 * to omission of the events in high frequency which might signal about wide band.
 */
public class BandDetector {

    static final int bands = 40;

    // From 4750 to 6800 Hz
    static final int highRangeStart = 35;
    static final int highRangeEnd = 39;

    // From 2156 to 3687 Hz
    static final int lowRangeStart = 23;
    static final int lowRangeEnd = 29;

    // Thresholds, selected during the experiments, about -30dB
    static final double noSignalLevel = 0.02;
    static final double signalLevel = 0.5;

    // Don't care if intensity is very low
    static final double lowIntensity = 1e+5;

    private FrontEnd frontend;
    private AudioFileDataSource source;

    public BandDetector() {

        // standard frontend
        source = new AudioFileDataSource(320, null);
        RaisedCosineWindower windower = new RaisedCosineWindower(0.97f,
                25.625f, 10.0f);
        DiscreteFourierTransform fft = new DiscreteFourierTransform(512, false);
        MelFrequencyFilterBank filterbank = new MelFrequencyFilterBank(130.0,
                6800.0, bands);

        ArrayList<DataProcessor> list = new ArrayList<DataProcessor>();
        list.add(source);
        list.add(windower);
        list.add(fft);
        list.add(filterbank);

        frontend = new FrontEnd(list);
    }

    public static void main(String args[]) throws FileNotFoundException {

        if (args.length < 1) {
            System.out
                    .println("Usage: Detector <filename.wav> or Detector <filelist>");
            return;
        }

        if (args[0].endsWith(".wav")) {
            BandDetector detector = new BandDetector();
            System.out.println("Bandwidth for " + args[0] + " is "
                    + detector.bandwidth(args[0]));
        } else {
            BandDetector detector = new BandDetector();
            Scanner s = new Scanner(new File(args[0]));
            while (s.hasNextLine()) {
                String line = s.nextLine().trim();
                if (detector.bandwidth(line))
                    System.out.println("Bandwidth for " + line + " is low");
            }
            s.close();
        }
        return;
    }

    public boolean bandwidth(String file) {

        source.setAudioFile(new File(file), "");

        Data data;
        double energy[] = new double[bands];

        while ((data = frontend.getData()) != null) {
            if (data instanceof DoubleData) {

                double maxIntensity = lowIntensity;
                double[] frame = ((DoubleData) data).getValues();

                for (int i = 0; i < bands; i++)
                    maxIntensity = Math.max(maxIntensity, frame[i]);

                if (maxIntensity <= lowIntensity) {
                    continue;
                }

                for (int i = 0; i < bands; i++) {
                    energy[i] = Math.max(frame[i] / maxIntensity, energy[i]);
                }
            }
        }

        double maxLow = max(energy, lowRangeStart, lowRangeEnd);
        double maxHi = max(energy, highRangeStart, highRangeEnd);

        // System.out.format("%f %f\n", maxHi, maxLow);
        // for (int i = 0; i < bands; i++)
        // System.out.format("%.4f ", energy[i]);
        // System.out.println();

        if (maxHi < noSignalLevel && maxLow > signalLevel)
            return true;

        return false;
    }

    private double max(double[] energy, int start, int end) {
        double max = 0;
        for (int i = start; i <= end; i++)
            max = Math.max(max, energy[i]);
        return max;
    }
}

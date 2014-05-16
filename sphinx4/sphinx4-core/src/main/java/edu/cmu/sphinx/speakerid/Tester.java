/*
 * 
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.speakerid;

import java.io.*;
import java.util.ArrayList;
import java.util.Random;

public class Tester {

    /**
     * Generates artificial input with distinct speakers based
     * 
     * @param vectorSize
     *            The dimension of a feature vector
     * @param vectorsCount
     *            The number of features vectors per speakers
     * @param speakersCount
     *            The number of speakers
     * @return List of features that satisfies the given requirements
     */
    public static ArrayList<float[]> generateDistinctSpeakers(int vectorSize, int vectorsCount,
            int speakersCount) {
        Random rd = new Random();
        ArrayList<float[]> ret = new ArrayList<float[]>();
        float[] dummy = new float[vectorSize];
        for (int i = 0; i < speakersCount; i++) {
            for (int j = 0; j < vectorSize; j++)
                dummy[j] = (float) (i + 1) / 10 + (float) rd.nextInt(5000) / 50000;
            dummy[0] = 3 + (float) (i + 1) / 10;
            for (int j = 0; j < vectorsCount; j++) {
                float[] copy = new float[vectorSize];
                for (int k = 0; k < vectorSize; k++)
                    copy[k] = dummy[k] + (float) rd.nextInt(5000) / 50000;
                ret.add(copy);
            }
        }
        return ret;
    }

    /**
     * Returns string version of the given time in miliseconds
     * 
     * @param seconds
     * @return mm:ss
     */
    public static String time(int seconds) {
        return (seconds / 60000) + ":" + (Math.round((double) (seconds % 60000) / 1000));
    }

    /**
     * @param speakers
     *            An array of clusters for which it is needed to be printed the
     *            speakers intervals
     */
    public static void printIntervals(ArrayList<SpeakerCluster> speakers) {
        System.out.println("Detected " + speakers.size() + " Speakers :");
        int idx = 0;
        for (SpeakerCluster spk : speakers) {
            System.out.print("Speaker " + (++idx) + ": ");
            ArrayList<Segment> segments = spk.getSpeakerIntervals();
            for (Segment seg : segments)
                System.out.print("[" + time(seg.getStartTime()) + " " + time(seg.getLength()) + "]");
            System.out.println();
        }
    }

    /**
     * 
     * @param speakers
     *            An array of clusters for which it is needed to be printed the
     *            speakers intervals
     * @throws IOException
     */
    public static void printSpeakerIntervals(ArrayList<SpeakerCluster> speakers, String fileName)
            throws IOException {
        String ofName = fileName.substring(0, fileName.indexOf('.')) + ".seg";
        FileWriter fr = new FileWriter(ofName);
        int idx = 0;
        for (SpeakerCluster spk : speakers) {
            idx ++;
            ArrayList<Segment> segments = spk.getSpeakerIntervals();
            for (Segment seg : segments)
                fr.write(fileName + " " + 1 + " " + seg.getStartTime() / 10 + " " + seg.getLength() / 10
                        + "U U U Speaker" + idx + "\n");
        }
        fr.close();
    }

    /**
     * Test method for SpeakerIdentification, based on artificial input with
     * non-repeated speakers
     * 
     * @param vectorSize
     *            number of features (Segment.FEATURES_SIZE)
     * @param vectorsCount
     *            number of frames for each speaker
     * @param speakersCount
     *            number of speakers
     */
    public static void testDistinctSpeakerIdentification(int vectorSize, int vectorsCount, int speakersCount) {
        ArrayList<float[]> ret = generateDistinctSpeakers(vectorSize, vectorsCount, speakersCount);
        printIntervals(new SpeakerIdentification().cluster(ret));
    }

    /**
     * Test method for SpeakerIdentification, based on artificial input with
     * repeated speakers
     * 
     * @param vectorSize
     *            number of features (Segment.FEATURES_SIZE)
     * @param vectorsCount
     *            number of frames for each speaker
     * @param speakersCount
     *            number of speakers
     * @param repeatFactor
     *            number of times the input should be repeated
     */
    public static void testRepeatedSpeakerIdentification(int vectorSize, int vectorCount, int speakersCount,
            int repeatFactor) {
        ArrayList<float[]> lst = new ArrayList<float[]>();
        ArrayList<float[]> aux = generateDistinctSpeakers(vectorSize, vectorCount, speakersCount);
        for (int i = 0; i < repeatFactor; i++)
            lst.addAll(aux);
        printIntervals(new SpeakerIdentification().cluster(lst));
    }

    /**
     * Tests SpeakerIdentification on input file given as parameter.
     * 
     * @param inputFile
     *            the input file that needs to be diarized
     */
    public static void testSpeakerIdentification(String inputFile) throws IOException {
        InputStream stream = new FileInputStream(inputFile);
        ArrayList<SpeakerCluster> speakers = new SpeakerIdentification().cluster(stream);
        printIntervals(speakers);
        printSpeakerIntervals(speakers, inputFile);
    }

    /**
     * @param args
     *            -i input file name
     */
    public static void main(String[] args) throws IOException {
        String inputFile = null;
        for (int i = 0; i < args.length; i++)
            if (args[i].equals("-i"))
                inputFile = args[++i];
        testSpeakerIdentification(inputFile);
    }
}

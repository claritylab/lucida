package edu.cmu.sphinx.demo.speakerid;

import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;

import edu.cmu.sphinx.speakerid.Segment;
import edu.cmu.sphinx.speakerid.SpeakerCluster;
import edu.cmu.sphinx.speakerid.SpeakerIdentification;

public class SpeakerIdentificationDemo {

    /**
     * Returns string version of the given time in miliseconds 
     * @param seconds
     * @return time in format mm:ss
     */
    public static String time(int seconds) {
        return (seconds / 60000) + ":" + (Math.round((double) (seconds % 60000) / 1000));
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
        int idx = 0;
        for (SpeakerCluster spk : speakers) {
            idx++;
            ArrayList<Segment> segments = spk.getSpeakerIntervals();
            for (Segment seg : segments)
                System.out.println(fileName + " " + " " + time(seg.getStartTime()) + " "
                        + time(seg.getLength()) + " Speaker" + idx);
        }
    }

    public static void main(String[] args) throws IOException {
        SpeakerIdentification sd = new SpeakerIdentification();
        URL url = SpeakerIdentificationDemo.class.getResource("test.wav");
        ArrayList<SpeakerCluster> clusters = sd.cluster(url.openStream());
        printSpeakerIntervals(clusters, url.getPath());
    }
}

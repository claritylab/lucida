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

import java.util.TreeSet;
import java.util.ArrayList;
import java.util.Iterator;

import org.apache.commons.math3.linear.Array2DRowRealMatrix;

public class SpeakerCluster {
    private TreeSet<Segment> segmentSet;
    private double bicValue;

    public double getBicValue() {
        return bicValue;
    }

    public void setBicValue(double bicValue) {
        this.bicValue = bicValue;
    }

    protected Array2DRowRealMatrix featureMatrix;

    public Array2DRowRealMatrix getFeatureMatrix() {
        return featureMatrix;
    }

    public SpeakerCluster() {
        this.segmentSet = new TreeSet<Segment>();
    }

    public SpeakerCluster(Segment s, Array2DRowRealMatrix featureMatrix, double bicValue) {
        this.segmentSet = new TreeSet<Segment>();
        this.featureMatrix = new Array2DRowRealMatrix(featureMatrix.getData());
        this.bicValue = bicValue;
        addSegment(s);
    }

    public SpeakerCluster(SpeakerCluster c) {
        this.segmentSet = new TreeSet<Segment>();
        this.featureMatrix = new Array2DRowRealMatrix(c.getFeatureMatrix().getData());
        Iterator<Segment> it = c.segmentSet.iterator();
        while (it.hasNext())
            this.addSegment(it.next());
    }

    public TreeSet<Segment> getSegments() {
        return this.segmentSet;
    }

    public ArrayList<Segment> getArrayOfSegments() {
        Iterator<Segment> it = segmentSet.iterator();
        ArrayList<Segment> ret = new ArrayList<Segment>();
        while (it.hasNext())
            ret.add(it.next());
        return ret;
    }

    public Boolean addSegment(Segment s) {
        return this.segmentSet.add(s);
    }

    public Boolean removeSegment(Segment s) {
        return this.segmentSet.remove(s);
    }

    /**
     * Returns a 2 * n length array where n is the numbers of intervals assigned
     * to the speaker modeled by this cluster every pair of elements with
     * indexes (2 * i, 2 * i + 1) represents the start time and the length for
     * each interval
     * 
     * We may need a delay parameter to this function because the segments may
     * not be exactly consecutive
     */
    public ArrayList<Segment> getSpeakerIntervals() {
        Iterator<Segment> it = segmentSet.iterator();
        Segment curent = new Segment(0, 0), previous = it.next();
        int start = previous.getStartTime();
        int length = previous.getLength();
        int idx = 0;
        ArrayList<Segment> ret = new ArrayList<Segment>();
        ret.add(previous);
        while (it.hasNext()) {
            curent = it.next();
            start = ret.get(idx).getStartTime();
            length = ret.get(idx).getLength();
            if ((start + length) == curent.getStartTime()) {
                ret.set(idx, new Segment(start, length + curent.getLength()));
            } else {
                idx++;
                ret.add(curent);
            }
            previous = curent;
        }
        return ret;
    }

    public void mergeWith(SpeakerCluster target) throws NullPointerException {
        if (target == null)
            throw new NullPointerException();
        Iterator<Segment> it = target.segmentSet.iterator();
        while (it.hasNext()) {
            if (!this.addSegment(it.next()))
                System.out.println("Something doesn't work in mergeWith method, Cluster class");
        }
        int rowDim = featureMatrix.getRowDimension() + target.getFeatureMatrix().getRowDimension();
        int colDim = featureMatrix.getColumnDimension();
        Array2DRowRealMatrix combinedFeatures = new Array2DRowRealMatrix(rowDim, colDim);
        combinedFeatures.setSubMatrix(featureMatrix.getData(), 0, 0);
        combinedFeatures
                .setSubMatrix(target.getFeatureMatrix().getData(), featureMatrix.getRowDimension(), 0);
        bicValue = SpeakerIdentification.getBICValue(combinedFeatures);
        featureMatrix = new Array2DRowRealMatrix(combinedFeatures.getData());
    }
}

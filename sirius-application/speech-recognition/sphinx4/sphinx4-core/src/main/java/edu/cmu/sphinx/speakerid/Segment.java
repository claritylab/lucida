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

public class Segment implements Comparable<Segment> {
    public final static int FEATURES_SIZE = 13;

    public final static int FRAME_LENGTH = 10;

    private int startTime, length;

    public Segment(Segment ref) {
        this.startTime = ref.startTime;
        this.length = ref.length;
    }

    public Segment(int startTime, int length) {
        this.startTime = startTime;
        this.length = length;
    }

    public Segment(int startTime, int length, float[] features) {
        this.startTime = startTime;
        this.length = length;
    }

    public Segment() {
        this.startTime = this.length = 0;
    }

    public void setStartTime(int startTime) {
        this.startTime = startTime;
    }

    public void setLength(int length) {
        this.length = length;
    }

    public int getStartTime() {
        return this.startTime;
    }

    public int getLength() {
        return this.length;
    }

    public int equals(Segment ref) {
        return (this.startTime == ref.startTime) ? 1 : 0;
    }

    @Override
    public String toString() {
        return this.startTime + " " + this.length + "\n";
    }

    public int compareTo(Segment ref) {
        return (this.startTime - ref.startTime);
    }
}

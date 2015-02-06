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

package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import edu.cmu.sphinx.frontend.Data;

import java.util.Collection;


/**
 * Represents a composite senone. A composite senone consists of a set of all possible {@link Senone senones} for a
 * given state. CompositeSenones are used when the exact context of a senone is not known. The CompositeSenone
 * represents all the possible senones.
 * <p/>
 * This class currently only needs to be public for testing purposes.
 * <p/>
 * Note that all scores are maintained in LogMath log base
 */

@SuppressWarnings("serial")
public class CompositeSenone extends ScoreCachingSenone {

    private final static int MAX_SENONES = 20000;
    private final static boolean wantMaxScore = true;
    private final Senone[] senones;
    private final float weight;

    /**
     * a factory method that creates a CompositeSenone from a list of senones.
     *
     * @param senoneCollection the Collection of senones
     * @return a composite senone
     */
    public static CompositeSenone create(Collection<Senone> senoneCollection,
                                         float weight) {
        return new CompositeSenone(senoneCollection.toArray(new Senone[senoneCollection.size()]), weight);
    }


    /**
     * Constructs a CompositeSenone given the set of constituent senones
     *
     * @param senones the set of constituent senones
     */
    public CompositeSenone(Senone[] senones, float weight) {
        this.senones = senones;
        this.weight = weight;
        System.out.print(" " + senones.length);
    }


    /**
     * Dumps this senone
     *
     * @param msg annotation for the dump
     */
    public void dump(String msg) {
        System.out.println("   CompositeSenone " + msg + ": ");
        for (Senone senone : senones) {
            senone.dump("   ");
        }
    }


    /**
     * Calculates the composite senone score. Typically this is the best score for all of the constituent senones
     */
    @Override
    public float calculateScore(Data feature) {
        float logScore;
        if (wantMaxScore) {
            logScore = -Float.MAX_VALUE;
            for (Senone senone : senones) {
                logScore = Math.max(logScore, senone.getScore(feature));
            }
        } else { // average score
            logScore = 0.0f;
            for (Senone senone : senones) {
                logScore += senone.getScore(feature);
            }
            logScore = logScore / senones.length;
        }
        return logScore + weight;
    }


    /**
     * Calculate scores for each component in the senone's distribution. Not yet implemented.
     *
     * @param feature the current feature
     * @return the score for the feature in LogMath
     */
    public float[] calculateComponentScore(Data feature) {
        assert false : "Not implemented!";
        return null;
    }


    /**
     * Returns the set of senones that compose this composite senone. This method is only needed for unit testing.
     *
     * @return the array of senones.
     */
    public Senone[] getSenones() {
        return senones;
    }


    /**
     * Determines if two objects are equal
     *
     * @param o the object to compare to this.
     * @return true if the objects are equal
     */
    @Override
    public boolean equals(Object o) {
        if (!(o instanceof Senone)) {
            return false;
        }
        Senone other = (Senone) o;
        return this.getID() == other.getID();
    }


    /**
     * Returns the hashcode for this object
     *
     * @return the hashcode
     */
    @Override
    public int hashCode() {
        long id = getID();
        int high = (int) ((id >> 32));
        int low = (int) (id);
        return high + low;
    }


    /**
     * Gets the ID for this senone
     *
     * @return the senone id
     */
    public long getID() {
        long factor = 1L;
        long id = 0L;
        for (Senone senone : senones) {
            id += senone.getID() * factor;
            factor = factor * MAX_SENONES;
        }
        return id;
    }


    /**
     * Retrieves a string form of this object
     *
     * @return the string representation of this object
     */
    @Override
    public String toString() {
        return "senone id: " + getID();
    }
}

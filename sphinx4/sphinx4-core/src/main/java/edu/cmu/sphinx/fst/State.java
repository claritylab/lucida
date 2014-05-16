/**
 * 
 * Copyright 1999-2012 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.fst;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

/**
 * The fst's mutable state implementation.
 * 
 * Holds its outgoing {@link edu.cmu.sphinx.fst.Arc} objects in an ArrayList
 * allowing additions/deletions
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class State {

    // State's Id
    protected int id = -1;

    // Final weight
    private float fnlWeight;

    // Outgoing arcs
    private ArrayList<Arc> arcs = null;

    // initial number of arcs
    protected int initialNumArcs = -1;

    /**
     * Default Constructor
     */
    protected State() {
        arcs = new ArrayList<Arc>();
    }

    /**
     * Constructor specifying the state's final weight
     * 
     * @param fnlWeight
     */
    public State(float fnlWeight) {
        this();
        this.fnlWeight = fnlWeight;
    }

    /**
     * Constructor specifying the initial capacity of the arc's ArrayList (this
     * is an optimization used in various operations)
     * 
     * @param initialNumArcs
     */
    public State(int initialNumArcs) {
        this.initialNumArcs = initialNumArcs;
        if (initialNumArcs > 0) {
            arcs = new ArrayList<Arc>(initialNumArcs);
        }
    }

    /**
     * Shorts the arc's ArrayList based on the provided Comparator
     */
    public void arcSort(Comparator<Arc> cmp) {
        Collections.sort(arcs, cmp);
    }

    /**
     * Get the state's final Weight
     */
    public float getFinalWeight() {
        return fnlWeight;
    }

    /**
     * Set the state's arcs ArrayList
     * 
     * @param arcs the arcs ArrayList to set
     */
    public void setArcs(ArrayList<Arc> arcs) {
        this.arcs = arcs;
    }

    /**
     * Set the state's final weight
     * 
     * @param fnlfloat the final weight to set
     */
    public void setFinalWeight(float fnlfloat) {
        this.fnlWeight = fnlfloat;
    }

    /**
     * Get the state's id
     */
    public int getId() {
        return id;
    }

    /**
     * Get the number of outgoing arcs
     */
    public int getNumArcs() {
        return this.arcs.size();
    }

    /**
     * Add an outgoing arc to the state
     * 
     * @param arc the arc to add
     */
    public void addArc(Arc arc) {
        this.arcs.add(arc);
    }

    /**
     * Get an arc based on it's index the arcs ArrayList
     * 
     * @param index the arc's index
     * @return the arc
     */
    public Arc getArc(int index) {
        return this.arcs.get(index);
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        State other = (State) obj;
        if (id != other.id)
            return false;
        if (!(fnlWeight == other.fnlWeight)) {
            if (Float.floatToIntBits(fnlWeight) != Float
                    .floatToIntBits(other.fnlWeight))
                return false;
        }
        if (arcs == null) {
            if (other.arcs != null)
                return false;
        } else if (!arcs.equals(other.arcs))
            return false;
        return true;
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("(" + id + ", " + fnlWeight + ")");
        return sb.toString();
    }

    /**
     * Delete an arc based on its index
     * 
     * @param index the arc's index
     * @return the deleted arc
     */
    public Arc deleteArc(int index) {
        return this.arcs.remove(index);
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#hashCode()
     */
    @Override
    public int hashCode() {
        return id * 991;
    }

    /**
     * Set an arc at the specified position in the arcs' ArrayList.
     * 
     * @param index the position to the arcs' array
     * @param arc the arc value to set
     */
    public void setArc(int index, Arc arc) {
        arcs.set(index, arc);
    }

}

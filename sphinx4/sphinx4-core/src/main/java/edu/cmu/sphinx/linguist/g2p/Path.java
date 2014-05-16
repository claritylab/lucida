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

package edu.cmu.sphinx.linguist.g2p;

import java.util.ArrayList;

import edu.cmu.sphinx.fst.semiring.Semiring;

/**
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class Path {
    // the path
    private ArrayList<String> path;
    
    // the path's cost
    private float cost;
    
    // the paths' semiring
    private Semiring semiring;

    /**
     * Create a Path instance with specified path and semiring elements
     */
    public Path(ArrayList<String> path, Semiring semiring) {
        this.path = path;
        this.semiring = semiring;
        cost = this.semiring.zero();
    }

    /**
     * Create a Path instance with specified semiring element
     */
    public Path(Semiring semiring) {
        this(new ArrayList<String>(), semiring);
    }

    /**
     * Get the path
     */
    public ArrayList<String> getPath() {
        return path;
    }

    /**
     * Get the paths' cost
     */
    public float getCost() {
        return cost;
    }

    /**
     * Set the paths' cost
     */
    public void setCost(float cost) {
        this.cost = cost;
    }

    /**
     * Get the paths
     */
    public void setPath(ArrayList<String> path) {
        this.path = path;
    }

    /*
     * (non-Javadoc)
     * 
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(cost + "\t");
        for (String s : path) {
            sb.append(s);

            if (!s.equals(path.get(path.size() - 1))) {
                sb.append(" ");
            }
        }
        return sb.toString();
    }
}

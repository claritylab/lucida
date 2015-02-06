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
package edu.cmu.sphinx.fst.utils;

/**
 * Pairs two elements
 * 
 * Original code obtained by
 * http://stackoverflow.com/questions/521171/a-java-collection-of-value
 * -pairs-tuples
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class Pair<L, R> {

    // The left element
    private L left;

    // The right element
    private R right;

    /**
     * Constructor specifying the left and right elements of the Pair.
     */
    public Pair(L left, R right) {
        this.left = left;
        this.right = right;
    }

    /**
     * Set the left element of the Pair
     */
    public void setLeft(L left) {
        this.left = left;
    }

    /**
     * Set the right element of the Pair
     */
    public void setRight(R right) {
        this.right = right;
    }

    /**
     * Get the left element of the Pair
     */
    public L getLeft() {
        return left;
    }

    /**
     * Get the right element of the Pair
     */
    public R getRight() {
        return right;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + left.hashCode();
        result = prime * result + right.hashCode();
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        @SuppressWarnings("unchecked")
        Pair<L,R> other = (Pair<L,R>) obj;
        if (!left.equals(other.left))
            return false;
        if (!right.equals(other.right))
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
        return "(" + left + ", " + right + ")";
    }

}

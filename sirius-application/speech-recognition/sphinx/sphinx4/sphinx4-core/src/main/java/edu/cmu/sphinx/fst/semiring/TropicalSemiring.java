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

package edu.cmu.sphinx.fst.semiring;

/**
 * Tropical semiring implementation.
 * 
 * @author "John Salatas <jsalatas@users.sourceforge.net>"
 * 
 */
public class TropicalSemiring extends Semiring {

    private static final long serialVersionUID = 2711172386738607866L;

    // zero value
    private static float zero = Float.POSITIVE_INFINITY;

    // one value
    private static float one = 0.f;

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.fst.weight.AbstractSemiring#Plus(edu.cmu.sphinx.fst.weight
     * .float, edu.cmu.sphinx.fst.weight.float)
     */
    @Override
    public float plus(float w1, float w2) {
        if (!isMember(w1) || !isMember(w2)) {
            return Float.NEGATIVE_INFINITY;
        }

        return w1 < w2 ? w1 : w2;
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.fst.weight.AbstractSemiring#Times(edu.cmu.sphinx.fst.weight
     * .float, edu.cmu.sphinx.fst.weight.float)
     */
    @Override
    public float times(float w1, float w2) {
        if (!isMember(w1) || !isMember(w2)) {
            return Float.NEGATIVE_INFINITY;
        }

        return w1 + w2;
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.fst.weight.AbstractSemiring#Divide(edu.cmu.sphinx.fst.
     * weight.float, edu.cmu.sphinx.fst.weight.float)
     */
    @Override
    public float divide(float w1, float w2) {
        if (!isMember(w1) || !isMember(w2)) {
            return Float.NEGATIVE_INFINITY;
        }

        if (w2 == zero) {
            return Float.NEGATIVE_INFINITY;
        } else if (w1 == zero) {
            return zero;
        }

        return w1 - w2;
    }

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.fst.weight.AbstractSemiring#zero()
     */
    @Override
    public float zero() {
        return zero;
    }

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.fst.weight.AbstractSemiring#one()
     */
    @Override
    public float one() {
        return one;
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.fst.weight.AbstractSemiring#isMember(edu.cmu.sphinx.fst
     * .weight.float)
     */
    @Override
    public boolean isMember(float w) {
        return (!Float.isNaN(w)) // not a NaN
                && (w != Float.NEGATIVE_INFINITY); // and different from -inf
    }

    /*
     * (non-Javadoc)
     * 
     * @see edu.cmu.sphinx.fst.semiring.Semiring#reverse(float)
     */
    @Override
    public float reverse(float w1) {
        return w1;
    }
}

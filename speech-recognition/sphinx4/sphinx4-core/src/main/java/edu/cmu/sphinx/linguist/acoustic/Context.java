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

package edu.cmu.sphinx.linguist.acoustic;

import java.io.Serializable;

/** Represents  the context for a unit */
@SuppressWarnings("serial")
public class Context implements Serializable {

    /** Represents an empty context */
    public final static Context EMPTY_CONTEXT = new Context();


    /** No instantiations allowed */
    protected Context() {
    }


    /**
     * Checks to see if there is a partial match with the given context. For a simple context such as this we always
     * match.
     *
     * @param context the context to check
     * @return true if there is a partial match
     */
    public boolean isPartialMatch(Context context) {
        return true;
    }


    /** Provides a string representation of a context */
    @Override
    public String toString() {
        return "";
    }


    /**
     * Determines if an object is equal to this context
     *
     * @param o the object to check
     * @return true if the objects are equal
     */
    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        } else if (o instanceof Context) {
            Context otherContext = (Context) o;
            return toString().equals(otherContext.toString());
        } else {
            return false;
        }
    }


    /**
     * calculates a hashCode for this context. Since we defined an equals for context, we must define a hashCode as
     * well
     *
     * @return the hashcode for this object
     */
    @Override
    public int hashCode() {
        return toString().hashCode();
    }
}

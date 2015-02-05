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

/** Represents  the context for a unit */
@SuppressWarnings("serial")
public class LeftRightContext extends Context {

    String stringRepresentation;
    final Unit[] leftContext;
    final Unit[] rightContext;

    /**
     * Creates a LeftRightContext
     *
     * @param leftContext  the left context or null if no left context
     * @param rightContext the right context or null if no right context
     */
    private LeftRightContext(Unit[] leftContext, Unit[] rightContext) {
        this.leftContext = leftContext;
        this.rightContext = rightContext;
    }

    /** Provides a string representation of a context */
    @Override
    public String toString() {
        return getContextName(leftContext) + ',' + getContextName(rightContext);
    }

    /**
     * Factory method for creating a left/right context
     *
     * @param leftContext  the left context or null if no left context
     * @param rightContext the right context or null if no right context
     * @return a left right context
     */
    public static LeftRightContext get(Unit[] leftContext, Unit[] rightContext) { 
        return new LeftRightContext(leftContext, rightContext);
    }

    /**
     * Retrieves the left context for this unit
     *
     * @return the left context
     */
    public Unit[] getLeftContext() {
        return leftContext;
    }

    /**
     * Retrieves the right context for this unit
     *
     * @return the right context
     */
    public Unit[] getRightContext() {
        return rightContext;
    }

    /**
     * Gets the context name for a particular array of units
     *
     * @param context the context
     * @return the context name
     */
    public static String getContextName(Unit[] context) {
        if (context == null)
            return "*";
        if (context.length == 0)
            return "(empty)";
        StringBuilder sb = new StringBuilder();
        for (Unit unit : context) {
            sb.append(unit == null ? null : unit.getName()).append('.');
        }
        sb.setLength(sb.length() - 1); // remove last period
        return sb.toString();
    }

    /**
     * Checks to see if there is a partial match with the given context. If both contexts are LeftRightContexts then  a
     * left or right context that is null is considered a wild card and matches anything, othewise the contexts must
     * match exactly. Anything matches the Context.EMPTY_CONTEXT
     *
     * @param context the context to check
     * @return true if there is a partial match
     */
    @Override
    public boolean isPartialMatch(Context context) {
        if (context instanceof LeftRightContext) {
            LeftRightContext lrContext = (LeftRightContext)context;
            Unit[] lc = lrContext.getLeftContext();
            Unit[] rc = lrContext.getRightContext();

            return (lc == null || leftContext == null || Unit.isContextMatch(lc, leftContext))
                && (rc == null || rightContext == null || Unit.isContextMatch(rc, rightContext));
        }
        return context == Context.EMPTY_CONTEXT && leftContext == null && rightContext == null;
    }

}

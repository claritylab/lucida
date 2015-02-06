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

import java.util.Arrays;

/** Represents a unit of speech. Units may represent phones, words or any other suitable unit */

public class Unit {

    public final static Unit[] EMPTY_ARRAY = new Unit[0];

    private final String name;
    private final boolean filler;
    private final boolean silence;
    private final int baseID;
    private final Unit baseUnit;
    private final Context context;

    private volatile String key;

    /**
     * Constructs a context independent unit. Constructors are package private, use the UnitManager to create and access
     * units.
     *
     * @param name   the name of the unit
     * @param filler <code>true</code> if the unit is a filler unit
     * @param id     the base id for the unit
     */
    Unit(String name, boolean filler, int id) {
        this.name = name;
        this.filler = filler;
        this.silence = name.equals(UnitManager.SILENCE_NAME);
        this.baseID = id;
        this.baseUnit = this;
        this.context = Context.EMPTY_CONTEXT;
    }

    /**
     * Constructs a context dependent unit. Constructors are package private, use the UnitManager to create and access
     * units.
     *
     * @param baseUnit the base id for the unit
     * @param filler   <code>true</code> if the unit is a filler unit
     * @param context  the context for this unit
     */
    Unit(Unit baseUnit, boolean filler, Context context) {
        this.name = baseUnit.getName();
        this.filler = filler;
        this.silence = name.equals(UnitManager.SILENCE_NAME);
        this.baseID = baseUnit.getBaseID();
        this.baseUnit = baseUnit;
        this.context = context;
    }

    /**
     * Gets the name for this unit
     *
     * @return the name for this unit
     */
    public String getName() {
        return name;
    }

    /**
     * Determines if this unit is a filler unit
     *
     * @return <code>true</code> if the unit is a filler unit
     */
    public boolean isFiller() {
        return filler;
    }

    /**
     * Determines if this unit is the silence unit
     *
     * @return true if the unit is the silence unit
     */
    public boolean isSilence() {
        return silence;
    }

    /**
     * Gets the base ID for this unit
     *
     * @return the id
     */
    public int getBaseID() {
        return baseID;
    }

    /**
     * Gets the  base unit associated with this HMM
     *
     * @return the unit associated with this HMM
     */
    public Unit getBaseUnit() {
        return baseUnit;
    }

    /**
     * Returns the context for this unit
     *
     * @return the context for this unit (or null if context independent)
     */
    public Context getContext() {
        return context;
    }

    /**
     * Determines if this unit is context dependent
     *
     * @return true if the unit is context dependent
     */
    public boolean isContextDependent() {
        return getContext() != Context.EMPTY_CONTEXT;
    }

    /** gets the key for this unit
     * @return the key
     */
    private String getKey() {
        return toString();
    }

    /**
     * Checks to see of an object is equal to this unit
     *
     * @param o the object to check
     * @return true if the objects are equal
     */
    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        } else if (o instanceof Unit) {
            Unit otherUnit = (Unit) o;
            return getKey().equals(otherUnit.getKey());
        } else {
            return false;
        }
    }

    /**
     * calculates a hashCode for this unit. Since we defined an equals for Unit, we must define a hashCode as well
     *
     * @return the hashcode for this object
     */
    @Override
    public int hashCode() {
        return getKey().hashCode();
    }

    /**
     * Converts to a string
     *
     * @return string version
     */
    @Override
    public String toString() {
        if (key == null) {
            if (context == Context.EMPTY_CONTEXT) {
                key = (filler ? "*" : "") + name;
            } else {
                key = (filler ? "*" : "") + name + '[' + context + ']';
            }
        }
        return key;
    }

    /**
     * Checks to see if the given unit with associated contexts is a partial match for this unit.   Zero, One or both
     * contexts can be null. A null context matches any context
     *
     * @param name    the name of the unit
     * @param context the  context to match against
     * @return true if this unit matches the name and non-null context
     */
    public boolean isPartialMatch(String name, Context context) {
        return getName().equals(name) && context.isPartialMatch(this.context);
    }

    /**
     * Creates and returns an empty context with the given size. The context is padded with SIL filler
     *
     * @param size the size of the context
     * @return the context
     */

    public static Unit[] getEmptyContext(int size) {
        Unit[] context = new Unit[size];
        Arrays.fill(context, UnitManager.SILENCE);
        return context;
    }

    /**
     * Checks to see that there is 100% overlap in the given contexts
     *
     * @param a context to check for a match
     * @param b context to check for a match
     * @return <code>true</code> if the contexts match
     */
    public static boolean isContextMatch(Unit[] a, Unit[] b) {
        if (a == null || b == null) {
            return a == b;
        } else if (a.length != b.length) {
            return false;
        } else {
            for (int i = 0; i < a.length; i++) {
                if (!a[i].getName().equals(b[i].getName())) {
                    return false;
                }
            }
            return true;
        }
    }
}

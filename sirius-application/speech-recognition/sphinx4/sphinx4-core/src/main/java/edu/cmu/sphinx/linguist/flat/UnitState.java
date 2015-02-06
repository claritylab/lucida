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

package edu.cmu.sphinx.linguist.flat;

import edu.cmu.sphinx.linguist.UnitSearchState;
import edu.cmu.sphinx.linguist.acoustic.HMMPosition;
import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.linguist.acoustic.UnitManager;

/** Represents a unit in an SentenceHMMS */
@SuppressWarnings("serial")
public class UnitState extends SentenceHMMState implements UnitSearchState {

    private final Unit unit;
    private HMMPosition position = HMMPosition.INTERNAL;


    /**
     * Creates a UnitState. Gets the left and right contexts from the unit itself.
     *
     * @param parent the parent state
     * @param which  the index of the given state
     * @param unit   the unit associated with this state
     */
    public UnitState(PronunciationState parent, int which, Unit unit) {
        super("U", parent, which);
        this.unit = unit;
        Unit[] units = parent.getPronunciation().getUnits();
        int length = units.length;

        // If the last phone is SIL, then we should be using
        // a word-ending phone for the last phone. Decrementing
        // length will make the phone before SIL the last phone.

        if (units[length - 1] == UnitManager.SILENCE && length > 1) {
            length--;
        }

        if (length == 1) {
            position = HMMPosition.SINGLE;
        } else if (which == 0) {
            position = HMMPosition.BEGIN;
        } else if (which == length - 1) {
            position = HMMPosition.END;
        }
    }


    /**
     * Creates a UnitState with the given unit and HMM position.
     *
     * @param unit     the unit associated with this state
     * @param position the HMM position of this unit
     */
    public UnitState(Unit unit, HMMPosition position) {
        this.unit = unit;
        this.position = position;
    }


    /**
     * Gets the unit associated with this state
     *
     * @return the unit
     */
    public Unit getUnit() {
        return unit;
    }


    /**
     * Returns true if this unit is the last unit of the pronunciation
     *
     * @return <code>true</code> if the unit is the last unit
     */
    public boolean isLast() {
        return position == HMMPosition.SINGLE || position == HMMPosition.END;
    }


    /**
     * Gets the name for this state
     *
     * @return the name for this state
     */
    @Override
    public String getName() {
        return super.getName() + '<' + unit + '>';
    }


    /**
     * Returns the value signature of this unit
     *
     * @return the value signature
     */
    @Override
    public String getValueSignature() {
        return unit.toString();
    }


    /**
     * Gets the pretty name for this unit sate
     *
     * @return the pretty name
     */
    @Override
    public String getPrettyName() {
        return unit.toString();
    }


    /**
     * Retrieves a short label describing the type of this state. Typically, subclasses of SentenceHMMState will
     * implement this method and return a short (5 chars or less) label
     *
     * @return the short label.
     */
    @Override
    public String getTypeLabel() {
        return "Unit";
    }


    /**
     * Gets the position for this unit
     *
     * @return the position for this unit
     */
    public HMMPosition getPosition() {
        return position;
    }


    @Override
    public boolean isUnit() {
        return true;
    }


    /**
     * Returns the state order for this state type
     *
     * @return the state order
     */
    @Override
    public int getOrder() {
        return 5;
    }
}



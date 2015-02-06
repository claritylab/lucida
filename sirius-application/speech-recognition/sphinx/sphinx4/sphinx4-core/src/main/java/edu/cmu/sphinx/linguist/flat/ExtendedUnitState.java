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

import edu.cmu.sphinx.linguist.acoustic.Unit;

/**
 * A unit state that modifies how the unit state is cached.  Caching keys are generated from the full name for the
 * sentence hmm. The default behavior for the unit (and all sentence hmms) is to generate the full name by combining the
 * name for this unit with the name of the parent.  For the simple linguist, this is undesirable, because there are many
 * different names for the parent pronunciations (differing contexts).  We want to be able to combine units that have
 * identical names and context and are in the same position in the same pronunciation.  By defining getFullName to
 * combine the name and the pronunciation index we allow units with identical contexts in the same position in a
 * pronunciation to be combined.
 */
@SuppressWarnings("serial")
public class ExtendedUnitState extends UnitState {

    /**
     * Creates a UnitState. Gets the left and right contexts from the unit itself.
     *
     * @param parent the parent state
     * @param which  the index of the given state
     * @param unit   the unit associated with this state
     */
    public ExtendedUnitState(PronunciationState parent, int which, Unit unit) {
        super(parent, which, unit);
    }


    /**
     * Gets the fullName for this state
     *
     * @return the full name for this state
     */
    @Override
    public String getFullName() {
        return getName() + " in P" + getParent().getWhich();
    }
}

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

import edu.cmu.sphinx.util.props.Configurable;

import java.io.IOException;
import java.util.Iterator;
import java.util.Properties;
import edu.cmu.sphinx.util.props.S4String;

/** Represents the generic interface to the Acoustic Model for sphinx4 */
public interface AcousticModel extends Configurable {

    /** The directory where the acoustic model data can be found. */
    @S4String(defaultValue = ".")
    public final static String PROP_LOCATION = "location";

    /**
     * Gets this acoustic model ready to use, allocating all necessary resources.
     *
     * @throws IOException if the model could not be loaded
     */
    public void allocate() throws IOException;


    /** Deallocates previously allocated resources */
    public void deallocate();


    /**
     * Returns the name of this AcousticModel, or null if it has no name.
     *
     * @return the name of this AcousticModel, or null if it has no name
     */
    public String getName();


    /**
     * Given a unit, returns the HMM that best matches the given unit. If exactMatch is false and an exact match is not
     * found, then different word positions are used. If any of the contexts are non-silence filler units. a silence
     * filler unit is tried instead.
     *
     * @param unit       the unit of interest
     * @param position   the position of the unit of interest
     * @param exactMatch if true, only an exact match is acceptable.
     * @return the HMM that best matches, or null if no match could be found.
     */
    public HMM lookupNearestHMM(Unit unit, HMMPosition position,
                                boolean exactMatch);


    /**
     * Returns an iterator that can be used to iterate through all the HMMs of the acoustic model
     *
     * @return an iterator that can be used to iterate through all HMMs in the model. The iterator returns objects of
     *         type <code>HMM</code>.
     */
    public Iterator<HMM> getHMMIterator();


    /**
     * Returns an iterator that can be used to iterate through all the CI units in the acoustic model
     *
     * @return an iterator that can be used to iterate through all CI units. The iterator returns objects of type
     *         <code>Unit</code>
     */
    public Iterator<Unit> getContextIndependentUnitIterator();


    /**
     * Returns the size of the left context for context dependent units
     *
     * @return the left context size
     */
    public int getLeftContextSize();


    /**
     * Returns the size of the right context for context dependent units
     *
     * @return the left context size
     */
    public int getRightContextSize();


    /**
     * Returns the properties of this acoustic model.
     *
     * @return the properties of this acoustic model
     */
    public Properties getProperties();
}


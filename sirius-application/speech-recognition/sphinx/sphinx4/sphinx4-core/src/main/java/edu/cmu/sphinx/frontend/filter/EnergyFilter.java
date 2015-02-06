/*
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2002-2004 Sun Microsystems, Inc.  
 * Portions Copyright 2002-2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.frontend.filter;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Double;

/**
 * EnergyFilter silently drops zero energy frames from the stream.
 * This is a deterministic alternative to {@link Dither}.
 */
public class EnergyFilter extends BaseDataProcessor {

    /** If energy is below this threshold frame is dropped */
    @S4Double(defaultValue = 2.0)
    public static final String PROP_MAX_ENERGY = "maxEnergy";
    private double maxEnergy;

    public EnergyFilter(  double maxEnergy) {
        initLogger();
        this.maxEnergy = maxEnergy;
        initialize();
    }

    public EnergyFilter( ) {

    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        maxEnergy = ps.getDouble(PROP_MAX_ENERGY);        
    }


    /**
     * Returns the next DoubleData object, skipping frames with zero energy
     *
     * @return the next available DoubleData object, or null if no Data is available
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     *          if a data processing error occurred
     */
    @Override
    public Data getData() throws DataProcessingException {
        float energy = 0;
        Data input = null;
        do {
            input = getPredecessor().getData();
            if (input == null || !(input instanceof DoubleData))
                return input;
            energy = 0.0f;
            for (double d : ((DoubleData) input).getValues()) {
                energy += d * d;
            }
        } while (energy < maxEnergy);
        
        return input;
    }
}
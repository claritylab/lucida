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

package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.linguist.acoustic.Unit;

import java.io.IOException;
import java.util.Map;
import java.util.Properties;

/** Generic interface for a loader of acoustic models */
public interface Loader extends Configurable {

    /**
     * Loads the acoustic model.
     *
     * @throws IOException if an error occurs while loading the model
     */
    public void load() throws IOException;

    /**
     * Gets the pool of means for this loader.
     *
     * @return the pool
     */
    public Pool<float[]> getMeansPool();

    /**
     * Gets the means transformation matrix pool.
     *
     * @return the pool
     */
    public Pool<float[][]> getMeansTransformationMatrixPool();

    /**
     * Gets the means transformation vectors pool.
     *
     * @return the pool
     */
    public Pool<float[]> getMeansTransformationVectorPool();

    /**
     * Gets the variance pool.
     *
     * @return the pool
     */
    public Pool<float[]> getVariancePool();

    /**
     * Gets the variance transformation matrix pool.
     *
     * @return the pool
     */
    public Pool<float[][]> getVarianceTransformationMatrixPool();

    /**
     * Gets the variance transformation vectors pool.
     *
     * @return the pool
     */
    public Pool<float[]> getVarianceTransformationVectorPool();

    /**
     * Gets the mixture weight pool.
     *
     * @return the pool
     */
    public Pool<float[]> getMixtureWeightPool();

    /**
     * Gets the transition matrix pool.
     *
     * @return the pool
     */
    public Pool<float[][]> getTransitionMatrixPool();

    /**
     * Gets the transformation matrix.
     *
     * @return the matrix
     */
    public float[][] getTransformMatrix();

    /**
     * Gets the senone pool for this loader.
     *
     * @return the pool
     */
    public Pool<Senone> getSenonePool();

    /**
     * Returns the HMM Manager for this loader.
     *
     * @return the HMM Manager
     */
    public HMMManager getHMMManager();

    /**
     * Returns the map of context indepent units. The map can be accessed by unit name.
     *
     * @return the map of context independent units
     */
    public Map<String, Unit> getContextIndependentUnits();

    /** logs information about this loader */
    public void logInfo();

    /**
     * Returns the size of the left context for context dependent units.
     *
     * @return the left context size
     */
    public int getLeftContextSize();

    /**
     * Returns the size of the right context for context dependent units.
     *
     * @return the left context size
     */
    public int getRightContextSize();
    
    /**
     * Returns the model properties
     */
    public Properties getProperties();
}

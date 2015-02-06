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


import java.util.*;
import java.util.logging.Logger;

/** Used to pool shared objects in the acoustic model */
public class Pool<T> {

    public enum Feature { NUM_SENONES, NUM_GAUSSIANS_PER_STATE, NUM_STREAMS }

    private final String name;
    private final List<T> pool;
    private final Map<Feature, Integer> features = new EnumMap<Feature, Integer>(Feature.class);

    /**
     * Creates a new pool.
     *
     * @param name the name of the pool
     */
    public Pool(String name) {
        this.name = name;
        pool = new ArrayList<T>();
    }

    /**
     * Returns the pool's name.
     *
     * @return the pool name
     */
    public String getName() {
        return name;
    }

    /**
     * Returns the object with the given ID from the pool.
     *
     * @param id the id of the object
     * @return the object
     * @throws IndexOutOfBoundsException if the ID is out of range
     */
    public T get(int id) {
        return pool.get(id);
    }

    /**
     * Returns the ID of a given object from the pool.
     *
     * @param object the object
     * @return the index
     */
    public int indexOf(T object) {
        return pool.indexOf(object);
    }

    /**
     * Places the given object in the pool.
     *
     * @param id a unique ID for this object
     * @param o  the object to add to the pool
     */
    public void put(int id, T o) {
        if (id == pool.size()) {
            pool.add(o);
        } else {
            pool.set(id, o);
        }
    }

    /**
     * Retrieves the size of the pool.
     *
     * @return the size of the pool
     */
    public int size() {
        return pool.size();
    }

    /**
     * Dump information on this pool to the given logger.
     *
     * @param logger the logger to send the info to
     */
    public void logInfo(Logger logger) {
        logger.info("Pool " + name + " Entries: " + size());
    }

    /**
     * Sets a feature for this pool.
     *
     * @param feature feature to set
     * @param value the value for the feature
     */
    public void setFeature(Feature feature, int value) {
        features.put(feature, value);
    }

    /**
     * Retrieves a feature from this pool.
     *
     * @param feature feature to get
     * @param defaultValue the defaultValue for the pool
     * @return the value for the feature
     */
    public int getFeature(Feature feature, int defaultValue) {
        Integer val = features.get(feature);
        return val == null ? defaultValue : val;
    }
}

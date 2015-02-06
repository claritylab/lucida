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


package edu.cmu.sphinx.frontend;

import java.util.HashMap;
import java.util.Map;

/**
 * Indicates events like beginning or end of data, data dropped, quality changed, etc.. It implements the Data
 * interface, and it will pass between DataProcessors to inform them about the Data that is passed between
 * DataProcessors.
 *
 * @see Data
 * @see DataProcessor
 */
@SuppressWarnings("serial")
public class Signal implements Data {

    /** the time this Signal was issued. */
    private final long time;

    /**
     * A (lazily initialized) collection of names properties of this signal. This collection might contain infos about
     * the file being processed, shift-size of frame-length of the windowing process, etc.
     */
    private Map<String, Object> props;


    /**
     * Constructs a Signal with the given name.
     *
     * @param time the time this Signal is created
     */
    protected Signal(long time) {
        this.time = time;
    }


    /**
     * Returns the time this Signal was created.
     *
     * @return the time this Signal was created
     */
    public long getTime() {
        return time;
    }


    /** Returns the properties associated to this signal.
     */
    public synchronized Map<String, Object> getProps() {
        if (props == null)
            props = new HashMap<String, Object>();

        return props;
    }
}

/*
 * Copyright 2004 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */


package edu.cmu.sphinx.frontend;

import edu.cmu.sphinx.util.props.ConfigurableAdapter;
import edu.cmu.sphinx.util.Timer;
import edu.cmu.sphinx.util.TimerPool;
import edu.cmu.sphinx.util.Utilities;

/**
 * An abstract DataProcessor implementing elements common to all concrete DataProcessors, such as name, predecessor, and
 * timer.
 */
public abstract class BaseDataProcessor extends ConfigurableAdapter implements DataProcessor {

    private DataProcessor predecessor;
    private Timer timer;

    public BaseDataProcessor() {
    }

    /**
     * Returns the processed Data output.
     *
     * @return an Data object that has been processed by this DataProcessor
     * @throws DataProcessingException if a data processor error occurs
     */
    public abstract Data getData() throws DataProcessingException;


    /** Initializes this DataProcessor. This is typically called after the DataProcessor has been configured. */
    public void initialize() {
    }


    /**
     * Returns the predecessor DataProcessor.
     *
     * @return the predecessor
     */
    public DataProcessor getPredecessor() {
        return predecessor;
    }


    /**
     * Returns the timer this DataProcessor uses.
     *
     * @return the timer
     */
    public synchronized Timer getTimer() {
        if(timer == null)
            this.timer = TimerPool.getTimer(this, Utilities.getReadable(getName()));
            
        return timer;
    }


    /**
     * Sets the predecessor DataProcessor. This method allows dynamic reconfiguration of the front end.
     *
     * @param predecessor the new predecessor of this DataProcessor
     */
    public void setPredecessor(DataProcessor predecessor) {
        this.predecessor = predecessor;
    }
}

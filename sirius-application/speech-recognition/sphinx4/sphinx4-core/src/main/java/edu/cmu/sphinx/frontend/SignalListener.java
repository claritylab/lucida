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


/** The listener interface for being informed when a {@link Signal Signal} is generated. */
public interface SignalListener {

    /**
     * Method called when a signal is detected
     *
     * @param signal the signal
     */
    public void signalOccurred(Signal signal);
}


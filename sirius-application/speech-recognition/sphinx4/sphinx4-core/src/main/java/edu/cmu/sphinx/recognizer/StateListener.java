/*
 * 
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.recognizer;

import edu.cmu.sphinx.util.props.Configurable;

import java.util.EventListener;

/** The listener interface for receiving recognizer status events */
public interface StateListener extends EventListener, Configurable {

    /**
     * Called when the status has changed.
     *
     * @param status the new status
     */
    public void statusChanged(Recognizer.State status);
}

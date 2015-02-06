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

import java.io.Serializable;

/**
 * Implements the interface for all Data objects that passes between
 * DataProcessors.
 *
 * Subclass of Data can contain the actual data, or be a signal
 * (e.g., data start, data end, speech start, speech end).
 *
 * @see Data
 * @see FrontEnd
 */
public interface Data extends Serializable {

}

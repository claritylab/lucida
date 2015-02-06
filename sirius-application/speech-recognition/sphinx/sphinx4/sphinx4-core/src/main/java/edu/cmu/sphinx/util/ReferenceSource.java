/*
 * Copyright 2004 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *[B
 */
package edu.cmu.sphinx.util;

import java.util.List;

/** A source of reference texts. */
public interface ReferenceSource {

    /** Returns a list of reference text. */
    public List<String> getReferences();
}

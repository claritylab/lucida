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

package edu.cmu.sphinx.linguist;

import edu.cmu.sphinx.linguist.acoustic.HMMState;

/** Represents a single HMM state in a language search space */
public interface HMMSearchState extends SearchState {

    /**
     * Gets the hmm state
     *
     * @return the hmm state
     */
    HMMState getHMMState();

}

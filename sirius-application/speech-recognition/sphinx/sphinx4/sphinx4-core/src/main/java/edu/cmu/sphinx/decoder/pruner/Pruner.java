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

package edu.cmu.sphinx.decoder.pruner;

import edu.cmu.sphinx.decoder.search.ActiveList;
import edu.cmu.sphinx.util.props.Configurable;


/** Provides a mechanism for pruning a set of StateTokens */
public interface Pruner extends Configurable {

    /** Starts the pruner */
    public void startRecognition();


    /**
     * prunes the given set of states
     *
     * @param stateTokenList a list containing StateToken objects to be scored
     * @return the pruned list, (may be the sample list as stateTokenList)
     */
    public ActiveList prune(ActiveList stateTokenList);


    /** Performs post-recognition cleanup. */
    public void stopRecognition();


    /** Allocates resources necessary for this pruner */
    public void allocate();


    /** Deallocates resources necessary for this pruner */
    public void deallocate();


}



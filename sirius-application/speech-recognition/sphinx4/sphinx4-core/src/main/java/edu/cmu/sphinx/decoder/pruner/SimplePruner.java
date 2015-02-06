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
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;

/** Performs the default pruning behavior which is to invoke the purge on the active list */
public class SimplePruner implements Pruner {

    private String name;


    /* (non-Javadoc)
     * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
     */
    public void newProperties(PropertySheet ps) throws PropertyException {
    }


    public SimplePruner() {
    }

    /* (non-Javadoc)
     * @see edu.cmu.sphinx.util.props.Configurable#getName()
     */
    public String getName() {
        return name;
    }


    /** Starts the pruner */
    public void startRecognition() {
    }


    /**
     * prunes the given set of states
     *
     * @param activeList a activeList of tokens
     */
    public ActiveList prune(ActiveList activeList) {
        return activeList.purge();
    }


    /** Performs post-recognition cleanup. */
    public void stopRecognition() {
    }


    /* (non-Javadoc)
     * @see edu.cmu.sphinx.decoder.pruner.Pruner#allocate()
     */
    public void allocate() {
    }


    /* (non-Javadoc)
     * @see edu.cmu.sphinx.decoder.pruner.Pruner#deallocate()
     */
    public void deallocate() {

    }


}



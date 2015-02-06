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

package edu.cmu.sphinx.linguist.flat;


/** a visitor interface */
interface SentenceHMMStateVisitor {

    /**
     * Method called when a state is visited by the vistor
     *
     * @param state the state that is being visited
     * @return true if the visiting should be terminated
     */
    public boolean visit(SentenceHMMState state);
}


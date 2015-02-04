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

package edu.cmu.sphinx.trainer;

/** Provides mechanisms for handling tokens in the trainer. */
public class TrainerLink {

    private TrainerToken source;
    private TrainerToken destination;
    private Edge transition;


    public TrainerLink(Edge transition, TrainerToken source, TrainerToken destination) {
        this.source = source;
        this.transition = transition;
        this.destination = destination;
    }


    public TrainerToken getSource() {
        return source;
    }


    public TrainerToken getDestination() {
        return destination;
    }


    public Edge getTransition() {
        return transition;
    }
}

/**
 * 
 * Copyright 1999-2012 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.fst.operations;

import java.util.ArrayList;

import edu.cmu.sphinx.fst.Arc;
import edu.cmu.sphinx.fst.Fst;
import edu.cmu.sphinx.fst.State;
import edu.cmu.sphinx.fst.semiring.Semiring;

/**
 * Extend an Fst to a single final state and undo operations.
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class ExtendFinal {

    /**
     * Default Contructor
     */
    private ExtendFinal() {
    }

    /**
     * Extends an Fst to a single final state.
     * 
     * It adds a new final state with a 0.0 (Semiring's 1) final wight and
     * connects the current final states to it using epsilon transitions with
     * weight equal to the original final state's weight.
     * 
     * @param fst the Fst to extend
     */
    public static void apply(Fst fst) {
        Semiring semiring = fst.getSemiring();
        ArrayList<State> fStates = new ArrayList<State>();

        int numStates = fst.getNumStates();
        for (int i = 0; i < numStates; i++) {
            State s = fst.getState(i);
            if (s.getFinalWeight() != semiring.zero()) {
                fStates.add(s);
            }
        }

        // Add a new single final
        State newFinal = new State(semiring.one());
        fst.addState(newFinal);
        for (State s : fStates) {
            // add epsilon transition from the old final to the new one
            s.addArc(new Arc(0, 0, s.getFinalWeight(), newFinal));
            // set old state's weight to zero
            s.setFinalWeight(semiring.zero());
        }
    }

    /**
     * Undo of the extend operation
     */
    public static void undo(Fst fst) {
        State f = null;
        int numStates = fst.getNumStates();
        for (int i = 0; i < numStates; i++) {
            State s = fst.getState(i);
            if (s.getFinalWeight() != fst.getSemiring().zero()) {
                f = s;
                break;
            }
        }

        if (f == null) {
            System.err.println("Final state not found.");
            return;
        }
        for (int i = 0; i < numStates; i++) {
            State s = fst.getState(i);
            for (int j = 0; j < s.getNumArcs(); j++) {
                Arc a = s.getArc(j);
                if (a.getIlabel() == 0 && a.getOlabel() == 0
                        && a.getNextState().getId() == f.getId()) {
                    s.setFinalWeight(a.getWeight());
                }
            }
        }
        fst.deleteState(f);
    }

}

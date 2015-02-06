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

import edu.cmu.sphinx.fst.Arc;
import edu.cmu.sphinx.fst.Fst;
import edu.cmu.sphinx.fst.State;
import edu.cmu.sphinx.fst.semiring.Semiring;

/**
 * Reverse operation.
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class Reverse {
    /**
     * Default Constructor
     */
    private Reverse() {
    }

    /**
     * Reverses an fst
     * 
     * @param fst the fst to reverse
     * @return the reversed fst
     */
    public static Fst get(Fst fst) {
        if (fst.getSemiring() == null) {
            return null;
        }

        ExtendFinal.apply(fst);

        Semiring semiring = fst.getSemiring();

        Fst res = new Fst(fst.getNumStates());
        res.setSemiring(semiring);

        res.setIsyms(fst.getOsyms());
        res.setOsyms(fst.getIsyms());

        State[] stateMap = new State[fst.getNumStates()];
        int numStates = fst.getNumStates();
        for (int i=0; i<numStates; i++) {
            State is = fst.getState(i);
            State s = new State(semiring.zero());
            res.addState(s);
            stateMap[is.getId()] = s;
            if (is.getFinalWeight() != semiring.zero()) {
                res.setStart(s);
            }
        }

        stateMap[fst.getStart().getId()].setFinalWeight(semiring.one());

        for (int i=0; i<numStates; i++) {
            State olds = fst.getState(i);
            State news = stateMap[olds.getId()];
            int numArcs = olds.getNumArcs();
            for (int j = 0; j < numArcs; j++) {
                Arc olda = olds.getArc(j);
                State next = stateMap[olda.getNextState().getId()];
                Arc newa = new Arc(olda.getIlabel(), olda.getOlabel(),
                        semiring.reverse(olda.getWeight()), news);
                next.addArc(newa);
            }
        }

        ExtendFinal.undo(fst);
        return res;
    }
}

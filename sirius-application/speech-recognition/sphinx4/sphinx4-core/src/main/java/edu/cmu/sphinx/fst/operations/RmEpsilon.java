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

import java.util.HashMap;

import edu.cmu.sphinx.fst.Arc;
import edu.cmu.sphinx.fst.Fst;
import edu.cmu.sphinx.fst.State;
import edu.cmu.sphinx.fst.semiring.Semiring;

/**
 * Remove epsilon operation.
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class RmEpsilon {
    /**
     * Default Constructor
     */
    private RmEpsilon() {
    }

    /**
     * Put a new state in the epsilon closure
     */
    private static void put(State fromState, State toState, float weight,
            HashMap<State, Float>[] cl) {
        HashMap<State, Float> tmp = cl[fromState.getId()];
        if (tmp == null) {
            tmp = new HashMap<State, Float>();
            cl[fromState.getId()] = tmp;
        }
        tmp.put(toState, weight);
    }

    /**
     * Add a state in the epsilon closure
     */
    private static void add(State fromState, State toState, float weight,
            HashMap<State, Float>[] cl, Semiring semiring) {
        Float old = getPathWeight(fromState, toState, cl);
        if (old == null) {
            put(fromState, toState, weight, cl);
        } else {
            put(fromState, toState, semiring.plus(weight, old), cl);
        }

    }

    /**
     * Calculate the epsilon closure
     */
    private static void calcClosure(Fst fst, State state,
            HashMap<State, Float>[] cl, Semiring semiring) {
        State s = state;

        float pathWeight;
        int numArcs = s.getNumArcs();
        for (int j = 0; j < numArcs; j++) {
            Arc a = s.getArc(j);
            if ((a.getIlabel() == 0) && (a.getOlabel() == 0)) {
                if (cl[a.getNextState().getId()] == null) {
                    calcClosure(fst, a.getNextState(), cl, semiring);
                }
                if (cl[a.getNextState().getId()] != null) {
                    for (State pathFinalState : cl[a.getNextState().getId()]
                            .keySet()) {
                        pathWeight = semiring.times(
                                getPathWeight(a.getNextState(), pathFinalState,
                                        cl), a.getWeight());
                        add(state, pathFinalState, pathWeight, cl, semiring);
                    }
                }
                add(state, a.getNextState(), a.getWeight(), cl, semiring);
            }
        }
    }

    /**
     * Get an epsilon path's cost in epsilon closure
     */
    private static Float getPathWeight(State in, State out,
            HashMap<State, Float>[] cl) {
        if (cl[in.getId()] != null) {
            return cl[in.getId()].get(out);
        }

        return null;
    }

    /**
     * Removes epsilon transitions from an fst.
     * 
     * It return a new epsilon-free fst and does not modify the original fst
     * 
     * @param fst the fst to remove epsilon transitions from
     * @return the epsilon-free fst
     */
    public static Fst get(Fst fst) {
        if (fst == null) {
            return null;
        }

        if (fst.getSemiring() == null) {
            return null;
        }

        Semiring semiring = fst.getSemiring();

        Fst res = new Fst(semiring);

        @SuppressWarnings("unchecked")
        HashMap<State, Float>[] cl = new HashMap[fst.getNumStates()];
        State[] oldToNewStateMap = new State[fst.getNumStates()];
        State[] newToOldStateMap = new State[fst.getNumStates()];

        int numStates = fst.getNumStates();
        for (int i = 0; i < numStates; i++) {
            State s = fst.getState(i);
            // Add non-epsilon arcs
            State newState = new State(s.getFinalWeight());
            res.addState(newState);
            oldToNewStateMap[s.getId()] = newState;
            newToOldStateMap[newState.getId()] = s;
            if (newState.getId() == fst.getStart().getId()) {
                res.setStart(newState);
            }
        }

        for (int i = 0; i < numStates; i++) {
            State s = fst.getState(i);
            // Add non-epsilon arcs
            State newState = oldToNewStateMap[s.getId()];
            int numArcs = s.getNumArcs();
            for (int j = 0; j < numArcs; j++) {
                Arc a = s.getArc(j);
                if ((a.getIlabel() != 0) || (a.getOlabel() != 0)) {
                    newState.addArc(new Arc(a.getIlabel(), a.getOlabel(), a
                            .getWeight(), oldToNewStateMap[a.getNextState()
                            .getId()]));
                }
            }

            // Compute e-Closure
            if (cl[s.getId()] == null) {
                calcClosure(fst, s, cl, semiring);
            }
        }

        // augment fst with arcs generated from epsilon moves.
        numStates = res.getNumStates();
        for (int i = 0; i < numStates; i++) {
            State s = res.getState(i);
            State oldState = newToOldStateMap[s.getId()];
            if (cl[oldState.getId()] != null) {
                for (State pathFinalState : cl[oldState.getId()].keySet()) {
                    State s1 = pathFinalState;
                    if (s1.getFinalWeight() != semiring.zero()) {
                        s.setFinalWeight(semiring.plus(s.getFinalWeight(),
                                semiring.times(getPathWeight(oldState, s1, cl),
                                        s1.getFinalWeight())));
                    }
                    int numArcs = s1.getNumArcs();
                    for (int j = 0; j < numArcs; j++) {
                        Arc a = s1.getArc(j);
                        if ((a.getIlabel() != 0) || (a.getOlabel() != 0)) {
                            Arc newArc = new Arc(a.getIlabel(), a.getOlabel(),
                                    semiring.times(a.getWeight(),
                                            getPathWeight(oldState, s1, cl)),
                                    oldToNewStateMap[a.getNextState().getId()]);
                            s.addArc(newArc);
                        }
                    }
                }
            }
        }

        res.setIsyms(fst.getIsyms());
        res.setOsyms(fst.getOsyms());

        Connect.apply(res);

        return res;
    }
}

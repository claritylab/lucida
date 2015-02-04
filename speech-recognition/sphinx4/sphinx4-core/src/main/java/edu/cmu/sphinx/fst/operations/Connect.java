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
import java.util.HashSet;

import edu.cmu.sphinx.fst.Arc;
import edu.cmu.sphinx.fst.Fst;
import edu.cmu.sphinx.fst.State;
import edu.cmu.sphinx.fst.semiring.Semiring;

/**
 * Connect operation.
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class Connect {
    /**
     * Calculates the coaccessible states of an fst
     */
    private static void calcCoAccessible(Fst fst, State state,
            ArrayList<ArrayList<State>> paths, HashSet<State> coaccessible) {
        // hold the coaccessible added in this loop
        ArrayList<State> newCoAccessibles = new ArrayList<State>();
        for (ArrayList<State> path : paths) {
            int index = path.lastIndexOf(state);
            if (index != -1) {
                if (state.getFinalWeight() != fst.getSemiring().zero()
                        || coaccessible.contains(state)) {
                    for (int j = index; j > -1; j--) {
                        if (!coaccessible.contains(path.get(j))) {
                            newCoAccessibles.add(path.get(j));
                            coaccessible.add(path.get(j));
                        }
                    }
                }
            }
        }

        // run again for the new coaccessibles
        for (State s : newCoAccessibles) {
            calcCoAccessible(fst, s, paths, coaccessible);
        }
    }

    /**
     * Copies a path
     */
    private static void duplicatePath(int lastPathIndex, State fromState,
            State toState, ArrayList<ArrayList<State>> paths) {
        ArrayList<State> lastPath = paths.get(lastPathIndex);
        // copy the last path to a new one, from start to current state
        int fromIndex = lastPath.indexOf(fromState);
        int toIndex = lastPath.indexOf(toState);
        if (toIndex == -1) {
            toIndex = lastPath.size() - 1;
        }
        ArrayList<State> newPath = new ArrayList<State>(lastPath.subList(
                fromIndex, toIndex));
        paths.add(newPath);
    }

    /**
     * The depth first search recursion
     */
    private static State depthFirstSearchNext(Fst fst, State start,
            ArrayList<ArrayList<State>> paths, ArrayList<Arc>[] exploredArcs,
            HashSet<State> accessible) {
        int lastPathIndex = paths.size() - 1;

        ArrayList<Arc> currentExploredArcs = exploredArcs[start.getId()];
        paths.get(lastPathIndex).add(start);
        if (start.getNumArcs() != 0) {
            int arcCount = 0;
            int numArcs = start.getNumArcs();
            for (int j = 0; j < numArcs; j++) {
                Arc arc = start.getArc(j);
                if ((currentExploredArcs == null)
                        || !currentExploredArcs.contains(arc)) {
                    lastPathIndex = paths.size() - 1;
                    if (arcCount++ > 0) {
                        duplicatePath(lastPathIndex, fst.getStart(), start,
                                paths);
                        lastPathIndex = paths.size() - 1;
                        paths.get(lastPathIndex).add(start);
                    }
                    State next = arc.getNextState();
                    addExploredArc(start.getId(), arc, exploredArcs);
                    // detect self loops
                    if (next.getId() != start.getId()) {
                        depthFirstSearchNext(fst, next, paths, exploredArcs, accessible);
                    }
                }
            }
        }
        lastPathIndex = paths.size() - 1;
        accessible.add(start);

        return start;
    }

    /**
     * Adds an arc top the explored arcs list
     */
    private static void addExploredArc(int stateId, Arc arc,
            ArrayList<Arc>[] exploredArcs) {
        if (exploredArcs[stateId] == null) {
            exploredArcs[stateId] = new ArrayList<Arc>();
        }
        exploredArcs[stateId].add(arc);

    }

    /**
     * Initialization of a depth first search recursion
     */
    private static void depthFirstSearch(Fst fst, HashSet<State> accessible,
            ArrayList<ArrayList<State>> paths, ArrayList<Arc>[] exploredArcs,
            HashSet<State> coaccessible) {
        State currentState = fst.getStart();
        State nextState = currentState;
        do {
            if (!accessible.contains(currentState)) {
                nextState = depthFirstSearchNext(fst, currentState, paths, exploredArcs,
                        accessible);
            }
        } while (currentState.getId() != nextState.getId());
        int numStates = fst.getNumStates();
        for (int i = 0; i < numStates; i++) {
            State s = fst.getState(i);
            if (s.getFinalWeight() != fst.getSemiring().zero()) {
                calcCoAccessible(fst, s, paths, coaccessible);
            }
        }
    }

    /**
     * Trims an Fst, removing states and arcs that are not on successful paths.
     * 
     * @param fst the fst to trim
     */
    public static void apply(Fst fst) {
        Semiring semiring = fst.getSemiring();
        if (semiring == null) {
            System.out.println("Fst has no semiring.");
            return;
        }

        HashSet<State> accessible = new HashSet<State>();
        HashSet<State> coaccessible = new HashSet<State>();
        @SuppressWarnings("unchecked")
        ArrayList<Arc>[] exploredArcs = new ArrayList[fst.getNumStates()];

        ArrayList<ArrayList<State>> paths = new ArrayList<ArrayList<State>>();
        paths.add(new ArrayList<State>());

        depthFirstSearch(fst, accessible, paths, exploredArcs, coaccessible);

        HashSet<State> toDelete = new HashSet<State>();

        for (int i = 0; i < fst.getNumStates(); i++) {
            State s = fst.getState(i);
            if (!(accessible.contains(s) || coaccessible.contains(s))) {
                toDelete.add(s);
            }
        }

        fst.deleteStates(toDelete);
    }
}

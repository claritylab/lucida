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

import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.PriorityQueue;

import edu.cmu.sphinx.fst.Arc;
import edu.cmu.sphinx.fst.Fst;
import edu.cmu.sphinx.fst.State;
import edu.cmu.sphinx.fst.utils.Pair;
import edu.cmu.sphinx.fst.semiring.Semiring;

/**
 * N-shortest paths operation.
 * 
 * See: M. Mohri, M. Riley,
 * "An Efficient Algorithm for the n-best-strings problem", Proceedings of the
 * International Conference on Spoken Language Processing 2002 (ICSLP '02).
 * 
 * See: M. Mohri,
 * "Semiring Framework and Algorithms for Shortest-Distance Problems", Journal
 * of Automata, Languages and Combinatorics, 7(3), pp. 321-350, 2002.
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class NShortestPaths {
    private NShortestPaths() {
    }

    /**
     * Calculates the shortest distances from each state to the final
     * 
     * @param fst
     *            the fst to calculate the shortest distances
     * @return the array containing the shortest distances
     */
    public static float[] shortestDistance(Fst fst) {

        Fst reversed = Reverse.get(fst);

        float[] d = new float[reversed.getNumStates()];
        float[] r = new float[reversed.getNumStates()];

        Semiring semiring = reversed.getSemiring();

        Arrays.fill(d, semiring.zero());
        Arrays.fill(r, semiring.zero());

        LinkedHashSet<State> queue = new LinkedHashSet<State>();

        queue.add(reversed.getStart());

        d[reversed.getStart().getId()] = semiring.one();
        r[reversed.getStart().getId()] = semiring.one();

        while (!queue.isEmpty()) {
            State q = queue.iterator().next();
            queue.remove(q);

            float rnew = r[q.getId()];
            r[q.getId()] = semiring.zero();

            for (int i = 0; i < q.getNumArcs(); i++) {
                Arc a = q.getArc(i);
                State nextState = a.getNextState();
                float dnext = d[a.getNextState().getId()];
                float dnextnew = semiring.plus(dnext,
                        semiring.times(rnew, a.getWeight()));
                if (dnext != dnextnew) {
                    d[a.getNextState().getId()] = dnextnew;
                    r[a.getNextState().getId()] = semiring.plus(r[a
                            .getNextState().getId()], semiring.times(rnew,
                            a.getWeight()));
                    if (!queue.contains(nextState.getId())) {
                        queue.add(nextState);
                    }
                }
            }
        }
        return d;
    }

    /**
     * Calculates the n-best shortest path from the initial to the final state.
     * 
     * @param fst
     *            the fst to calculate the nbest shortest paths
     * @param n
     *            number of best paths to return
     * @param determinize
     *            if true the input fst will bwe determinized prior the
     *            operation
     * @return an fst containing the n-best shortest paths
     */
    public static Fst get(Fst fst, int n, boolean determinize) {
        if (fst == null) {
            return null;
        }

        if (fst.getSemiring() == null) {
            return null;
        }
        Fst fstdet = fst;
        if (determinize) {
            fstdet = Determinize.get(fst);
        }
        final Semiring semiring = fstdet.getSemiring();
        Fst res = new Fst(semiring);
        res.setIsyms(fstdet.getIsyms());
        res.setOsyms(fstdet.getOsyms());

        final float[] d = shortestDistance(fstdet);

        ExtendFinal.apply(fstdet);

        int[] r = new int[fstdet.getNumStates()];

        PriorityQueue<Pair<State, Float>> queue = new PriorityQueue<Pair<State, Float>>(
                10, new Comparator<Pair<State, Float>>() {

                    public int compare(Pair<State, Float> o1,
                            Pair<State, Float> o2) {
                        float previous = o1.getRight();
                        float d1 = d[o1.getLeft().getId()];

                        float next = o2.getRight();
                        float d2 = d[o2.getLeft().getId()];

                        float a1 = semiring.times(next, d2);
                        float a2 = semiring.times(previous, d1);

                        if (semiring.naturalLess(a1, a2))
                            return 1;

                        if (a1 == a2)
                            return 0;

                        return -1;
                    }
                });

        HashMap<Pair<State, Float>, Pair<State, Float>> previous = new HashMap<Pair<State, Float>, Pair<State, Float>>(
                fst.getNumStates());
        HashMap<Pair<State, Float>, State> stateMap = new HashMap<Pair<State, Float>, State>(
                fst.getNumStates());

        State start = fstdet.getStart();
        Pair<State, Float> item = new Pair<State, Float>(start, semiring.one());
        queue.add(item);
        previous.put(item, null);

        while (!queue.isEmpty()) {
            Pair<State, Float> pair = queue.remove();
            State p = pair.getLeft();
            Float c = pair.getRight();

            State s = new State(p.getFinalWeight());
            res.addState(s);
            stateMap.put(pair, s);
            if (previous.get(pair) == null) {
                // this is the start state
                res.setStart(s);
            } else {
                // add the incoming arc from previous to current
                State previouState = stateMap.get(previous.get(pair));
                State previousOldState = previous.get(pair).getLeft();
                for (int j = 0; j < previousOldState.getNumArcs(); j++) {
                    Arc a = previousOldState.getArc(j);
                    if (a.getNextState().equals(p)) {
                        previouState.addArc(new Arc(a.getIlabel(), a
                                .getOlabel(), a.getWeight(), s));
                    }
                }
            }

            Integer stateIndex = p.getId();
            r[stateIndex]++;

            if ((r[stateIndex] == n) && (p.getFinalWeight() != semiring.zero())) {
                break;
            }

            if (r[stateIndex] <= n) {
                for (int j = 0; j < p.getNumArcs(); j++) {
                    Arc a = p.getArc(j);
                    float cnew = semiring.times(c, a.getWeight());
                    Pair<State, Float> next = new Pair<State, Float>(
                            a.getNextState(), cnew);
                    previous.put(next, pair);
                    queue.add(next);
                }
            }
        }

        return res;
    }
}

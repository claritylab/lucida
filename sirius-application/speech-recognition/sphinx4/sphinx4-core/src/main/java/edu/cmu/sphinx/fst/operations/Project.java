/**
 * 
 */
package edu.cmu.sphinx.fst.operations;

import edu.cmu.sphinx.fst.Arc;
import edu.cmu.sphinx.fst.Fst;
import edu.cmu.sphinx.fst.ImmutableFst;
import edu.cmu.sphinx.fst.State;

/**
 * Project operation. 
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class Project {
    /**
     * Default Constructor
     */
    private Project() {
    }

    /**
     * Projects an fst onto its domain or range by either copying each arc's
     * input label to its output label or vice versa.
     * 
     * 
     * @param fst
     * @param pType
     */
    public static void apply(Fst fst, ProjectType pType) {
        if (pType == ProjectType.INPUT) {
            fst.setOsyms(fst.getIsyms());
        } else if (pType == ProjectType.OUTPUT) {
            fst.setIsyms(fst.getOsyms());
        }

        int numStates = fst.getNumStates();
        for (int i = 0; i < numStates; i++) {
            State s = fst.getState(i);
            // Immutable fsts hold an additional (null) arc
            int numArcs = (fst instanceof ImmutableFst) ? s.getNumArcs() - 1: s
                    .getNumArcs();
                for (int j = 0; j < numArcs; j++) {
                Arc a = s.getArc(j);
                if (pType == ProjectType.INPUT) {
                    a.setOlabel(a.getIlabel());
                } else if (pType == ProjectType.OUTPUT) {
                    a.setIlabel(a.getOlabel());
                }
            }
        }
    }
}

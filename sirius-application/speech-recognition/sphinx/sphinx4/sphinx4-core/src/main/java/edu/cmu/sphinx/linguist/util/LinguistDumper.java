/*
 * Copyright 1999-2003 Carnegie Mellon University.  
 * Portions Copyright 2003 Sun Microsystems, Inc.  
 * Portions Copyright 2003 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.linguist.util;

import edu.cmu.sphinx.linguist.*;
import edu.cmu.sphinx.util.Cache;
import edu.cmu.sphinx.util.props.*;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.util.*;

/**
 * A linguist processor that dumps out the search space in a simple format. This processor is designed so that it can be
 * easily extended by replacing the dumpNode and the dumpEdge methods.
 */
public class LinguistDumper extends LinguistProcessor {

    /** The property for the destination of the LinguistDumper */
    @S4String(defaultValue = "linguistDump.txt")
    public final static String PROP_FILENAME = "filename";

    // ------------------------------
    // Configuration data
    // -------------------------------
    private boolean depthFirst = true;
    private String filename;

    public LinguistDumper( String filename, Linguist linguist ) {
        super( linguist );
        this.filename = filename;
    }

    public LinguistDumper() {

    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        filename = ps.getString(PROP_FILENAME);
    }


    /** Dumps the search space hmm in GDL format */
    @Override
    public void run() {
        try {
            FileOutputStream fos = new FileOutputStream(filename);
            PrintStream out = new PrintStream(fos);
            SearchState firstState =
                    getLinguist().getSearchGraph().getInitialState();
            dumpSearchGraph(out, firstState);
            out.close();
        } catch (FileNotFoundException fnfe) {
            System.out.println("Can't dump to file " + filename + ' ' + fnfe);
        }
    }


    /**
     * Sets whether the traversal is depth first or breadth first
     *
     * @param depthFirst if true traversal is depth first, otherwise the traversal is breadth first
     */
    protected void setDepthFirst(boolean depthFirst) {
        this.depthFirst = depthFirst;
    }

    /**
     * Called at the start of the dump
     *
     * @param out the output stream.
     */
    protected void startDump(PrintStream out) {
    }


    /**
     * Called at the end of the dump
     *
     * @param out the output stream.
     */
    protected void endDump(PrintStream out) {
    }


    /**
     * Called to dump out a node in the search space
     *
     * @param out   the output stream.
     * @param state the state to dump
     * @param level the level of the state
     */
    protected void startDumpNode(PrintStream out, SearchState state, int level) {
    }


    /**
     * Called to dump out a node in the search space
     *
     * @param out   the output stream.
     * @param state the state to dump
     * @param level the level of the state
     */
    protected void endDumpNode(PrintStream out, SearchState state, int level) {
    }


    /**
     * Dumps an arc
     *
     * @param out   the output stream.
     * @param from  arc leaves this state
     * @param arc   the arc to dump
     * @param level the level of the state
     */
    protected void dumpArc(PrintStream out, SearchState from,
                           SearchStateArc arc, int level) {
    }


    /**
     * Dumps the search graph
     *
     * @param out           place to dump the output
     * @param startingState the initial state of the search space
     */
    private void dumpSearchGraph(PrintStream out, SearchState startingState) {
        List<StateLevel> queue = new LinkedList<StateLevel>();
        Set<String> visitedStates = new HashSet<String>();
        startDump(out);
        queue.add(new StateLevel(startingState, 0));
        while (!queue.isEmpty()) {
            StateLevel stateLevel = queue.remove(0);
            int level = stateLevel.getLevel();
            SearchState state = stateLevel.getState();
            // equalCheck(state);
            if (!visitedStates.contains(state.getSignature())) {
                visitedStates.add(state.getSignature());
                startDumpNode(out, state, level);
                SearchStateArc[] arcs = state.getSuccessors();
                for (int i = arcs.length - 1; i >= 0; i--) {
                    SearchState nextState = arcs[i].getState();
                    dumpArc(out, state, arcs[i], level);
                    if (depthFirst) {
                        // if depth first, its a stack
                        queue.add(0, new StateLevel(nextState, level + 1));
                    } else {
                        queue.add(new StateLevel(nextState, level + 1));
                    }
                }
                endDumpNode(out, state, level);
            }
        }
        endDump(out);
    }


    final Cache<SearchState> eqStates = new Cache<SearchState>();
    final Map<String, SearchState> eqSigs = new HashMap<String, SearchState>();


    /**
     * This is a bit of test/debugging code that ensures that the states that have equal signatures are also considered
     * to be object.equals and vice versa.. This method will dump out any states where this contract is not true
     *
     * @param state the state to check
     */
    @SuppressWarnings("unused")
    private void equalCheck(SearchState state) {
        SearchState eqState = eqStates.cache(state);
        SearchState eqSig = eqSigs.get(state.getSignature());
        if (eqState == null ^ eqSig == null) {
            System.out.println("Missing one: ");
            System.out.println("  state val: " + state);
            System.out.println("  state sig: " + state.getSignature());
            System.out.println("  eqState val: " + eqState);
            System.out.println("  eqSig val: " + eqSig);
            if (eqState != null) {
                System.out.println("   eqState sig: " + eqState.getSignature());
            }
            if (eqSig != null) {
                System.out.println("   eqSig sig: " + eqSig.getSignature());
            }
        }
        if (eqState == null) {
            eqState = state;
        }
        if (eqSig == null) {
            eqSigs.put(state.getSignature(), state);
            eqSig = state;
        }
        if (!eqState.getSignature().equals(state.getSignature())) {
            System.out.println("Sigs mismatch for: ");
            System.out.println("  state sig: " + state.getSignature());
            System.out.println("  eqSig sig: " + eqSig.getSignature());
            System.out.println("  state val: " + state);
            System.out.println("  eqSig val: " + eqSig);
        }
        if (!eqState.equals(state)) {
            System.out.println("obj mismatch for: ");
            System.out.println("  state sig: " + state.getSignature());
            System.out.println("  eqSig sig: " + eqSig.getSignature());
            System.out.println("  state val: " + state);
            System.out.println("  eqSig val: " + eqSig);
        }
    }
}

/** A class for bundling together a SearchState and its level. */

class StateLevel {

    private final int level;
    private final SearchState state;


    /**
     * Constructs a StateLevel from its primitive components.
     *
     * @param state the state to be bundled in the StateLevel
     * @param level the level of the state
     */
    StateLevel(SearchState state, int level) {
        this.state = state;
        this.level = level;
    }


    /**
     * Returns the state
     *
     * @return the state
     */
    SearchState getState() {
        return state;
    }


    /**
     * Returns the level
     *
     * @return the level.
     */
    int getLevel() {
        return level;
    }


    /**
     * Returns a string representation of the object
     *
     * @return a string representation
     */
    @Override
    public String toString() {
        return String.valueOf(level) + ' ' + state.getSignature() + " 1 "
                + getTypeLabel(state);
    }


    /**
     * Retrieves a type label for a state
     *
     * @param state the state of interest
     * @return a label for the type of state (one of Unit, Word, HMM or other
     */
    public String getTypeLabel(SearchState state) {
        if (state instanceof UnitSearchState) {
            return "Unit";
        }
        if (state instanceof WordSearchState) {
            return "Word";
        }
        if (state instanceof HMMSearchState) {
            return "HMM";
        }
        return "other";
    }
}

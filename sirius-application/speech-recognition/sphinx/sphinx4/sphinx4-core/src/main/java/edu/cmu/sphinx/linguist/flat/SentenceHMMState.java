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


import edu.cmu.sphinx.linguist.SearchState;
import edu.cmu.sphinx.linguist.SearchStateArc;
import edu.cmu.sphinx.linguist.WordSequence;
import edu.cmu.sphinx.linguist.dictionary.Word;

import java.io.Serializable;
import java.util.*;


/** Represents a single state in an SentenceHMM */
@SuppressWarnings("serial")
public abstract class SentenceHMMState implements Serializable, SearchState {

    private final static int MASK_IS_FINAL = 0x1;
    private final static int MASK_COLOR_RED = 0x2;
    private final static int MASK_PROCESSED = 0x4;
    private final static int MASK_FAN_IN = 0x8;
    private final static int MASK_IS_WORD_START = 0x10;
    private final static int MASK_IS_SHARED_STATE = 0x20;
    private final static int MASK_WHICH = 0xffff;
    private final static int SHIFT_WHICH = 0x8;

    /** A Color is used to tag SentenceHMM nodes */
    public static enum Color { RED, GREEN }

    private static int globalStateNumber = -1000;

    private int stateNumber;

    // a number of separate variables are maintained in 'fields'
    // inorder to reduce the size of the SentenceHMM

    private int fields;
    private String name;

    private final Map<String, SentenceHMMStateArc> arcs;
    private SentenceHMMState parent;
    private String cachedName;
    private String fullName;
    private SentenceHMMStateArc[] successorArray;


    /**
     * Creates a SentenceHMMState
     *
     * @param name   the name of the current SentenceHMMState
     * @param parent the parent of the current instance
     * @param which  the index of the current instance
     */
    protected SentenceHMMState(String name, SentenceHMMState parent, int which) {
        this();
        this.name = name + which;
        this.parent = parent;
        setWhich(which);
        setProcessed(false);
        setColor(Color.RED);
    }


    /** Empty contructor */
    protected SentenceHMMState() {
        stateNumber = globalStateNumber--;
        this.arcs = new LinkedHashMap<String, SentenceHMMStateArc>();
    }


    /**
     * Determines if this state marks the beginning of a word
     *
     * @return true if the state marks the beginning of a word
     */
    public boolean isWordStart() {
        return (fields & MASK_IS_WORD_START) == MASK_IS_WORD_START;
    }


    /**
     * Sets the 'wordStart' flag
     *
     * @param wordStart <code>true</code> if this state marks the beginning of a word.
     */
    public void setWordStart(boolean wordStart) {
        if (wordStart) {
            this.fields |= MASK_IS_WORD_START;
        } else {
            this.fields &= ~MASK_IS_WORD_START;
        }
    }


    /**
     * Determines if this state is a shard state
     *
     * @return true if the state marks the beginning of a word
     */
    public boolean isSharedState() {
        return (fields & MASK_IS_SHARED_STATE) == MASK_IS_SHARED_STATE;
    }


    /**
     * Sets the shared state flag
     *
     * @param shared <code>true</code> if this state is shared
     */
    public void setSharedState(boolean shared) {
        if (shared) {
            this.fields |= MASK_IS_SHARED_STATE;
        } else {
            this.fields &= ~MASK_IS_SHARED_STATE;
        }
    }


    /**
     * Returns the word associated with the particular unit
     *
     * @return the word associated with this state, or null if there is no word associated with this state.
     */
    public Word getAssociatedWord() {
        Word word = null;
        SentenceHMMState state = this;

        while (state != null && !(state instanceof WordState)) {
            state = state.getParent();
        }

        if (state != null) {
            WordState wordState = (WordState) state;
            word = wordState.getWord();
        }
        return word;
    }


    /**
     * Retrieves a short label describing the type of this state. Typically, subclasses of SentenceHMMState will
     * implement this method and return a short (5 chars or less) label
     *
     * @return the short label.
     */
    public String getTypeLabel() {
        return "state";
    }


    /**
     * Determines if this state is a fan-in state. The search may need to adjust the pruning for states that fan in
     * since they are shared by multiple paths
     *
     * @return <code>true</code> if the state is a fan in state
     */
    public boolean isFanIn() {
        return (fields & MASK_FAN_IN) == MASK_FAN_IN;
    }


    /**
     * Sets the fan in state
     *
     * @param fanIn if true its a fan in state
     */
    public void setFanIn(boolean fanIn) {
        if (fanIn) {
            this.fields |= MASK_FAN_IN;
        } else {
            this.fields &= ~MASK_FAN_IN;
        }
    }


    /**
     * Sets the processed flag for this state
     *
     * @param processed the new setting for the processed flag
     */
    public void setProcessed(boolean processed) {
        if (processed) {
            this.fields |= MASK_PROCESSED;
        } else {
            this.fields &= ~MASK_PROCESSED;
        }
    }


    /**
     * Determines if this state has been 'processed'. The meaning of 'processed' is not defined here, but is up to the
     * higher levels
     *
     * @return true if the state has been processed.
     */
    public boolean isProcessed() {
        return (fields & MASK_PROCESSED) == MASK_PROCESSED;
    }


    /** Reset process flags for this state and all successor states */
    public void resetAllProcessed() {
        visitStates(new SentenceHMMStateVisitor() {
            public boolean visit(SentenceHMMState state) {
                state.setProcessed(false);
                return false;
            }
        }, this, false);
    }


    /**
     * Gets the word history for this state.
     *
     * @return the word history.
     */
    //TODO Not implemented
    public WordSequence getWordHistory() {
        return WordSequence.EMPTY;
    }


    /**
     * Gets the number of successors
     *
     * @return the number of successors
     */
    public int getNumSuccessors() {
        return arcs.size();
    }


    /**
     * Gets a successor to this search state
     *
     * @return the set of successors
     */
    public SearchStateArc[] getSuccessors() {
        if (successorArray == null) {
            successorArray = arcs.values().toArray(new SentenceHMMStateArc[arcs.size()]);
        }
        return successorArray;
    }


    /**
     * Returns the lextree state
     *
     * @return the lex tree state
     */
    public SentenceHMMState getLexState() {
        return this;
    }


    /**
     * remove the given arc from the set of succors
     *
     * @param arc the arc to remove
     */
    void deleteSuccessor(SentenceHMMStateArc arc) {
        arcs.remove(arc);
    }


    /**
     * Connects the arc to this sentence hmm.  If the node at the end of the arc is already pointing to some other node
     * as its predecessor, don't change that relationship, since its probably a result of the nodes being reused'
     *
     * @param arc the path to the next state
     */
    public void connect(SentenceHMMStateArc arc) {
        if (successorArray != null) {
            successorArray = null;
        }
        rawConnect(arc);
    }


    /**
     * Connects the arc to this sentence hmm, but don't affect the predecessor relation ship
     *
     * @param arc the arc to the next state
     */
    private void rawConnect(SentenceHMMStateArc arc) {
        SentenceHMMState state = (SentenceHMMState) arc.getState();

        // attach the state-number because the state-signature is not necessarily unique
        arcs.put(state.getValueSignature() + state.getStateNumber(), arc);
    }


    /**
     * Determines if this state is an emitting state
     *
     * @return true if the state is an emitting state
     */
    public boolean isEmitting() {
        return false;
    }


    /**
     * Determines if this is a final state
     *
     * @return true if this is a final state
     */
    public boolean isFinal() {
        return (fields & MASK_IS_FINAL) == MASK_IS_FINAL;
    }


    /**
     * Sets this is to be final state
     *
     * @param state true if this is a final state
     */
    public void setFinalState(boolean state) {
        if (state) {
            this.fields |= MASK_IS_FINAL;
        } else {
            this.fields &= ~MASK_IS_FINAL;
        }
    }


    /**
     * Determines if this state is a unit state
     *
     * @return <code>true</code> if the state is a unit state.
     */
    public boolean isUnit() {
        return false;
    }


    /** Dumps this SentenceHMMState and all its successors. Just for debugging. */
    public void dumpAll() {
        visitStates(new SentenceHMMStateVisitor() {
            public boolean visit(SentenceHMMState state) {
                state.dump();
                return false;
            }
        }, this, true);
    }


    /**
     * Returns any annotation for this state
     *
     * @return the annotation
     */
    protected String getAnnotation() {
        return "";
    }


    /** Dumps this state */
    private void dump() {
        System.out.println(" ----- " + getTitle() + " ---- ");
        for (int i = 0; i < getSuccessors().length; i++) {
            SentenceHMMStateArc arc = (SentenceHMMStateArc) getSuccessors()[i];
            System.out.println("   -> " +
                    arc.getState().toPrettyString());
        }
    }


    /** Validates this SentenceHMMState and all successors */
    public void validateAll() {
        // TODO fix me
    }


    /**
     * Gets the name for this state
     *
     * @return the name
     */
    public String getName() {
        return name;
    }


    /**
     * Returns a pretty name for this HMM
     *
     * @return a pretty name
     */
    public String getPrettyName() {
        return getName();
    }


    /** Returns the string representation of this object */
    @Override
    public String toString() {
        if (cachedName == null) {
            StringBuilder sb = new StringBuilder();
            if (isEmitting()) {
                sb.append('*');
            }
            sb.append(getName());

            // String base = (isEmitting() ? "*" : "") + getName()
            //       + getWhich() + (isFinal() ? "!" : "");

            if (parent != null) {
                sb.append('_');
                sb.append(parent);
            }

            if (isFinal()) {
                sb.append('!');
            }
            cachedName = sb.toString();
        }
        return cachedName;
    }


    /*
    * Returns a pretty version of the string representation
    * for this object
    *
    * @return a pretty string
    */
    public String toPrettyString() {
        return toString();
    }


    /**
     * Gets the fullName for this state
     *
     * @return the full name for this state
     */
    public String getFullName() {
        if (fullName == null) {
            if (parent == null) {
                fullName = getName();
            } else {
                fullName = getName() + '.' + parent.getFullName();
            }
        }

        return fullName;
    }


    /**
     * Gets the signature for this state
     *
     * @return the signature
     */
    public String getSignature() {
        return getFullName();
    }


    /**
     * gets the title (fullname + stateNumber) for this state
     *
     * @return the title
     */
    public String getTitle() {
        return getFullName() + ':' + stateNumber;
        // return getSignature() + ":" + stateNumber;
    }


    /**
     * Retrieves the index for this state words
     *
     * @return the index
     */
    public int getWhich() {
        return (fields >> SHIFT_WHICH) & MASK_WHICH;
    }


    /**
     * Sets the index for this state
     *
     * @param which the index for this state
     */
    public void setWhich(int which) {
        assert which >= 0 && which <= MASK_WHICH;
        fields |= (which & MASK_WHICH) << SHIFT_WHICH;
    }


    /**
     * Retrieves the parent sate
     *
     * @return the parent state (or null if this state does not have a parent state).
     */
    public SentenceHMMState getParent() {
        return parent;
    }
    

    /**
     * Searches the set of arcs for an arc that points to a state with an identical value
     *
     * @param state the state to search for
     * @return the arc or null if none could be found.
     */
    public SentenceHMMStateArc findArc(SentenceHMMState state) {
        return arcs.get(state.getValueSignature());
    }


    /**
     * Returns the value signature of this unit
     *
     * @return the value signature
     */
    public String getValueSignature() {
        return getFullName();
    }


    /**
     * Visit all of the states starting at start with the given vistor
     *
     * @param visitor the state visitor
     * @param start   the place to start the search
     * @param sorted  if true, states are sorted before visited
     * @return true if the visiting was terminated before all nodes were visited
     */
    public static boolean visitStates(SentenceHMMStateVisitor visitor,
                                      SentenceHMMState start, boolean sorted) {
        Set<SentenceHMMState> states = collectStates(start);

        if (sorted) {
            // sort the states by stateNumber

            TreeSet<SentenceHMMState> sortedStates = new TreeSet<SentenceHMMState>(new Comparator<SentenceHMMState>() {

                public int compare(SentenceHMMState o1, SentenceHMMState o2) {
                    SentenceHMMState so1 = o1;
                    SentenceHMMState so2 = o2;
                    return so1.stateNumber - so2.stateNumber;
                }
            });

            sortedStates.addAll(states);
            states = sortedStates;
        }

        for (SentenceHMMState state : states) {
            if (visitor.visit(state)) {
                return true;
            }
        }
        return false;
    }


    /**
     * Sets the color for this node
     *
     * @param color the color of this node
     */
    public void setColor(Color color) {
        if (color == Color.RED) {
            this.fields |= MASK_COLOR_RED;
        } else {
            this.fields &= ~MASK_COLOR_RED;
        }
    }


    /**
     * Gets the color for this node
     *
     * @return the color of this node
     */
    public Color getColor() {
        if ((fields & MASK_COLOR_RED) == MASK_COLOR_RED) {
            return Color.RED;
        } else {
            return Color.GREEN;
        }
    }


    /**
     * Gets the state number for this state
     *
     * @return the state number
     */
    private int getStateNumber() {
        return stateNumber;
    }


    /**
     * Collect all states starting from the given start state
     *
     * @param start the state to start the search from
     * @return the set of collected state
     */
    public static Set<SentenceHMMState> collectStates(SentenceHMMState start) {
        Set<SentenceHMMState> visitedStates = new HashSet<SentenceHMMState>();
        List<SentenceHMMState> queue = new LinkedList<SentenceHMMState>();

        queue.add(start);

        while (!queue.isEmpty()) {
            SentenceHMMState state = queue.remove(0);

            visitedStates.add(state);
            SearchStateArc[] successors = state.getSuccessors();
            for (SearchStateArc arc : successors) {
                SentenceHMMState nextState = (SentenceHMMState)arc.getState();
                if (!visitedStates.contains(nextState) && !queue.contains(nextState)) {
                    queue.add(nextState);
                }
            }
        }
        return visitedStates;
    }


    /**
     * Returns the order of this particular state
     *
     * @return the state order for this state
     */
    abstract public int getOrder();
}


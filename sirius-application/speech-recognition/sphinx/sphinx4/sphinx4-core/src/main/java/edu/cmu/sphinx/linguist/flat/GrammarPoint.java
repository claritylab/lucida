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

import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.linguist.dictionary.Pronunciation;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.linguist.language.grammar.GrammarArc;
import edu.cmu.sphinx.linguist.language.grammar.GrammarNode;

import java.util.ArrayList;
import java.util.List;

/**
 * Manages a particular point in a grammar. The GrammarPoint is used to manage the look-ahead for generating
 * right-contexts. Since we haven't built the HMM tree yet, looking ahead can be difficult. The GrammarPoint class
 * points to a particular unit within a pronunciation/word/grammar.  From a particular grammar point, it is possible to
 * get the set of next grammar points.
 */

public class GrammarPoint {

    private GrammarNode node;        // the grammar node
    private int alternativeIndex;    // which alternative in the grammar
    private int wordIndex;        // which word in the alternative
    private int pronunciationIndex;    // which pronunciation in the word
    private int unitIndex;        // which unit in the pronunciation
    private static boolean bounded;


    /**
     * Creates a grammar point that points to the given unit of the given pronunciation state.
     *
     * @param state the pronunciation of interest
     */
    public GrammarPoint(SentenceHMMState state) {
        while (state != null) {
            if (state instanceof UnitState) {
                unitIndex = state.getWhich();
            } else if (state instanceof PronunciationState) {
                pronunciationIndex = state.getWhich();
            } else if (state instanceof WordState) {
                wordIndex = state.getWhich();
            } else if (state instanceof AlternativeState) {
                alternativeIndex = state.getWhich();
            } else if (state instanceof GrammarState) {
                node = ((GrammarState) state).getGrammarNode();
            }
            state = state.getParent();
        }
        assert node != null;
    }


    /**
     * Creates a grammar node that points to the first unit of the first pronunciation of the first word of the given
     * grammar node
     *
     * @param node the grammar node of interest
     */
    public GrammarPoint(GrammarNode node) {
        this(node, -1, 0, 0, 0);
    }


    /**
     * Creates a GrammarPoint that corresponds to the given unit of the given pronunciation
     *
     * @param state the pronunciation state
     * @param which the index of the unit
     */
    public GrammarPoint(PronunciationState state, int which) {
        this(state);
        unitIndex = which;
    }


    /**
     * Creates a GrammarPoint that points to a fully specified unit
     *
     * @param node               the grammar node
     * @param wordIndex          the index of the word in the node
     * @param pronunciationIndex the index of the pronunciation in the word.
     * @param unitIndex          the index of the unit in the pronunciation
     */
    public GrammarPoint(GrammarNode node, int alternativeIndex,
                        int wordIndex, int pronunciationIndex, int unitIndex) {
        assert node != null;
        this.node = node;
        this.alternativeIndex = alternativeIndex;
        this.wordIndex = wordIndex;
        this.pronunciationIndex = pronunciationIndex;
        this.unitIndex = unitIndex;
    }


    /**
     * Gets the unit associated with this point in the grammar
     *
     * @return the unit, or null if there is no unit associated with this point in the grammar
     */
    private Unit getUnit() {
        Unit unit = null;
        Word[][] alternatives = node.getAlternatives();
        if (alternativeIndex != -1 && alternativeIndex < alternatives.length) {
            Word[] words = alternatives[alternativeIndex];
            if (wordIndex < words.length) {
                Pronunciation[] pronunciations =
                        words[wordIndex].getPronunciations(null);
                if (pronunciationIndex < pronunciations.length) {
                    Unit[] units =
                            pronunciations[pronunciationIndex].getUnits();
                    if (unitIndex < units.length) {
                        unit = units[unitIndex];
                    }
                }
            }
        }
        return unit;
    }


    /**
     * Gets the unit associated with this point in the grammar. If there is no unit, return filler
     *
     * @return the unit for this grammar node or a filler unit
     */
    private Unit getUnitOrFill() {
        Unit unit = getUnit();
        if (unit == null) {
            unit = UnitManager.SILENCE;
        }
        return unit;
    }


    /**
     * Gets all of the right contexts for this grammar point. The contexts returned are guaranteed to be 'size' units
     * in length, The number of contexts returned depends upon the perplexity of the grammar downstream from this
     * GrammarPoint
     *
     * @param size             the size of each context returned
     * @param startWithCurrent include the current state in the context
     * @param maxContexts      the maxium number of right contexts to return
     * @return a list of containing Unit[] contexts.
     */
    public List<Unit[]> getRightContexts(int size, boolean startWithCurrent,
                                 int maxContexts) {
        List<Unit[]> contexts = new ArrayList<Unit[]>();
        List<GrammarPoint> nextPoints = getNextGrammarPoints(startWithCurrent);

        if (nextPoints.isEmpty()) {
            Unit[] units = Unit.getEmptyContext(size);
            addContext(contexts, units);
        } else {
            for (GrammarPoint gp : nextPoints) {
                if (size == 1) {
                    Unit[] units = new Unit[size];
                    units[0] = gp.getUnitOrFill();
                    addContext(contexts, units);
                } else {
                    List<Unit[]> rc = gp.getRightContexts(size - 1, false,
                        maxContexts - contexts.size());
                    for (Unit[] rcUnits : rc) {
                        Unit[] units = Unit.getEmptyContext(rcUnits.length + 1);
                        units[0] = gp.getUnitOrFill();
                        System.arraycopy(rcUnits, 0, units, 1, rcUnits.length);
                        addContext(contexts, units);
                    }
                }
                if (contexts.size() >= maxContexts) {
                    break;
                }
            }
        }
        return contexts;
    }


    /**
     * Add a context to a list of contexts after ensuring that no identical contexts exist on the list. When a right
     * context is collected it may contain duplicates in certain cases (when this unit is the last unit in a grammar
     * node, and there is a branch to multiple words in subsequent nodes, for instance)
     *
     * @param contexts the list of contexts to add the new units to
     * @param units    the units to add to the context
     */
    private void addContext(List<Unit[]> contexts, Unit[] units) {
        for (Unit[] onList : contexts) {
            if (Unit.isContextMatch(onList, units)) {
                return; // found on list so bailout
            }
        }
        contexts.add(units);
    }


    /**
     * Returns a list of next GrammarPoints for this GrammarPoint. If there are no more downstream grammar points with
     * words, an empty list is returned.
     *
     * @param startWithCurrent include the current state in the context
     * @return the (possibly empty) list of next GrammarPoint objects
     */
    private List<GrammarPoint> getNextGrammarPoints(boolean startWithCurrent) {
        List<GrammarPoint> nextPoints = new ArrayList<GrammarPoint>();
        int unitsLength = 0;

        // if this GrammarPoint is associated with a grammar node
        // and the grannar node has alternatives, add points for each
        // alternative
        if (alternativeIndex == -1 && node.getAlternatives().length > 0) {
            for (int i = 0; i < node.getAlternatives().length; i++) {
                GrammarPoint gp = new GrammarPoint(node, i, 0, 0, 0);
                nextPoints.add(gp);
            }
        }

        // If we don't have any alternatives, (i.e. this grammar node
        // has no words at all associated with it, then just go and
        // find the set of next grammar nodes with words, collect
        // them up, expand them and return that set.

        else if (node.getAlternatives().length == 0) {
            addNextGrammarPointsWithWords(node, nextPoints);
        } else {

            // At this point we are at a node with a set of alternatives

            GrammarPoint next;

            if (startWithCurrent) {
                next = this;
            } else {
                next = new GrammarPoint(node, alternativeIndex, wordIndex,
                        pronunciationIndex, unitIndex + 1);
            }
            Pronunciation[] pronunciations = node.
                    getAlternatives()[alternativeIndex][wordIndex].
                    getPronunciations(null);

            unitsLength = pronunciations[pronunciationIndex].getUnits().length;

            if (next.unitIndex < unitsLength) {
                nextPoints.add(next);
            } else {
                next.unitIndex = 0;
                Word[] alternative =
                        next.node.getAlternatives()[alternativeIndex];
                if (++next.wordIndex < alternative.length) {
                    Word word = alternative[next.wordIndex];
                    for (int i = 0; i < word.getPronunciations(null).length;
                         i++) {
                        GrammarPoint newGP = new GrammarPoint(next.node,
                                next.alternativeIndex, next.wordIndex, i, 0);
                        nextPoints.add(newGP);
                    }
                } else if (!bounded) {
                    addNextGrammarPointsWithWords(next.node, nextPoints);
                }
            }
        }
        return nextPoints;
    }


    /**
     * Given a GrammarNode return a list of successors GrammarNodes that contain words
     *
     * @param node successors are gathered from this node
     * @return list the list of grammar nodes
     */
    private static List<GrammarNode> getNextGrammarNodesWithWords(GrammarNode node) {
        List<GrammarNode> list = new ArrayList<GrammarNode>();

        for (GrammarArc arc : node.getSuccessors()) {
            GrammarNode gnode = arc.getGrammarNode();
            if (gnode.getAlternatives().length == 0) {
                if (gnode.isFinalNode()) {
                    list.add(gnode);
                } else {
                    list.addAll(getNextGrammarNodesWithWords(gnode));
                }
            } else {
                list.add(gnode);
            }
        }
        return list;
    }


    /**
     * Adds the next set of grammar points that contain words to the given list
     *
     * @param node       the grammar node
     * @param nextPoints where the grammar points should be added
     */
    private static void addNextGrammarPointsWithWords(GrammarNode
            node, List<GrammarPoint> nextPoints) {
        for (GrammarNode nextNode : getNextGrammarNodesWithWords(node)) {
            for (int j = 0; j < nextNode.getAlternatives().length; j++) {
                GrammarPoint gp = new GrammarPoint(nextNode, j, 0, 0, 0);
                nextPoints.add(gp);
            }
        }
    }


    /**
     * Sets the state of the bounded configuration flag
     *
     * @param state if true searches for context will not cross grammar nodes.
     */
    static void setBounded(boolean state) {
        bounded = state;
    }
}

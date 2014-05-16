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

package edu.cmu.sphinx.linguist.dictionary;

import java.util.List;

import edu.cmu.sphinx.linguist.acoustic.Unit;


/** Provides pronunciation information for a word. */
public class Pronunciation {

    public static final Pronunciation UNKNOWN = new Pronunciation(Unit.EMPTY_ARRAY, null, null, 1.0f);

    private Word word;
    private final WordClassification wordClassification;
    private final Unit[] units;
    private final String tag;
    private final float probability;


    /**
     * Creates a pronunciation
     *
     * @param units              represents the pronunciation
     * @param tag                a grammar specific tag
     * @param wordClassification the classification for this word
     * @param probability        the probability of this pronunciation occurring
     */
    protected Pronunciation(Unit[] units,
                  String tag,
                  WordClassification wordClassification,
                  float probability) {
        this.wordClassification = wordClassification;
        this.units = units;
        this.tag = tag;
        this.probability = probability;
    }

    /**
     * Creates a pronunciation
     *
     * @param units              represents the pronunciation
     * @param tag                a grammar specific tag
     * @param wordClassification the classification for this word
     * @param probability        the probability of this pronunciation occurring
     */
    protected Pronunciation(List<Unit> units,
                  String tag,
                  WordClassification wordClassification,
                  float probability) {
        Unit[] unitsArray = units.toArray(new Unit[units.size()]);
        this.wordClassification = wordClassification;
        this.units = unitsArray;
        this.tag = tag;
        this.probability = probability;
    }

    /**
     * Creates a pronunciation with defaults
     *
     * @param units              represents the pronunciation
     */
    protected Pronunciation(List<Unit> units) {
        this(units, null, null, 1.0f);
    }


    /**
     * Sets the word this pronunciation represents.
     *
     * @param word the Word this Pronunciation represents
     */
    protected void setWord(Word word) {
        if (this.word == null) {
            this.word = word;
        } else {
            throw new Error("Word of Pronunciation cannot be set twice.");
        }
    }


    /**
     * Retrieves the word that this Pronunciation object represents.
     *
     * @return the word
     */
    public Word getWord() {
        return word;
    }


    /**
     * Retrieves the word classification for this pronunciation
     *
     * @return the word classification for this pronunciation.
     */
    public WordClassification getWordClassification() {
        return wordClassification;
    }


    /**
     * Retrieves the units for this pronunciation
     *
     * @return the units for this pronunciation
     */
    public Unit[] getUnits() {
        return units;
    }


    /**
     * Retrieves the tag associated with the pronunciation or null if there is no tag associated with this
     * pronunciation. Pronunciations can optionally be tagged to allow applications to distinguish between different
     * pronunciations.
     *
     * @return the tag or null if no tag is available.
     */
    public String getTag() {
        return tag;
    }


    /**
     * Retrieves the probability for the pronunciation. A word may have multiple pronunciations that are not all equally
     * probable. All probabilities for particular word sum to 1.0.
     *
     * @return the probability of this pronunciation as a value between 0 and 1.0.
     *         <p/>
     *         TODO: FIX Note that probabilities are currently maintained in the linear domain (unlike just about
     *         everything else)
     */
    public float getProbability() {
        return probability;
    }


    /** Dumps a pronunciation */
    public void dump() {
        System.out.println(toString());
    }


    /**
     * Returns a string representation of this Pronunication.
     *
     * @return a string of this Pronunciation
     */
    @Override
    public String toString() {
        StringBuilder result = new StringBuilder().append(word).append('(');
        for (Unit unit : units) {
            result.append(unit).append(' ');
        }
        result.append(')');
        return result.toString();
    }


    /**
     * Returns a detailed string representation of this Pronunication.
     *
     * @return a string of this Pronunciation
     */
    public String toDetailedString() {
        StringBuilder result = new StringBuilder().append(word).append(' ');
        for (Unit unit : units) {
            result.append(unit).append(' ');
        }
        result.append("\n   class: ").append(wordClassification)
            .append(" tag: ").append(tag).append(" prob: ").append(probability);

        return result.toString();
    }
}


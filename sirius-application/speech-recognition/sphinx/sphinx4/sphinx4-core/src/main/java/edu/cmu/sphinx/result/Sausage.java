/*
 * Copyright 1999-2004 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 *
 * Created on Aug 11, 2004
 */

package edu.cmu.sphinx.result;

import edu.cmu.sphinx.util.LogMath;

import java.io.FileWriter;
import java.io.IOException;
import java.util.*;

/**
 * A Sausage is a sequence of confusion sets, one for each position in an utterance.
 *
 * @author pgorniak
 */

public class Sausage implements ConfidenceResult {

    protected final List<ConfusionSet> confusionSets;


    /**
     * Construct a new sausage.
     *
     * @param size The number of word slots in the sausage
     */
    public Sausage(int size) {
        confusionSets = new ArrayList<ConfusionSet>(size);
        for (int i = 0; i < size; i++) {
            confusionSets.add(new ConfusionSet());
        }
    }


    /**
     * Get an iterator for the sausage. The iterator will return SortedMaps, which are confusion sets mapping Double
     * posteriors to Sets of word Strings.
     *
     * @return an iterator that steps through confusion sets
     */
    public Iterator<ConfusionSet> iterator() {
        return confusionSets.iterator();
    }

    /**
     * Adds skip elements for each word slot in which the word posteriors do not add up to linear 1.
     */
    public void fillInBlanks() {
        LogMath logMath = LogMath.getInstance();
        int index = 0;
        for (ConfusionSet set : confusionSets) {
            float sum = LogMath.LOG_ZERO;
            for (Double val : set.keySet()) {
                sum = logMath.addAsLinear(sum, val.floatValue());
            }
            if (sum < LogMath.LOG_ONE - 10) {
                float remainder = logMath.subtractAsLinear
                    (LogMath.LOG_ONE, sum);
                addWordHypothesis(index, "<skip>", remainder);
            } else {
                ConfusionSet newSet = new ConfusionSet();
                for (Map.Entry<Double, Set<WordResult>> entry : set.entrySet()) {
                    Double oldProb = entry.getKey();
                    Double newProb = oldProb - sum;
                    newSet.put(newProb, entry.getValue());
                }
                confusionSets.set(index, newSet);
            }
            index++;
        }
    }


    /**
     * Add a word hypothesis to a given word slot in the sausage.
     *
     * @param position the position to add a hypothesis to
     * @param word     the word to add
     */
    public void addWordHypothesis(int position, WordResult word) {
        getConfusionSet(position).addWordHypothesis(word);
    }


    public void addWordHypothesis(int position, String word, double confidence)
    {
        WordResult wr = new WordResult(word, confidence);
        addWordHypothesis(position, wr);
    }


    /** @see edu.cmu.sphinx.result.ConfidenceResult#getBestHypothesis() */
    public Path getBestHypothesis() {
        return getBestHypothesis(true);
    }


    /**
     * Get the best hypothesis path discarding any filler words.
     *
     * @return the best path without fillers
     */
    public Path getBestHypothesisNoFiller() {
        return getBestHypothesis(false);
    }


    /**
     * Get the best hypothesis path optionally discarding any filler words.
     *
     * @param wantFiller whether to keep filler words
     * @return the best path
     */
    protected Path getBestHypothesis(boolean wantFiller) {
        WordResultPath path = new WordResultPath();
        for (ConfusionSet cs : this) {
            WordResult wr = cs.getBestHypothesis();
            if (wantFiller || !wr.isFiller()) {
                path.add(cs.getBestHypothesis());
            }
        }
        return path;
    }


    /**
     * Remove all filler words from the sausage. Also removes confusion sets that might've been emptied in the process
     * of removing fillers.
     */
    public void removeFillers() {
        for (Iterator<ConfusionSet> c = iterator(); c.hasNext();) {
            ConfusionSet cs = c.next();
            for (Iterator<Set<WordResult>> j = cs.values().iterator(); j.hasNext();) {
                Set<WordResult> words = j.next();
                Iterator<WordResult> w = words.iterator();
                while (w.hasNext()) {
                    WordResult word = w.next();
                    if (word.isFiller()) {
                        w.remove();
                    }
                }
                if (words.isEmpty()) {
                    j.remove();
                }
            }
            if (cs.isEmpty()) {
                c.remove();
            }
        }

    }


    /**
     * Get a string representing the best path through the sausage.
     *
     * @return best string
     */
    public String getBestHypothesisString() {
        return getBestHypothesis().toString();
    }


    /**
     * Get the word hypothesis with the highest posterior for a word slot
     *
     * @param pos the word slot to look at
     * @return the word with the highest posterior in the slot
     */
    public Set<WordResult> getBestWordHypothesis(int pos) {
        ConfusionSet set = confusionSets.get(pos);
        return set.get(set.lastKey());
    }


    /**
     * Get the the highest posterior for a word slot
     *
     * @param pos the word slot to look at
     * @return the highest posterior in the slot
     */

    public double getBestWordHypothesisPosterior(int pos) {
        return confusionSets.get(pos).lastKey();
    }


    /**
     * Get the confusion set stored in a given word slot.
     *
     * @param pos the word slot to look at.
     * @return a map from Double posteriors to Sets of String words, sorted from lowest to highest.
     */
    public ConfusionSet getConfusionSet(int pos) {
        return confusionSets.get(pos);
    }


    public int countWordHypotheses() {
        int count = 0;
        Iterator<ConfusionSet> i = iterator();
        while (i.hasNext()) {
        	ConfusionSet cs = i.next();
            for (Set<WordResult> words : cs.values()) {
                count += words.size();
            }
        }
        return count;
    }


    /**
     * size of this sausage in word slots.
     *
     * @return The number of word slots in this sausage
     */
    public int size() {
        return confusionSets.size();
    }


    /**
     * Write this sausage to an aisee format text file.
     *
     * @param fileName The file to write to.
     * @param title    the title to give the graph.
     */
    public void dumpAISee(String fileName, String title) {
        try {
            System.err.println("Dumping " + title + " to " + fileName);
            FileWriter f = new FileWriter(fileName);
            f.write("graph: {\n");
            f.write("title: \"" + title + "\"\n");
            f.write("display_edge_labels: yes\n");
            f.write("orientation: left_to_right\n");
            int index = 0;
            for (ConfusionSet set : confusionSets) {
                f.write("node: { title: \"" + index + "\" label: \"" + index + "\"}\n");
                for (Map.Entry<Double, Set<WordResult>> entry : set.entrySet()) {
                    Double prob = entry.getKey();
                    StringBuilder edge = new StringBuilder();
                    edge.append("edge: { sourcename: \"").append(index)
                        .append("\" targetname: \"").append(index + 1)
                        .append("\" label: \"");
                    Set<WordResult> wordSet = entry.getValue();
                    for (WordResult wordResult : wordSet)
                        edge.append(wordResult).append('/');
                    if (!wordSet.isEmpty())
                        edge.setLength(edge.length() - 1);
                    edge.append(':').append(prob).append("\" }\n");
                    f.write(edge.toString());
                }
                index++;
            }
            f.write("node: { title: \"" + size() + "\" label: \"" + size() + "\"}\n");
            f.write("}\n");
            f.close();
        } catch (IOException e) {
            throw new Error(e.toString());
        }
    }
}

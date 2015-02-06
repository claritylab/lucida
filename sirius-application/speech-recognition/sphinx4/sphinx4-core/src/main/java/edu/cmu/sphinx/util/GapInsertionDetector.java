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
package edu.cmu.sphinx.util;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.StringTokenizer;


/**
 * A program that takes in a reference transcript and a hypothesis transcript and figures out how many gap insertion
 * errors are there. The hypothesis transcript file should contain timestamps for when each word was entered and exited.
 * <p>The gap insertion detection algorithm works as follows. It takes each hypothesized word individually and see
 * whether it falls into a non-speech region in the reference transcript. If it does, that hypothesized word is counted
 * as a gap insertion.
 */
public class GapInsertionDetector {


    private ReferenceFile referenceFile;
    private HypothesisFile hypothesisFile;
    private boolean showGapInsertions;


    /**
     * Create a gap insertion detector to detect gap insertions using the given reference file and hypothesis file.
     *
     * @param referenceFile     the file of references
     * @param hypothesisFile    the file of hypotheses
     * @param showGapInsertions if true show gap insertions.
     */
    public GapInsertionDetector(String referenceFile, String hypothesisFile,
                                boolean showGapInsertions)
            throws IOException {
        this.referenceFile = new ReferenceFile(referenceFile);
        this.hypothesisFile = new HypothesisFile(hypothesisFile);
    }


    /**
     * Detect the gap insertion errors.
     *
     * @return the total number of gap insertion errors
     */
    public int detect() throws IOException {
        int gaps = 0;
        boolean done = false;
        ReferenceUtterance reference = referenceFile.nextUtterance();
        StringBuilder log = new StringBuilder();
        while (!done) {
            HypothesisWord word = hypothesisFile.nextWord();
            if (word != null) {
                boolean hasGapError = false;

                // go to the relevant reference utterance
                while (reference != null &&
                        reference.getEndTime() < word.getStartTime()) {
                    reference = referenceFile.nextUtterance();
                }

                // 'reference' should be the relevant one now
                if (reference != null) {
                    if (reference.isSilenceGap()) {
                        hasGapError = true;
                    } else {
                        while (reference.getEndTime() < word.getEndTime()) {
                            reference = referenceFile.nextUtterance();
                            if (reference == null ||
                                    reference.isSilenceGap()) {
                                hasGapError = true;
                                break;
                            }
                        }
                    }
                } else {
                    // if no more reference words, this is a gap insertion
                    hasGapError = true;
                }

                if (hasGapError) {
                    gaps++;
                    if (showGapInsertions) {
                        log.append("GapInsError: Utterance: ").append(hypothesisFile.getUtteranceCount())
                                .append(" Word: ").append(word.getText()).append(" (")
                                .append(word.getStartTime()).append(',').append(word.getEndTime()).append("). ");
                        if (reference != null) {
                            assert reference.isSilenceGap();
                            log.append("Reference: <sil> (").append(reference.getStartTime())
                                    .append(',').append(reference.getEndTime()).append(')');
                        }
                        log.append('\n');
                    }
                }
            } else {
                done = true;
            }
        }
        if (showGapInsertions) {
            System.out.println(log);
        }
        return gaps;
    }


    /**
     * A command line program for detecting gap insertion errors. To run this program, type: <code> java
     * GapInsertionDetector {propsFile} {referenceFile} {hypothesisFile} </code> The propsFile need to have only one
     * property: <code> edu.cmu.sphinx.util.GapInsertionDetector.showGapInsertions=true/false </code>
     */
    public static void main(String[] argv) {

        if (argv.length < 2) {
            System.out.println("Usage: java GapInsertionDetector " +
                    "<referenceFile> <hypothesisFile>");
        }
        try {
            String referenceFile = argv[0];
            String hypothesisFile = argv[1];

            GapInsertionDetector gid = new GapInsertionDetector
                    (referenceFile, hypothesisFile, true);
            System.out.println("# of gap insertions: " + gid.detect());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}

/**
 * Creates a ReferenceFile.
 */
class ReferenceFile {

    private BufferedReader reader;


    /**
     * Creates a ReferenceFile, given the name of the reference file.
     *
     * @param fileName the name of the reference file
     */
    ReferenceFile(String fileName) throws IOException {
        reader = new BufferedReader(new FileReader(fileName));
    }


    /**
     * Returns the next available ReferenceUtterance. This method skips all the silence gaps.
     *
     * @return the next available ReferenceUtterance, or null if the end of file has been reached.
     */
    ReferenceUtterance nextUtterance() throws IOException {
        String line = reader.readLine();
        if (line != null) {
            return new ReferenceUtterance(line);
        } else {
            return null;
        }
    }
}

/**
 * Converts a line in the HUB-4 .stm reference file into an object.
 */
class ReferenceUtterance {

    private boolean isSilenceGap;
    private final float startTime;
    private final float endTime;
    private final String[] words;


    /**
     * Creates a ReferenceUtterance from the given line of reference.
     *
     * @param line the line of reference, in the format: [test_name] [category] [speaker_name|"inter_segment_gap"]
     *             [start_time] [end_time] [<params>] [reference_text]
     */
    ReferenceUtterance(String line) {
        StringTokenizer st = new StringTokenizer(line);
        st.nextToken();                 // parse the test set name
        st.nextToken();                 // parse category
        String type = st.nextToken();   // parse speaker
        if (type.equals("inter_segment_gap")) {
            isSilenceGap = true;
        }
        startTime = Float.parseFloat(st.nextToken()); // parse start time
        endTime = Float.parseFloat(st.nextToken());   // parse end time

        if (st.hasMoreTokens()) {
            st.nextToken();                               // parse <...>
            words = new String[st.countTokens()];
            for (int i = 0; i < words.length; i++) {
                words[i] = st.nextToken();
            }
        } else {
            words = new String[0];
        }
    }


    /**
     * Returns true if this is a silence gap.
     *
     * @return true if this is a silence gap, false otherwise.
     */
    boolean isSilenceGap() {
        return isSilenceGap;
    }


    /**
     * Returns the starting time (in seconds) of this utterance.
     *
     * @return the starting time of this utterance
     */
    float getStartTime() {
        return startTime;
    }


    /**
     * Returns the ending time (in seconds) of this utterance.
     *
     * @return the ending time of this utterance
     */
    float getEndTime() {
        return endTime;
    }


    /**
     * Returns the text of this utterance.
     *
     * @return the text of this utterance
     */
    String[] getWords() {
        return words;
    }
}

class HypothesisFile {

    private BufferedReader reader;
    private Iterator<HypothesisWord> iterator;
    private int utteranceCount;


    /**
     * Creates a HypothesisFile from the given file.
     *
     * @param fileName the name of the hypothesis file
     */
    HypothesisFile(String fileName) throws IOException {
        reader = new BufferedReader(new FileReader(fileName));
    }


    /**
     * Returns the next hypothesized word in the hypothesis file.
     *
     * @return the next hypothesized word
     */
    HypothesisWord nextWord() throws IOException {
        if (iterator == null || !iterator.hasNext()) {
            HypothesisUtterance utterance = nextUtterance();
            if (utterance != null) {
                iterator = utterance.getWords().iterator();
            } else {
                iterator = null;
            }
        }
        if (iterator == null) {
            return null;
        } else {
            return iterator.next();
        }
    }


    /**
     * Returns the next available hypothesis utterance.
     *
     * @return the next available hypothesis utterance, or null if the end of file has been reached
     */
    private HypothesisUtterance nextUtterance() throws IOException {
        String line = reader.readLine();
        if (line != null) {
            utteranceCount++;
            HypothesisUtterance utterance = new HypothesisUtterance(line);
            if (utterance.getWordCount() <= 0) {
                return nextUtterance();
            } else {
                return utterance;
            }
        } else {
            return null;
        }
    }


    /**
     * Returns the utterance count.
     *
     * @return the utterance count
     */
    public int getUtteranceCount() {
        return utteranceCount;
    }
}

/**
 * A hypothesis utterance, which will give you a list of hypothesis words.
 */
class HypothesisUtterance {

    private final List<HypothesisWord> words;
    private float startTime;
    private float endTime;


    /**
     * Creates a hypothesis utterance from a line of input describing the hypothesis.
     */
    HypothesisUtterance(String line) {
        words = new LinkedList<HypothesisWord>();
        StringTokenizer st = new StringTokenizer(line, " \t\n\r\f(),");
        while (st.hasMoreTokens()) {
            String text = st.nextToken();
            try {
                float myStartTime = Float.parseFloat(st.nextToken());
                float myEndTime = Float.parseFloat(st.nextToken());
                HypothesisWord word = new HypothesisWord
                        (text, myStartTime, myEndTime);
                words.add(word);
            } catch (NumberFormatException nfe) {
                System.out.println("NumberFormatException at line: " + line);
                nfe.printStackTrace();
            }
        }
        if (!words.isEmpty()) {
            HypothesisWord firstWord = words.get(0);
            startTime = firstWord.getStartTime();
            HypothesisWord lastWord =
                    words.get(words.size() - 1);
            endTime = lastWord.getEndTime();
        }
    }


    /**
     * Returns the number of words in this hypothesis.
     *
     * @return the number of words in this hypothesis
     */
    int getWordCount() {
        return words.size();
    }


    /**
     * Returns a list of the words in this hypothesis.
     *
     * @return a list of the words in this hypothesis
     */
    List<HypothesisWord> getWords() {
        List<HypothesisWord> newList = new LinkedList<HypothesisWord>();
        newList.addAll(words);
        return newList;
    }


    /**
     * Returns the start time of this hypothesis.
     *
     * @return the start time of this hypothesis
     */
    float getStartTime() {
        return startTime;
    }


    /**
     * Returns the end time of this hypothesis.
     *
     * @return the end time of this hypothesis
     */
    float getEndTime() {
        return endTime;
    }
}

/**
 * A word in the hypothesis, containing information about when the word started and ended.
 */
class HypothesisWord {

    private final String text;
    private final float startTime;
    private final float endTime;


    /**
     * Constructs a hypothesis word with the given start and end times.
     *
     * @param text      the text of the hypothesized word
     * @param startTime the starting time of the word
     * @param endTime   the ending time of the word
     */
    HypothesisWord(String text, float startTime, float endTime) {
        this.text = text;
        this.startTime = startTime;
        this.endTime = endTime;
    }


    /**
     * Returns the text of the word.
     *
     * @return the text of the word
     */
    String getText() {
        return text;
    }


    /**
     * Returns the starting time of the word.
     *
     * @return the starting time of the word
     */
    float getStartTime() {
        return startTime;
    }


    /**
     * Returns the ending time of the word.
     *
     * @return the ending time of the word
     */
    float getEndTime() {
        return endTime;
    }
}

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
package edu.cmu.sphinx.tools.live;

import edu.cmu.sphinx.frontend.util.ConcatFileDataSource;
import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.util.*;
import edu.cmu.sphinx.util.props.*;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.net.URL;
import java.util.LinkedList;
import java.util.List;

/**
 * Decodes a batch file containing a list of files to decode. The files can be either audio files or cepstral files, but
 * defaults to audio files.
 */
public class LiveModeRecognizer implements Configurable {

    /** The property that specifies the recognizer to use */
    @S4Component(type = Recognizer.class)
    public final static String PROP_RECOGNIZER = "recognizer";

    /** The property that specifies the source of the transcript */
    @S4Component(type = ConcatFileDataSource.class)
    public final static String PROP_INPUT_SOURCE = "inputSource";

    /** The property specifying whether to print out the gap insertion errors. */
    @S4Boolean(defaultValue = false)
    public static final String PROP_SHOW_GAP_INSERTIONS = "showGapInsertions";

    /** The property specifying the transcript file. */
    @S4String(defaultValue = "hypothesis.txt")
    public final static String PROP_HYPOTHESIS_TRANSCRIPT = "hypothesisTranscript";

    /** The property specifying the number of files to decode before alignment is performed. */
    @S4Integer(defaultValue = -1)
    public final static String PROP_ALIGN_INTERVAL = "alignInterval";

    // TODO - the instrumentation in here that is looking for gap insertions
    // and performing the alignment and reporting of the live summary data
    // should probably be moved to a separate instrumentation package, much
    // like the BestPathAccuracyTracker.

    // -------------------------------
    // Configuration data
    // --------------------------------
    private Recognizer recognizer;
    private ConcatFileDataSource dataSource;
    private String hypothesisFile;
    private boolean showGapInsertions;

    // -------------------------------
    // Working data
    // --------------------------------
    private int alignInterval;
    private int numUtterances;

    private FileWriter hypothesisTranscript;
    private ReferenceSource referenceSource;
    private final NISTAlign aligner = new NISTAlign(true, true);

    public LiveModeRecognizer( Recognizer recognizer, ConcatFileDataSource dataSource, int skip, boolean showGapInsertions, String hypothesisFile, int alignInterval ) {
        this.recognizer = recognizer;
        this.dataSource = dataSource;
        this.showGapInsertions = showGapInsertions;
        this.hypothesisFile = hypothesisFile;
        this.alignInterval = alignInterval;
        this.referenceSource = dataSource;
    }

    public LiveModeRecognizer() {

    }
    
    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        recognizer = (Recognizer) ps.getComponent(PROP_RECOGNIZER);
        dataSource = (ConcatFileDataSource) ps.getComponent(PROP_INPUT_SOURCE);
        showGapInsertions = ps.getBoolean(PROP_SHOW_GAP_INSERTIONS);

        hypothesisFile = ps.getString(PROP_HYPOTHESIS_TRANSCRIPT);

        alignInterval = ps.getInt(PROP_ALIGN_INTERVAL);

        referenceSource = dataSource;
    }


    /** Decodes the batch of audio files */
    public void decode() throws IOException {
        List<String> resultList = new LinkedList<String>();
        Result result;
        int startReference = 0;
        hypothesisTranscript = new FileWriter(hypothesisFile);
        recognizer.allocate();
        while ((result = recognizer.recognize()) != null) {
            numUtterances++;
            String resultText = result.getBestResultNoFiller();

            System.out.println("\nHYP: " + resultText);
            System.out.println("   Sentences: " + numUtterances);
            resultList.add(resultText);

            hypothesisTranscript.write(result.getTimedBestResult(false, true)
                    + '\n');
            hypothesisTranscript.flush();

            if (alignInterval > 0 && (numUtterances % alignInterval == 0)) {
                // perform alignment if the property 'alignInterval' is set
                List<String> references = referenceSource.getReferences();
                List<String> section = references.subList(startReference, references
                        .size());
                alignResults(resultList, section);
                resultList = new LinkedList<String>();
                startReference = references.size();
            }
        }

        hypothesisTranscript.close();

        // perform alignment on remaining results
        List<String> references = referenceSource.getReferences();
        List<String> section = references.subList(startReference, references.size());
        if (!resultList.isEmpty() || !section.isEmpty()) {
            alignResults(resultList, section);
        }
        System.out.println("# ------------- Summary Statistics -------------");
        aligner.printTotalSummary();

        recognizer.deallocate();

        showLiveSummary();
        System.out.println();
    }


    /** Shows the test statistics that relates to live mode decoding. */
    private void showLiveSummary() throws IOException {
        int actualUtterances = referenceSource.getReferences().size();
        int gapInsertions = detectGapInsertionErrors();

        System.out.println
                ("   Utterances:  Actual: " + actualUtterances +
                        "  Found: " + numUtterances);
        System.out.println
                ("   Gap Insertions: " + gapInsertions);
    }


    /** Detect gap insertion errors. */
    private int detectGapInsertionErrors() throws IOException {
        Timer gapTimer = TimerPool.getTimer(this, "GapInsertionDetector");
        gapTimer.start();
        GapInsertionDetector gid = new GapInsertionDetector(dataSource
                .getTranscriptFile(), hypothesisFile, showGapInsertions);
        int gapInsertions = gid.detect();
        gapTimer.stop();
        return gapInsertions;
    }


    /**
     * Align the list of results with reference text. This method figures out how many words and sentences match, and
     * the different types of errors.
     *
     * @param hypothesisList the list of hypotheses
     * @param referenceList  the list of references
     */
    private void alignResults(List<String> hypothesisList, List<String> referenceList) {
        System.out.println();
        System.out.println("Aligning results...");
        System.out.println("   Utterances: Found: " + hypothesisList.size()
                + "   Actual: " + referenceList.size());

        String hypothesis = listToString(hypothesisList);
        String reference = listToString(referenceList);
        saveAlignedText(hypothesis, reference);

        getAlignTimer().start();
        aligner.align(reference, hypothesis);
        getAlignTimer().stop();

        System.out.println(" ...done aligning");
        System.out.println();
    }


    /**
     * Saves the aligned hypothesis and reference text to the aligned text file.
     *
     * @param hypothesis the aligned hypothesis text
     * @param reference  the aligned reference text
     */
    private void saveAlignedText(String hypothesis, String reference) {
        try {
            FileWriter writer = new FileWriter("align.txt");
            writer.write(hypothesis);
            writer.write("\n");
            writer.write(reference);
            writer.close();
        } catch (IOException ioe) {
            ioe.printStackTrace();
        }
    }


    /**
     * Converts the given list of strings into one string, putting a space character in between the strings.
     *
     * @param resultList the list of strings
     * @return a string which is a concatenation of the strings in the list, separated by a space character
     */
    private String listToString(List<String> resultList) {
        StringBuilder sb = new StringBuilder();
        for (String result : resultList) {
            sb.append(result).append(' ');
        }
        return sb.toString();
    }


    /** Return the timer for alignment. */
    private Timer getAlignTimer() {
        return TimerPool.getTimer(this, "Align");
    }


    /** Do clean up */
    public void close() throws IOException {
        hypothesisTranscript.close();
    }


    /**
     * Main method of this BatchDecoder.
     *
     * @param argv argv[0] : config file argv[1] : a file listing all the audio files to decode
     */
    public static void main(String[] argv) {
        if (argv.length != 1) {
            System.out.println("Usage: LiveModeRecognizer config-file.xml ");
            System.exit(1);
        }
        String cmFile = argv[0];
        ConfigurationManager cm;
        LiveModeRecognizer lmr;

        try {
            URL url = new File(cmFile).toURI().toURL();
            cm = new ConfigurationManager(url);
            lmr = (LiveModeRecognizer) cm.lookup("live");
        } catch (IOException ioe) {
            System.err.println("I/O error during initialization: \n   " + ioe);
            return;
        } catch (PropertyException e) {
            System.err.println("Error during initialization: \n  " + e);
            e.printStackTrace();
            return;
        }

        if (lmr == null) {
            System.err.println("Can't find liveModeRecognizer in " + cmFile);
            return;
        }

        try {
            lmr.decode();
        } catch (IOException ioe) {
            System.err
                    .println("I/O error during decoding: " + ioe.getMessage());
        }
    }

//
//    @Test
//    public void testUseConcatDataSource() {
//        Map<String, Object> props = new HashMap<String, Object>();
//        props.put("inputSource", new ConcatFileDataSource());
//        props.put("recognizer", new Recognizer());
//        LiveModeRecognizer liveRecognizer = (LiveModeRecognizer) ConfigurationManager.getInstance(LiveModeRecognizer.class, props);
//
//    }
}

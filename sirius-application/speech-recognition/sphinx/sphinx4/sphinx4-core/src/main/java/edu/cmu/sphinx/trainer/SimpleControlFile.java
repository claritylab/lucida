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

package edu.cmu.sphinx.trainer;

import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.S4Component;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Logger;

/** Provides mechanisms for accessing a next utterance's file name and transcription. */
public class SimpleControlFile implements ControlFile {
	
	@S4Component(type = TrainerDictionary.class)
	public static final String DICTIONARY = "dictionary";
	private TrainerDictionary dictionary;

    private String audioFile;           // the audio file
    private String transcriptFile;      // the transcript file
    private String wordSeparator;       // the word separator
    private int currentPartition;       // the current partition
    private int numberOfPartitions;     // total number of partitions
    private Iterator<String> audioFileIterator; // iterator for the control file
    private Iterator<String> transcriptFileIterator; // iterator for the transcriptions
    private List<String> audioFileList;         // list containing the audio files
    private List<String> transcriptFileList;    // list containing the transcriptions


    /*
    * The logger for this class
    */
    private Logger logger;


    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();
        this.dictionary = (TrainerDictionary)ps.getComponent(DICTIONARY);
        try {
			dictionary.allocate();
		} catch (IOException e) {
			throw new PropertyException(e);
		}

        this.audioFile = ps.getString(PROP_AUDIO_FILE);
        this.transcriptFile = ps.getString(PROP_TRANSCRIPT_FILE);
        this.currentPartition = ps.getInt(PROP_WHICH_BATCH);
        this.numberOfPartitions = ps.getInt(PROP_TOTAL_BATCHES);
       

        logger.info("Audio control file: " + this.audioFile);
        logger.info("Transcript file: " + this.transcriptFile);
        this.wordSeparator = " \t\n\r\f"; // the white spaces
        logger.info("Processing part " + this.currentPartition +
                " of " + this.numberOfPartitions);
        try {
            this.audioFileList = getLines(audioFile);
        } catch (IOException ioe) {
            throw new Error("IOE: Can't open file " + audioFile, ioe);
        }
        try {
            this.transcriptFileList = getLines(transcriptFile);
        } catch (IOException ioe) {
            throw new Error("IOE: Can't open file " + transcriptFile, ioe);
        }
    }


    /** Gets an iterator for utterances. */
    public void startUtteranceIterator() {
        audioFileIterator = audioFileList.iterator();
        transcriptFileIterator = transcriptFileList.iterator();
    }


    /**
     * Returns whether there is another utterance.
     *
     * @return true if there is another utterance.
     */
    public boolean hasMoreUtterances() {
        // Should throw exception or break if one has next and the
        // other doesn't.
        return (audioFileIterator.hasNext()
                && transcriptFileIterator.hasNext());
    }


    /**
     * Gets the next utterance.
     *
     * @return the next utterance.
     */
    public Utterance nextUtterance() {
        logger.fine("processing ext utterance");
        
        String utteranceLine = audioFileIterator.next()  + ".mfc";
        Utterance utterance = new SimpleUtterance(utteranceLine);
        String utteranceFilename =
                utteranceLine.replaceFirst("^.*/", "").replaceFirst("\\..*$", "");
        String transcriptLine = transcriptFileIterator.next();
        // Finds out if the audio file name is part of the transcript line
        assert transcriptLine.matches(".*[ \t]\\(" + utteranceFilename + "\\)$") :
                "File name in transcript \"" + transcriptLine +
                        "\" and control file \"" + utteranceFilename +
                        "\" have to match.";
        // Removes the filename from the transcript line.
        // The transcript line is of the form:
        //    She washed her dark suit (st002)
        String transcript = transcriptLine.replaceFirst("[ \t]\\(.*\\)$", "");
        utterance.add(transcript, dictionary, false, wordSeparator);
        return utterance;
    }

    // Next method copied from decoder.BatchDecoder


    /**
     * Gets the set of lines from the file.
     *
     * @param file the name of the file
     * @throws IOException if error occurs while reading file
     */
    private List<String> getLines(String file) throws IOException {
        List<String> list = new ArrayList<String>();
        BufferedReader reader
                = new BufferedReader(new FileReader(file));

        String line = null;

        while ((line = reader.readLine()) != null) {
            list.add(line);
        }
        reader.close();

        if (numberOfPartitions > 1) {
            int linesPerBatch = list.size() / numberOfPartitions;
            if (linesPerBatch < 1) {
                linesPerBatch = 1;
            }
            if (currentPartition >= numberOfPartitions) {
                currentPartition = numberOfPartitions - 1;
            }
            int startLine = currentPartition * linesPerBatch;
            // last batch needs to get all remaining lines
            if (currentPartition == (numberOfPartitions - 1)) {
                list = list.subList(startLine, list.size());
            } else {
                list = list.subList(startLine, startLine +
                        linesPerBatch);
            }
        }
        return list;
    }
}

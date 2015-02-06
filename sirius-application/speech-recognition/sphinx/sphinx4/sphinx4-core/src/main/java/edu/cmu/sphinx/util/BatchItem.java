/*
 * Copyright 2004 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.util;


/** Provides a standard interface to for a single decode in a batch of decodes */
public class BatchItem {

    private final String filename;
    private final String transcript;


    /**
     * Creates a batch item
     *
     * @param filename   the filename
     * @param transcript the transcript
     */
    public BatchItem(String filename, String transcript) {
        this.filename = filename;
        this.transcript = transcript;
    }


    /**
     * Gets the filename for this batch
     *
     * @return the file name
     */
    public String getFilename() {
        return filename;
    }


    /**
     * Gets the transcript for the batch
     *
     * @return the transcript (or null if there is no transcript)
     */
    public String getTranscript() {
        return transcript;
    }
}
  

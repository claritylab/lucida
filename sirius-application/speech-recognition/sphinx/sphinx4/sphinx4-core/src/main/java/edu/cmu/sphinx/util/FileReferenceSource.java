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

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.LinkedList;
import java.util.List;

/** A source of reference texts. */
public class FileReferenceSource implements ReferenceSource {

    private final List<String> references;


    /**
     * Constructs a ReferenceSource from a reference file.
     *
     * @param file the reference file
     */
    public FileReferenceSource(String file) throws IOException {
        references = new LinkedList<String>();
        BufferedReader reader = new BufferedReader(new FileReader(file));
        String line = null;
        while ((line = reader.readLine()) != null) {
            if (!line.startsWith(";;")) {
                int fromIndex = 0;
                boolean isSilence = false;
                for (int i = 0; i < 6; i++) {
                    if (i == 2) {
                        String type = line.substring(fromIndex);
                        if (type.startsWith("inter_segment_gap")) {
                            isSilence = true;
                            break;
                        }
                    }
                    fromIndex = line.indexOf(' ', fromIndex) + 1;
                }
                if (!isSilence) {
                    String reference = line.substring(fromIndex).trim();
                    // System.out.println("REF: " + reference);
                    references.add(reference);
                }
            }
        }
        reader.close();
    }


    /**
     * Returns a list of reference text.
     *
     * @return a list of reference text
     */
    public List<String> getReferences() {
        return references;
    }
}

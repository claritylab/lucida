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
import java.util.ArrayList;
import java.util.List;


/** Provides a set of utilities methods for manipulating batch files. */
public class BatchFile {

    /**
     * Returns a List of the lines in a batch file.
     *
     * @param batchFile the batch file to read
     * @return a List of the lines in a batch file
     */
    public static List<String> getLines(String batchFile) throws IOException {
        return getLines(batchFile, 0);
    }


    /**
     * Returns a List of the lines in a batch file.
     *
     * @param batchFile the batch file to read
     * @param skip      the number of lines to skip between items
     * @return a List of the lines in a batch file
     */
    public static List<String> getLines(String batchFile, int skip) throws IOException {
        int curCount = skip;
        List<String> list = new ArrayList<String>();
        BufferedReader reader = new BufferedReader(new FileReader(batchFile));

        String line = null;

        while ((line = reader.readLine()) != null) {
            if (!line.isEmpty()) {
                if (++curCount >= skip) {
                    list.add(line);
                    curCount = 0;
                }
            }
        }
        reader.close();
        return list;
    }


    /**
     * Returns the file name portion of a line in a batch file. This is the portion of the line before the first space.
     *
     * @return the file name portion of a line in a batch file.
     */
    public static String getFilename(String batchFileLine) {
        int firstSpace = batchFileLine.indexOf(' ');
        return batchFileLine.substring(0, firstSpace).trim();
    }


    /**
     * Returns the reference string portion of a line in a batch file. This is the portion of the line after the first
     * space
     *
     * @return the reference string portion of a line in a batch file.
     */
    public static String getReference(String batchFileLine) {
        int firstSpace = batchFileLine.indexOf(' ');
        return batchFileLine.substring(firstSpace + 1).trim();
    }
}

  

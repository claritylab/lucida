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
package edu.cmu.sphinx.result;

import edu.cmu.sphinx.linguist.dictionary.Dictionary;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;


/**
 * Creates a Lattice from a GDL (AISee) Lattice file. One can obtain such a GDL file from a lattice by calling the
 * <code>Lattice.dumpAISee</code> method.
 */
public class GDLLatticeFactory {

    private GDLLatticeFactory() {
    }


    /**
     * Create a Lattice from a GDL (AISee) Lattice file.
     *
     * @param gdlFile    the lattice file
     * @param dictionary the dictionary to use to look up words
     */
    public static Lattice getLattice(String gdlFile, Dictionary dictionary)
            throws IOException {
        Lattice lattice = new Lattice();

        BufferedReader reader = new BufferedReader(new FileReader(gdlFile));
        String line = null;

        while ((line = reader.readLine()) != null) {
            if (line.startsWith("node")) {
                createNode(line, lattice, dictionary);
            } else if (line.startsWith("edge")) {
                createEdge(line, lattice);
            }
        }
        reader.close();
        return lattice;
    }


    private static void createNode(String line, Lattice lattice,
                                   Dictionary dictionary) {
        String[] text = line.split("\\s");
        String id = text[3].substring(1, text[3].length() - 1);
        String contents = text[5].substring(1);
        String posterior = text[6].substring(2, text[6].length() - 2);

        String word = contents.substring(0, contents.indexOf('['));
        contents = contents.substring(contents.indexOf('[') + 1);

        String start = contents.substring(0, contents.indexOf(','));
        String end = contents.substring(contents.indexOf(',') + 1);

        Node node = lattice.addNode(id, dictionary.getWord(word),
                Integer.parseInt(start),
                Integer.parseInt(end));
        node.setPosterior(Double.parseDouble(posterior));

        if (word.equals("<s>")) {
            lattice.setInitialNode(node);
        } else if (word.equals("</s>")) {
            lattice.setTerminalNode(node);
        }
    }


    private static void createEdge(String line, Lattice lattice) {
        String[] text = line.split("\\s");
        String src = text[3].substring(1, text[3].length() - 1);
        String dest = text[5].substring(1, text[5].length() - 1);
        String contents = text[7].substring(1, text[7].length() - 1);
        String[] scores = contents.split(",");

        lattice.addEdge(lattice.getNode(src), lattice.getNode(dest),
                Double.parseDouble(scores[0]),
                Double.parseDouble(scores[1]));
    }
}

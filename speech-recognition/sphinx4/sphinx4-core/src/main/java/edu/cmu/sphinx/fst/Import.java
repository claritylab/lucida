/**
 * 
 * Copyright 1999-2012 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.fst;

import java.io.IOException;

import edu.cmu.sphinx.fst.semiring.TropicalSemiring;

/**
 * Provides a command line utility to convert an Fst in openfst's text format to
 * java binary fst model
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class Import {

    /**
     * Default Constructor
     */
    private Import() {
    }

    /**
     * Imports an openfst text format and serializes it as java binary model
     * Several files are imported as follows: - basename.input.syms -
     * basename.output.syms - basename.fst.txt
     * 
     * @param args[0] openfst's files basename
     * @param args[1] the java binary model filename
     * @throws IOException 
     * @throws NumberFormatException 
     */
    public static void main(String[] args) throws NumberFormatException, IOException {
        if (args.length < 2) {
            System.err.println("Input and output files not provided");
            System.err
                    .println("You need to provide both the input binary openfst model");
            System.err.println("and the output serialized java fst model.");
            System.exit(1);
        }

        Fst fst = Convert.importFst(args[0], new TropicalSemiring());

        // Serialize the java fst model to disk
        System.out.println("Saving as binary java fst model...");
        try {
            fst.saveModel(args[1]);

        } catch (IOException e) {
            System.err.println("Cannot write to file " + args[1]);
            System.exit(1);
        }
    }
}

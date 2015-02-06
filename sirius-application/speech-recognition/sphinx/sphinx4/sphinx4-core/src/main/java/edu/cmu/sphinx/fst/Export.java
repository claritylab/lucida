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


/**
 * Provides a command line utility to convert a java binary fst model to
 * openfst's text format
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class Export {
    /**
     * Default Constructor
     */
    private Export() {
    }

    /**
     * Exports a java binary model to openfst text format  
     * Several files are exported as follows:
     * - basename.input.syms
     * - basename.output.syms
     * - basename.fst.txt
     * 
     * @param args[0] the java binary model filename 
     * @param args[1] openfst's files basename
     * @throws IOException 
     * @throws ClassNotFoundException 
     */
    public static void main(String[] args) throws IOException, ClassNotFoundException {
        if (args.length < 2) {
            System.err.println("Input and output files not provided");
            System.err
                    .println("You need to provide both the input serialized java fst model");
            System.err.println("and the output binary openfst model.");
            System.exit(1);
        }

        Fst fst = Fst.loadModel(args[0]);

        // Serialize the java fst model to disk
        System.out.println("Saving as openfst text model...");
        Convert.export(fst, args[1]);
    }

}

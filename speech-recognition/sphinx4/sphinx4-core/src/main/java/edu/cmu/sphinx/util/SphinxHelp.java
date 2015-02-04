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

/** This class provides information on Sphinx-4 and how to use it */

public class SphinxHelp {

    private final static String VERSION = "1.0 beta6";

    /**
     * The main entry point
     *
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        System.out.println();
        System.out.println("Welcome to Sphinx-4!");
        System.out.println();
        System.out.println("   This is version " + VERSION + '.');
        System.out.println("   For information on how to configure and run");
        System.out.println("   Sphinx-4 please read:");
        System.out.println("   http://cmusphinx.sourceforge.net/sphinx4");
    }
}

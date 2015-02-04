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

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.util.HashMap;

import edu.cmu.sphinx.fst.semiring.Semiring;
import edu.cmu.sphinx.fst.utils.Utils;

/**
 * Provides the required functionality in order to convert from/to openfst's
 * text format
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class Convert {

    /**
     * Default private Constructor.
     */
    private Convert() {
    }

    /**
     * Exports an fst to the openfst text format Several files are created as
     * follows: - basename.input.syms - basename.output.syms - basename.fst.txt
     * See <a
     * href="http://www.openfst.org/twiki/bin/view/FST/FstQuickTour">OpenFst
     * Quick Tour</a>
     * 
     * @param fst
     *            the fst to export
     * @param basename
     *            the files' base name
     * @throws IOException
     */
    public static void export(Fst fst, String basename) throws IOException {
        exportSymbols(fst.getIsyms(), basename + ".input.syms");
        exportSymbols(fst.getOsyms(), basename + ".output.syms");
        exportFst(fst, basename + ".fst.txt");
    }

    /**
     * Exports an fst to the openfst text format
     * 
     * @param fst
     *            the fst to export
     * @param filename
     *            the openfst's fst.txt filename
     * @throws IOException
     */
    private static void exportFst(Fst fst, String filename) throws IOException {
        FileWriter file;

        file = new FileWriter(filename);
        PrintWriter out = new PrintWriter(file);

        // print start first
        State start = fst.getStart();
        out.println(start.getId() + "\t" + start.getFinalWeight());

        // print all states
        int numStates = fst.getNumStates();
        for (int i = 0; i < numStates; i++) {
            State s = fst.getState(i);
            if (s.getId() != fst.getStart().getId()) {
                out.println(s.getId() + "\t" + s.getFinalWeight());
            }
        }

        String[] isyms = fst.getIsyms();
        String[] osyms = fst.getOsyms();
        numStates = fst.getNumStates();
        for (int i = 0; i < numStates; i++) {
            State s = fst.getState(i);
            int numArcs = s.getNumArcs();
            for (int j = 0; j < numArcs; j++) {
                Arc arc = s.getArc(j);
                String isym = (isyms != null) ? isyms[arc.getIlabel()]
                        : Integer.toString(arc.getIlabel());
                String osym = (osyms != null) ? osyms[arc.getOlabel()]
                        : Integer.toString(arc.getOlabel());

                out.println(s.getId() + "\t" + arc.getNextState().getId()
                        + "\t" + isym + "\t" + osym + "\t" + arc.getWeight());
            }
        }

        out.close();

    }

    /**
     * Exports a symbols' map to the openfst text format
     * 
     * @param syms
     *            the symbols' map
     * @param filename
     *            the the openfst's symbols filename
     * @throws IOException
     */
    private static void exportSymbols(String[] syms, String filename)
            throws IOException {
        if (syms == null)
            return;

        FileWriter file = new FileWriter(filename);
        PrintWriter out = new PrintWriter(file);

        for (int i = 0; i < syms.length; i++) {
            String key = syms[i];
            out.println(key + "\t" + i);
        }

        out.close();

    }

    /**
     * Imports an openfst's symbols file
     * 
     * @param filename
     *            the symbols' filename
     * @return HashMap containing the imported string-to-id mapping
     * @throws IOException
     * @throws NumberFormatException
     */
    private static HashMap<String, Integer> importSymbols(String filename)
            throws NumberFormatException, IOException {

        File symfile = new File(filename);
        if (!(symfile.exists() && symfile.isFile())) {
            return null;
        }
        
        FileInputStream fis = new FileInputStream(filename); 
        DataInputStream dis = new DataInputStream(fis);
        BufferedReader br = new BufferedReader(new InputStreamReader(dis));
        HashMap<String, Integer> syms = new HashMap<String, Integer>();
        String strLine;

        while ((strLine = br.readLine()) != null) {
            String[] tokens = strLine.split("\\t");
            String sym = tokens[0];
            Integer index = Integer.parseInt(tokens[1]);
            syms.put(sym, index);

        }
        br.close();

        return syms;
    }

    /**
     * Imports an openfst text format Several files are imported as follows: -
     * basename.input.syms - basename.output.syms - basename.fst.txt
     * 
     * @param basename
     *            the files' base name
     * @param semiring
     *            the fst's semiring
     * @throws IOException
     * @throws NumberFormatException
     */
    public static Fst importFst(String basename, Semiring semiring)
            throws NumberFormatException, IOException {
        Fst fst = new Fst(semiring);

        HashMap<String, Integer> isyms = importSymbols(basename + ".input.syms");
        if (isyms == null) {
            isyms = new HashMap<String, Integer>();
            isyms.put("<eps>", 0);
        }

        HashMap<String, Integer> osyms = importSymbols(basename
                + ".output.syms");
        if (osyms == null) {
            osyms = new HashMap<String, Integer>();
            osyms.put("<eps>", 0);
        }

        HashMap<String, Integer> ssyms = importSymbols(basename
                + ".states.syms");

        // Parse input
        FileInputStream fis = new FileInputStream(basename + ".fst.txt");

        DataInputStream dis = new DataInputStream(fis);
        BufferedReader br = new BufferedReader(new InputStreamReader(dis));
        boolean firstLine = true;
        String strLine;
        HashMap<Integer, State> stateMap = new HashMap<Integer, State>();

        while ((strLine = br.readLine()) != null) {
            String[] tokens = strLine.split("\\t");
            Integer inputStateId;
            if (ssyms == null) {
                inputStateId = Integer.parseInt(tokens[0]);
            } else {
                inputStateId = ssyms.get(tokens[0]);
            }
            State inputState = stateMap.get(inputStateId);
            if (inputState == null) {
                inputState = new State(semiring.zero());
                fst.addState(inputState);
                stateMap.put(inputStateId, inputState);
            }

            if (firstLine) {
                firstLine = false;
                fst.setStart(inputState);
            }

            if (tokens.length > 2) {
                Integer nextStateId;
                if (ssyms == null) {
                    nextStateId = Integer.parseInt(tokens[1]);
                } else {
                    nextStateId = ssyms.get(tokens[1]);
                }

                State nextState = stateMap.get(nextStateId);
                if (nextState == null) {
                    nextState = new State(semiring.zero());
                    fst.addState(nextState);
                    stateMap.put(nextStateId, nextState);
                }
                // Adding arc
                if (isyms.get(tokens[2]) == null) {
                    isyms.put(tokens[2], isyms.size());
                }
                int iLabel = isyms.get(tokens[2]);
                if (osyms.get(tokens[3]) == null) {
                    osyms.put(tokens[3], osyms.size());
                }
                int oLabel = osyms.get(tokens[3]);

                float arcWeight;
                if (tokens.length > 4) {
                    arcWeight = Float.parseFloat(tokens[4]);
                } else {
                    arcWeight = 0;
                }
                Arc arc = new Arc(iLabel, oLabel, arcWeight, nextState);
                inputState.addArc(arc);
            } else {
                if (tokens.length > 1) {
                    float finalWeight = Float.parseFloat(tokens[1]);
                    inputState.setFinalWeight(finalWeight);
                } else {
                    inputState.setFinalWeight(0.0f);
                }
            }
        }
        dis.close();

        fst.setIsyms(Utils.toStringArray(isyms));
        fst.setOsyms(Utils.toStringArray(osyms));

        return fst;
    }
}

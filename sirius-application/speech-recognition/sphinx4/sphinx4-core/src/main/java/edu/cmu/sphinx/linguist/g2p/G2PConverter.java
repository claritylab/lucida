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

package edu.cmu.sphinx.linguist.g2p;

import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Queue;
import java.util.regex.Pattern;

import edu.cmu.sphinx.fst.Arc;
import edu.cmu.sphinx.fst.Fst;
import edu.cmu.sphinx.fst.ImmutableFst;
import edu.cmu.sphinx.fst.State;
import edu.cmu.sphinx.fst.operations.ArcSort;
import edu.cmu.sphinx.fst.operations.Compose;
import edu.cmu.sphinx.fst.operations.ILabelCompare;
import edu.cmu.sphinx.fst.operations.NShortestPaths;
import edu.cmu.sphinx.fst.operations.OLabelCompare;
import edu.cmu.sphinx.fst.operations.Project;
import edu.cmu.sphinx.fst.operations.ProjectType;
import edu.cmu.sphinx.fst.operations.RmEpsilon;
import edu.cmu.sphinx.fst.semiring.Semiring;
import edu.cmu.sphinx.fst.semiring.TropicalSemiring;
import edu.cmu.sphinx.fst.utils.Utils;

/**
 * The grapheme-to-phoneme (g2p) decoder
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class G2PConverter {

    // epsilon symbol
    String eps = "<eps>";

    // end sequence symbol
    String se = "</s>";

    // begin sequence symbol
    String sb = "<s>";

    // skip symbol
    String skip = "_";

    // separator symbol
    String tie = "|";

    // set containing sequences to ignore
    HashSet<String> skipSeqs = new HashSet<String>();

    // clusters
    ArrayList<String>[] clusters = null;

    // the g2p model
    ImmutableFst g2pmodel;

    // fst containing the epsilon filter for the compose operation
    Fst epsilonFilter;

    /**
     * Create a decoder by loading the serialized model from a specified URL
     * 
     * @param g2pModelUrl
     *            the URL of the serialized model
     * @throws IOException
     * @throws ClassNotFoundException 
     */
    public G2PConverter(URL g2pModelUrl) throws IOException {
        try {
            g2pmodel = ImmutableFst.loadModel(g2pModelUrl.openStream());
        } catch (ClassNotFoundException e) {
            throw new IOException("Failed to load the model from " + g2pModelUrl, e);
        }
        init();
    }

    /**
     * Create a decoder by loading the serialized model from a specified
     * filename
     * 
     * @param g2pmodel_file
     *            the filename of the serialized model
     */
    public G2PConverter(String g2pmodel_file) {
        g2pmodel = ImmutableFst.loadModel(g2pmodel_file);
        init();
    }

    /**
     * Initialize the decoder
     */
    private void init() {
        skipSeqs.add(eps);
        skipSeqs.add(sb);
        skipSeqs.add(se);
        skipSeqs.add(skip);
        skipSeqs.add("-");
        // keep an augmented copy (for compose)
        Compose.augment(0, g2pmodel, g2pmodel.getSemiring());
        ArcSort.apply(g2pmodel, new ILabelCompare());

        String[] isyms = g2pmodel.getIsyms();

        loadClusters(isyms);

        // get epsilon filter for composition
        epsilonFilter = Compose.getFilter(g2pmodel.getIsyms(),
                g2pmodel.getSemiring());
        ArcSort.apply(epsilonFilter, new ILabelCompare());
    }

    /**
     * Phoneticize a word
     * 
     * @param entry
     *            the word to phoneticize transformed to an ArrayList of Strings
     *            (each element hold a single character)
     * @param nbest
     *            the number of distinct pronunciations to return
     * @return the pronunciation(s) of the input word
     */
    public ArrayList<Path> phoneticize(ArrayList<String> entry, int nbest) {
        Fst efst = entryToFSA(entry);
        Semiring s = efst.getSemiring();
        Compose.augment(1, efst, s);
        ArcSort.apply(efst, new OLabelCompare());
        Fst result = Compose.compose(efst, epsilonFilter, s, true);
        ArcSort.apply(result, new OLabelCompare());
        result = Compose.compose(result, g2pmodel, s, true);
        Project.apply(result, ProjectType.OUTPUT);
        if (nbest == 1) {
            result = NShortestPaths.get(result, 1, false);
        } else {
            // Requesting 10 times more best paths than what was asking
            // as there might be several paths resolving to same pronunciation
            // due to epsilon transitions.
            // I really hate cosmological constants :)
            result = NShortestPaths.get(result, nbest * 10, false);
        }
        // result = NShortestPaths.get(result, nbest, false);
        result = RmEpsilon.get(result);
        ArrayList<Path> paths = findAllPaths(result, nbest, skipSeqs,
                tie);

        return paths;
    }

    /**
     * Phoneticize a word
     * 
     * @param entry
     *            the word to phoneticize
     * @param nbest
     *            the number of distinct pronunciations to return
     * @return the pronunciation(s) of the input word
     */
    public ArrayList<Path> phoneticize(String word, int nbest) {
        ArrayList<String> entry = new ArrayList<String>(word.length());
        for (int i = 0; i < word.length(); i++) {
            String ch = word.substring(i, i + 1);
            if (Utils.getIndex(g2pmodel.getIsyms(), ch) >= 0) {
                entry.add(ch);
            }
        }
        return phoneticize(entry, nbest);
    }

    /**
     * Transforms an input spelling/pronunciation into an equivalent FSA, adding
     * extra arcs as needed to accommodate clusters.
     * 
     * @param entry
     *            the input vector
     * @return the created fst
     */
    private Fst entryToFSA(ArrayList<String> entry) {
        TropicalSemiring ts = new TropicalSemiring();
        Fst efst = new Fst(ts);

        State s = new State(ts.zero());
        efst.addState(s);
        efst.setStart(s);

        // Build the basic FSA
        for (int i = 0; i < entry.size() + 1; i++) {
            s = new State(ts.zero());
            efst.addState(s);
            if (i >= 1) {
                int symIndex = Utils.getIndex(g2pmodel.getIsyms(),
                        entry.get(i - 1));
                efst.getState(i).addArc(new Arc(symIndex, symIndex, 0.f, s));
            } else if (i == 0) {
                int symIndex = Utils.getIndex(g2pmodel.getIsyms(), sb);
                efst.getStart().addArc(new Arc(symIndex, symIndex, 0.f, s));
            }

            if (i == entry.size()) {
                State s1 = new State(ts.zero());
                efst.addState(s1);
                int symIndex = Utils.getIndex(g2pmodel.getIsyms(), se);
                s.addArc(new Arc(symIndex, symIndex, 0.f, s1));
                s1.setFinalWeight(0.f);
            }
        }

        // Add any cluster arcs
        for (int value = 0; value < clusters.length; value++) {
            ArrayList<String> cluster = clusters[value];
            if (cluster != null) {
                int start = 0;
                int k = 0;
                while (k != -1) {
                    k = Utils.search(entry, cluster, start);
                    if (k != -1) {
                        State from = efst.getState(start + k + 1);
                        from.addArc(new Arc(value, value, 0.f, efst
                                .getState(start + k + cluster.size() + 1)));
                        start = start + k + cluster.size();
                    }
                }
            }
        }

        efst.setIsyms(g2pmodel.getIsyms());
        efst.setOsyms(g2pmodel.getIsyms());

        return efst;
    }

    /**
     * Finds nbest paths in an Fst returned by NShortestPaths operation
     * 
     * @param fst
     *            the input fst
     * @param nbest
     *            the number of paths to return
     * @param skipSeqs
     *            the sequences to ignore
     * @param tie
     *            the separator symbol
     * @return the paths
     */
    @SuppressWarnings("unchecked")
    private ArrayList<Path> findAllPaths(Fst fst, int nbest,
            HashSet<String> skipSeqs, String tie) {
        Semiring semiring = fst.getSemiring();

        // ArrayList<Path> finalPaths = new ArrayList<Path>();
        HashMap<String, Path> finalPaths = new HashMap<String, Path>();
        HashMap<State, Path> paths = new HashMap<State, Path>();
        Queue<State> queue = new LinkedList<State>();
        Path p = new Path(fst.getSemiring());
        p.setCost(semiring.one());
        paths.put(fst.getStart(), p);

        queue.add(fst.getStart());

        String[] osyms = fst.getOsyms();
        while (!queue.isEmpty()) {
            State s = queue.remove();
            Path currentPath = paths.get(s);

            if (s.getFinalWeight() != semiring.zero()) {
                String pathString = currentPath.getPath().toString();
                if (finalPaths.containsKey(pathString)) {
                    // path already exist. update its cost
                    Path old = finalPaths.get(pathString);
                    if (old.getCost() > currentPath.getCost()) {
                        finalPaths.put(pathString, currentPath);
                    }
                } else {
                    finalPaths.put(pathString, currentPath);
                }
            }

            int numArcs = s.getNumArcs();
            for (int j = 0; j < numArcs; j++) {
                Arc a = s.getArc(j);
                p = new Path(fst.getSemiring());
                Path cur = paths.get(s);
                p.setCost(cur.getCost());
                p.setPath((ArrayList<String>) cur.getPath().clone());

                String sym = osyms[a.getOlabel()];

                String[] symsArray = sym.split("\\" + tie);

                for (int i = 0; i < symsArray.length; i++) {
                    String phone = symsArray[i];
                    if (!skipSeqs.contains(phone)) {
                        p.getPath().add(phone);
                    }
                }
                p.setCost(semiring.times(p.getCost(), a.getWeight()));
                State nextState = a.getNextState();
                paths.put(nextState, p);
                if (!queue.contains(nextState)) {
                    queue.add(nextState);
                }
            }
        }

        ArrayList<Path> res = new ArrayList<Path>();
        for (Path path : finalPaths.values()) {
            res.add(path);
        }

        Collections.sort(res, new PathComparator());
        int numPaths = res.size();
        for (int i = nbest; i < numPaths; i++) {
            res.remove(res.size() - 1);
        }

        return res;
    }

    /**
     * Initialize clusters
     */
    @SuppressWarnings("unchecked")
    private void loadClusters(String[] syms) {
        clusters = new ArrayList[syms.length];
        for (int i = 0; i < syms.length; i++) {
            clusters[i] = null;
        }
        for (int i = 2; i < syms.length; i++) {
            String sym = syms[i];
            if (sym.contains(tie)) {
                String split[] = sym.split(Pattern.quote(tie));
                ArrayList<String> cluster = new ArrayList<String>(
                        Arrays.asList(split));
                clusters[i] = cluster;
            }
        }
    }
}

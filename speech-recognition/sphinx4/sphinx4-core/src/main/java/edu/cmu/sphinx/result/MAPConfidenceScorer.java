/*
 * Copyright 1999-2004 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.result;

import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.util.props.*;

import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

/**
 * Computes confidences for the highest scoring path in a Result. The highest scoring path refers to the path with the
 * maximum a posteriori (MAP) probability, which is why this class is so named. Note that this MAPConfidenceScorer
 * creates a {@link edu.cmu.sphinx.result.Lattice} from the result first, which means that you should only use this
 * confidence scorer if the result is created from the {@link edu.cmu.sphinx.linguist.lextree.LexTreeLinguist} and the
 * {@link edu.cmu.sphinx.decoder.search.WordPruningBreadthFirstSearchManager}.
 */
public class MAPConfidenceScorer implements ConfidenceScorer, Configurable {

    /** The property that defines the weight multiplier that will be applied to language score already scaled by language weight. */
    @S4Double(defaultValue = 1.0)
    public final static String PROP_LANGUAGE_WEIGHT_ADJUSTMENT = "languageWeightAdjustment";


    /** The property that specifies whether to dump the lattice. */
    @S4Boolean(defaultValue = false)
    public final static String PROP_DUMP_LATTICE = "dumpLattice";


    /** The property that specifies whether to dump the sausage. */
    @S4Boolean(defaultValue = false)
    public final static String PROP_DUMP_SAUSAGE = "dumpSausage";


    private float languageWeightAdjustment;
    private boolean dumpLattice;
    private boolean dumpSausage;

    public MAPConfidenceScorer(float languageWeightAdjustment, boolean dumpLattice, boolean dumpSausage) {
        this.languageWeightAdjustment = languageWeightAdjustment;
        this.dumpLattice = dumpLattice;
        this.dumpSausage = dumpSausage;
    }

    public MAPConfidenceScorer() {
    }

    /*
     * (non-Javadoc)
     *
     * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
     */
    public void newProperties(PropertySheet ps) throws PropertyException {
        languageWeightAdjustment = ps.getFloat(PROP_LANGUAGE_WEIGHT_ADJUSTMENT);
        dumpLattice = ps.getBoolean(PROP_DUMP_LATTICE);
        dumpSausage = ps.getBoolean(PROP_DUMP_SAUSAGE);
    }


    /**
     * Computes confidences for a Result and returns a ConfidenceResult, a compact representation of all the hypothesis
     * contained in the result together with their per-word and per-path confidences.
     *
     * @param result the result to compute confidences for
     * @return a confidence result
     */
    public ConfidenceResult score(Result result) {
        Lattice lattice = new Lattice(result);
        LatticeOptimizer lop = new LatticeOptimizer(lattice);
        lop.optimize();
        lattice.computeNodePosteriors(languageWeightAdjustment);
        SausageMaker sm = new SausageMaker(lattice);
        Sausage s = sm.makeSausage();

        if (dumpLattice) {
            lattice.dumpAISee("mapLattice.gdl", "MAP Lattice");
        }
        if (dumpSausage) {
            //s.dumpAISee("mapSausage.gdl", "MAP Sausage");
        }

        WordResultPath mapPath = new WordResultPath();
        List<Token> wordTokens = getWordTokens(result.getBestToken());

        /* start with the first slot */
        int slot = 0;

        for (Token wordToken : wordTokens) {
            String word = wordToken.getWord().getSpelling();
            WordResult wr = null;
            ConfusionSet cs = null;

            /* track through all the slots to find the word */
            while (slot < s.size() && wr == null) {
                cs = s.getConfusionSet(slot);
                wr = cs.getWordResult(word);
                if (wr == null) {
                    slot++;
                }
            }
            if (wr != null) {
                mapPath.add(wr);
            } else {
                cs.dump("Slot " + slot);
                throw new Error
                        ("Can't find WordResult in ConfidenceResult slot " +
                                slot + " for word " + word);
            }
            slot++;
        }

        return (new MAPConfidenceResult(s, mapPath));
    }


    /**
     * Returns all the word tokens ending at the given token as a List.
     *
     * @param lastToken the last token in the token chain
     * @return a list of word tokens in order of appearance
     */
    private List<Token> getWordTokens(Token lastToken) {
        List<Token> wordTokens = new LinkedList<Token>();
        Token token = lastToken;
        while (token != null) {
            if (token.isWord()) {
                wordTokens.add(0, token);
            }
            token = token.getPredecessor();
        }
        return wordTokens;
    }


    /** The confidence result for the highest scoring path. */
    class MAPConfidenceResult implements ConfidenceResult {

        private final ConfidenceResult sausage;
        private final Path mapPath;


        /**
         * Constructs a MAPConfidenceResult.
         *
         * @param sausage the sausge that this MAPConfidenceResult is based on
         * @param mapPath the maximum posterior probability path
         */
        public MAPConfidenceResult(ConfidenceResult sausage, Path mapPath) {
            this.sausage = sausage;
            this.mapPath = mapPath;
        }


        /**
         * Returns the path with the maximum posterior probability path. This path should be the same as that returned
         * by Result.getBestToken().
         */
        public Path getBestHypothesis() {
            return mapPath;
        }


        /**
         * Get the number of word slots contained in this result
         *
         * @return length of the result
         */
        public int size() {
            return sausage.size();
        }


        /**
         * Iterator through the confusion sets in this result.
         *
         * @return confusion set iterator
         */
        public Iterator<ConfusionSet> iterator() {
            return sausage.iterator();
        }


        /**
         * Get the nth confusion set in this result
         *
         * @param i the index of the confusion set to get
         * @return the requested confusion set
         */
        public ConfusionSet getConfusionSet(int i) {
            return sausage.getConfusionSet(i);
        }
    }
}

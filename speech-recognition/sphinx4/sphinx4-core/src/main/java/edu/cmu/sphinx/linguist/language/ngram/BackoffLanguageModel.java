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
package edu.cmu.sphinx.linguist.language.ngram;

import edu.cmu.sphinx.linguist.WordSequence;

/**
 * Represents the generic interface to an N-Gram language model
 * that uses backoff to estimate unseen probabilities. Backoff
 * depth is important in search space optimization, for example
 * it's used in LexTreeLinguist to collapse states which has
 * only unigram backoff. This ways unlikely sequences are penalized.
 */
public interface BackoffLanguageModel extends LanguageModel {

    public ProbDepth getProbDepth(WordSequence wordSequence);
}

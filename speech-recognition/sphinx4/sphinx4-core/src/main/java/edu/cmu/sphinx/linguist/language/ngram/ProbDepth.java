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

/**
 * Class for returning results from {@link BackoffLanguageModel} 
 */
public class ProbDepth {
	public float probability;
	public int depth;
	
	public ProbDepth(float probability, int depth) {
		super();
		this.probability = probability;
		this.depth = depth;
	}
}

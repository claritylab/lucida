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
package edu.cmu.sphinx.linguist.language.grammar;

import java.util.List;
import java.util.ListIterator;
import java.util.ArrayList;

import edu.cmu.sphinx.linguist.dictionary.Dictionary;

import edu.cmu.sphinx.util.LogMath;

public class AlignerGrammar extends Grammar {

    private final LogMath logMath;

	private boolean modelRepeats = false;
	private boolean modelSkips = false;

	private float wordRepeatProbability = 0.0f;
	private float wordSkipProbability = 0.0f;
	private int wordSkipRange;

	protected GrammarNode finalNode;
	private final List<String> tokens = new ArrayList<String>();

	public AlignerGrammar(final boolean showGrammar, final boolean optimizeGrammar,
			final boolean addSilenceWords, final boolean addFillerWords,
			final Dictionary dictionary) {
		super(showGrammar, optimizeGrammar, addSilenceWords, addFillerWords,
				dictionary);
		logMath = LogMath.getInstance();
	}

	public AlignerGrammar() {
		logMath = LogMath.getInstance();
	}

	/*
	 * Reads Text and converts it into a list of tokens
	 */
	public void setText(String text) {
		String[] words = text.split(" ");
		tokens.clear();
		for (String word : words) {
		    if (!word.isEmpty())
		        tokens.add(word.toLowerCase());
		}
		createGrammar();		
		postProcessGrammar();
	}

	@Override
    protected GrammarNode createGrammar() {

		logger.info("Creating Grammar");
		initialNode = createGrammarNode(Dictionary.SILENCE_SPELLING);
		finalNode = createGrammarNode(Dictionary.SILENCE_SPELLING);
		finalNode.setFinalNode(true);
		final GrammarNode branchNode = createGrammarNode(false);

		final List<GrammarNode> wordGrammarNodes = new ArrayList<GrammarNode>();
		final int end = tokens.size();

		logger.info("Creating Grammar nodes");
		for (final String word : tokens.subList(0, end)) {
			final GrammarNode wordNode = createGrammarNode(word.toLowerCase());
			wordGrammarNodes.add(wordNode);
		}
		logger.info("Done creating grammar node");

		// now connect all the GrammarNodes together
		initialNode.add(branchNode, LogMath.LOG_ONE);

		createBaseGrammar(wordGrammarNodes, branchNode, finalNode);

		if (modelRepeats) {
			addForwardJumps(wordGrammarNodes, branchNode, finalNode);
		}
		if (modelSkips) {
			addBackwardJumps(wordGrammarNodes, branchNode, finalNode);
		}

		logger.info("Done making Grammar");
		// initialNode.dumpDot("./graph.dot");
		return initialNode;
	}


	private void addBackwardJumps(List<GrammarNode> wordGrammarNodes,
			GrammarNode branchNode, GrammarNode finalNode) {
		GrammarNode currNode;
		for (int i = 0; i < wordGrammarNodes.size(); i++) {
			currNode = wordGrammarNodes.get(i);
			for (int j = Math.max(i - wordSkipRange, 0); j < i; j++) {
				GrammarNode jumpToNode = wordGrammarNodes.get(j);
				currNode.add(jumpToNode,
						logMath.linearToLog(wordRepeatProbability));
			}
		}
	}

	private void addForwardJumps(List<GrammarNode> wordGrammarNodes,
			GrammarNode branchNode, GrammarNode finalNode) {
		GrammarNode currNode = branchNode;
		for (int i = -1; i < wordGrammarNodes.size(); i++) {
			if (i > -1) {
				currNode = wordGrammarNodes.get(i);
			}
			for (int j = i + 2; j < Math.min(wordGrammarNodes.size(), i
					+ wordSkipRange); j++) {
				GrammarNode jumpNode = wordGrammarNodes.get(j);
				currNode.add(jumpNode,
						logMath.linearToLog(wordSkipProbability));
			}
		}
		for (int i = wordGrammarNodes.size() - wordSkipRange; i < wordGrammarNodes
				.size() - 1; i++) {
			int j = wordGrammarNodes.size();
			currNode = wordGrammarNodes.get(i);
			currNode.add(
					finalNode,
					logMath.linearToLog(wordSkipProbability
							* Math.pow(Math.E, j - i)));
		}

	}

	private void createBaseGrammar(List<GrammarNode> wordGrammarNodes,
			GrammarNode branchNode, GrammarNode finalNode) {
		GrammarNode currNode = branchNode;
		ListIterator<GrammarNode> iter = wordGrammarNodes.listIterator();
		while (iter.hasNext()) {
			GrammarNode nextNode = iter.next();
			currNode.add(nextNode, LogMath.LOG_ONE);
			currNode = nextNode;
		}
		currNode.add(finalNode, LogMath.LOG_ONE);
	}

}

/**
 * Copyright 1998-2009 Sun Microsystems, Inc.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.jsgf.rule;

import java.util.List;

public class JSGFRuleAlternatives extends JSGFRule {
	protected List<JSGFRule> rules;
	protected List<Float> weights;

	public JSGFRuleAlternatives() {
	}
	
	public JSGFRuleAlternatives(List<JSGFRule> rules) {
		setRules(rules);
		weights = null;
	}

	public JSGFRuleAlternatives(List<JSGFRule> rules, List<Float> weights)
			throws IllegalArgumentException {
		assert (rules.size() == weights.size());
		setRules(rules);
		setWeights(weights);
	}

	public void append(JSGFRule rule) {
		assert rule != null;
		rules.add(rule);
		if (weights != null)
		    weights.add(1.0f);
	}

	public List<JSGFRule> getRules() {
		return rules;
	}

	public List<Float> getWeights() {
		return weights;
	}

	public void setRules(List<JSGFRule> rules) {
		if ((weights != null) && (rules.size() != weights.size())) {
			weights = null;
		}
		this.rules = rules;
	}

	public void setWeights(List<Float> newWeights)
			throws IllegalArgumentException {
		if ((newWeights == null) || (newWeights.size() == 0)) {
			weights = null;
			return;
		}

		if (newWeights.size() != rules.size()) {
			throw new IllegalArgumentException(
					"weights/rules array length mismatch");
		}
		float f = 0.0F;

		for (Float w : newWeights) {
			if (Float.isNaN(w))
				throw new IllegalArgumentException("illegal weight value: NaN");
			if (Float.isInfinite(w))
				throw new IllegalArgumentException(
						"illegal weight value: infinite");
			if (w < 0.0D) {
				throw new IllegalArgumentException(
						"illegal weight value: negative");
			}
			f += w;
		}

		if (f <= 0.0D) {
			throw new IllegalArgumentException(
					"illegal weight values: all zero");
		}
		weights = newWeights;
	}

	@Override
    public String toString() {
		if (rules == null || rules.size() == 0) {
			return "<VOID>";
		}
		StringBuilder sb = new StringBuilder();

		for (int i = 0; i < rules.size(); ++i) {
			if (i > 0)
				sb.append(" | ");

			if (weights != null)
				sb.append("/" + weights.get(i) + "/ ");

			JSGFRule r = rules.get(i);
			if (rules.get(i) instanceof JSGFRuleAlternatives)
				sb.append("( ").append(r).append(" )");
			else {
				sb.append(r);
			}
		}
		return sb.toString();
	}
}

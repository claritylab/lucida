/**
 * Copyright 1998-2009 Sun Microsystems, Inc.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.jsgf.rule;

import java.util.List;

public class JSGFRuleSequence extends JSGFRule {
	protected List<JSGFRule> rules;

	public JSGFRuleSequence() {
		setRules(null);
	}

	public JSGFRuleSequence(List<JSGFRule> rules) {
		setRules(rules);
	}

	public void append(JSGFRule rule) {
		if (rules == null) {
			throw new NullPointerException("null rule to append");
		}
		rules.add(rule);
	}

	public List<JSGFRule> getRules() {
		return rules;
	}

	public void setRules(List<JSGFRule> rules) {
		this.rules = rules;
	}

	@Override
    public String toString() {
		if (rules.size() == 0) {
			return "<NULL>";
		}
		StringBuilder sb = new StringBuilder();

		for (int i = 0; i < rules.size(); ++i) {
			if (i > 0)
				sb.append(' ');

			JSGFRule r = rules.get(i);
			if ((r instanceof JSGFRuleAlternatives) || (r instanceof JSGFRuleSequence))
				sb.append("( ").append(r).append(" )");
			else {
				sb.append(r);
			}
		}
		return sb.toString();
	}
}

/**
 * Copyright 1998-2009 Sun Microsystems, Inc.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.jsgf.rule;

public class JSGFRuleTag extends JSGFRule {
	protected JSGFRule rule;
	protected String tag;

	public JSGFRuleTag() {
		setRule(null);
		setTag(null);
	}

	public JSGFRuleTag(JSGFRule rule, String tag) {
		setRule(rule);
		setTag(tag);
	}

	private String escapeTag(String tag) {
		StringBuilder stringBuilder = new StringBuilder(tag);

		if ((tag.indexOf('}') >= 0) || (tag.indexOf('\\') >= 0)
				|| (tag.indexOf('{') >= 0)) {
			for (int i = stringBuilder.length() - 1; i >= 0; --i) {
				int j = stringBuilder.charAt(i);
				if ((j == '\\') || (j == '}') || (j == '{')) {
					stringBuilder.insert(i, '\\');
				}
			}
		}
		return stringBuilder.toString();
	}

	public JSGFRule getRule() {
		return rule;
	}

	public String getTag() {
		return tag;
	}

	public void setRule(JSGFRule rule) {
		this.rule = rule;
	}

	public void setTag(String tag) {
		if (tag == null)
			this.tag = "";
		else
			this.tag = tag;
	}

	@Override
    public String toString() {
		String str = " {" + escapeTag(tag) + "}";

		if ((rule instanceof JSGFRuleToken) || (rule instanceof JSGFRuleName)) {
			return rule.toString() + str;
		}
		return "(" + rule.toString() + ")" + str;
	}
}

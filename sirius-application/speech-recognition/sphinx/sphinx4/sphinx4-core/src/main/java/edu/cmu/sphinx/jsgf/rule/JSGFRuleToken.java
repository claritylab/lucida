/**
 * Copyright 1998-2009 Sun Microsystems, Inc.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.jsgf.rule;

public class JSGFRuleToken extends JSGFRule {
	protected String text;

	public JSGFRuleToken() {
		setText(null);
	}

	public JSGFRuleToken(String text) {
		setText(text);
	}

	private boolean containsWhiteSpace(String text) {
		for (int i = 0; i < text.length(); ++i) {
			if (Character.isWhitespace(text.charAt(i)))
				return true;
		}
		return false;
	}

	public String getText() {
		return text;
	}

	public void setText(String text) {
		this.text = text;
	}

	@Override
    public String toString() {
		if ((containsWhiteSpace(text)) || (text.indexOf('\\') >= 0)
				|| (text.indexOf('"') >= 0)) {
			StringBuilder stringBuilder = new StringBuilder(text);

			for (int j = stringBuilder.length() - 1; j >= 0; --j) {
				int i;
				i = stringBuilder.charAt(j);
				if ((i == '"') || (i == '\\')) {
					stringBuilder.insert(j, '\\');
				}
			}
			stringBuilder.insert(0, '"');
			stringBuilder.append('"');

			return stringBuilder.toString();
		}
		return text;
	}
}

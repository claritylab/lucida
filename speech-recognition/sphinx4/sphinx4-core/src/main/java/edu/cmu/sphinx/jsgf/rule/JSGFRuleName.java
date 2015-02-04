/**
 * Copyright 1998-2009 Sun Microsystems, Inc.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.jsgf.rule;

import java.util.StringTokenizer;

public class JSGFRuleName extends JSGFRule {
	
	protected String fullRuleName;
	
	protected String packageName;
	protected String simpleGrammarName;
	protected String simpleRuleName;

	public String resolvedRuleName;

	public static JSGFRuleName NULL = new JSGFRuleName("NULL");

	public static JSGFRuleName VOID = new JSGFRuleName("VOID");

	public JSGFRuleName() {
		this("NULL");
	}

	public JSGFRuleName(String name) {
		// System.out.println ("Building rule name " + name);
		setRuleName(name);
	}

	public String getFullGrammarName() {
		// System.out.println ("Getting full grammar name from " + fullRuleName);
		if (packageName != null) {
			return packageName + "." + simpleGrammarName;
		}
		// System.out.println ("Result is " + simpleGrammarName);
		return simpleGrammarName;
	}

	public String getPackageName() {
		return packageName;
	}

	public String getRuleName() {
		return fullRuleName;
	}

	public String getSimpleGrammarName() {
		return simpleGrammarName;
	}

	public String getSimpleRuleName() {
		return simpleRuleName;
	}

	public boolean isLegalRuleName() {
		return isLegalRuleName(fullRuleName);
	}

	public static boolean isLegalRuleName(String name) {
		if (name == null) {
			return false;
		}
		name = stripRuleName(name);

		if (name.endsWith(".*")) {
			name = name.substring(0, name.length() - 2);
		}

		if (name.length() == 0) {
			return false;
		}

		if ((name.startsWith(".")) || (name.endsWith("."))
				|| (name.indexOf("..") >= 0)) {
			return false;
		}

		StringTokenizer localStringTokenizer = new StringTokenizer(name, ".");

		while (localStringTokenizer.hasMoreTokens()) {
			String str = localStringTokenizer.nextToken();

			int i = str.length();

			if (i == 0)
				return false;

			for (int j = 0; j < i; ++j) {
				if (!(isRuleNamePart(str.charAt(j))))
					return false;
			}
		}
		return true;
	}

	public static boolean isRuleNamePart(char c) {
		if (Character.isJavaIdentifierPart(c)) {
			return true;
		}
		return ((c == '!') || (c == '#') || (c == '%')
				|| (c == '&') || (c == '(')
				|| (c == ')') || (c == '+')
				|| (c == ',') || (c == '-')
				|| (c == '/') || (c == ':')
				|| (c == ';') || (c == '=')
				|| (c == '@') || (c == '[')
				|| (c == '\\') || (c == ']')
				|| (c == '^') || (c == '|') || (c == '~'));
	}

	public void setRuleName(String ruleName) {
		String strippedName = stripRuleName(ruleName);
		fullRuleName = strippedName;

		int j = strippedName.lastIndexOf('.');

		if (j < 0) {
			packageName = null;
			simpleGrammarName = null;
			simpleRuleName = strippedName;
		} else {
			int i = strippedName.lastIndexOf('.', j - 1);

			if (i < 0) {
				packageName = null;
				simpleGrammarName = strippedName.substring(0, j);
				simpleRuleName = strippedName.substring(j + 1);
			} else {
				packageName = strippedName.substring(0, i);
				simpleGrammarName = strippedName.substring(i + 1, j);
				simpleRuleName = strippedName.substring(j + 1);
			}
		}
	}

	public static String stripRuleName(String name) {
		if ((name.startsWith("<")) && (name.endsWith(">"))) {
			return name.substring(1, name.length() - 1);
		}
		return name;
	}

	@Override
    public String toString() {
		return "<" + fullRuleName + ">";
	}
}

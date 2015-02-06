/**
 * Copyright 1998-2009 Sun Microsystems, Inc.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.jsgf.rule;

public class JSGFRule {
	
	public String ruleName;
	public JSGFRule parent;

	@Override
    public String toString() {
		return ruleName;
	}
}

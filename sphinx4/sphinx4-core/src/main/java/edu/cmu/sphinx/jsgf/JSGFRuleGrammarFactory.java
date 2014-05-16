/**
 * Copyright 1998-2009 Sun Microsystems, Inc.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.jsgf;

public class JSGFRuleGrammarFactory {
	
	JSGFRuleGrammarManager manager;
	public JSGFRuleGrammarFactory (JSGFRuleGrammarManager manager) {
		this.manager = manager;
	}
	
	public JSGFRuleGrammar newGrammar(String name) {
		
		assert manager != null;
		JSGFRuleGrammar grammar = new JSGFRuleGrammar(name, manager);
		manager.storeGrammar(grammar);
		return grammar;
	}
}

/**
 * Copyright 1998-2003 Sun Microsystems, Inc.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.jsgf;

@SuppressWarnings("serial")
public class JSGFGrammarParseException extends Exception
{
	public int lineNumber;
	public int charNumber;
	public String message;
	public String details;

	public JSGFGrammarParseException (int lineNumber, int charNumber, String message, String details) {
		this.lineNumber = lineNumber;
		this.charNumber = charNumber;
		this.message = message;
		this.details = details;
	}
	public JSGFGrammarParseException (String message) {
     	this.message = message;
    }
}

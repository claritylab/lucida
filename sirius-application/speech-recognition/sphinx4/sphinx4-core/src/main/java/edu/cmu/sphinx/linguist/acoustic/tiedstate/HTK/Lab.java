/*
 * Copyright 2007 LORIA, France.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.linguist.acoustic.tiedstate.HTK;

/**
 * Represents a label, i.e. a model name + a state number
 * 
 * @author Christophe Cerisara
 * 
 */
public class Lab {
	private String nameHMM;
	private int numState = -1;
	private int start = -1, end = -1;

	public Lab() {
	}

	public Lab(String s) {
		setName(s);
	}

	public Lab(String s, int n) {
		setName(s);
		setStateIdx(n);
	}

	// copy-constructor
	public Lab(Lab ref) {
		setDeb(ref.getStart());
		setFin(ref.getEnd());
		setName(ref.getName());
		setStateIdx(ref.getState());
	}

	public String getName() {
		return nameHMM;
	}

	public int getState() {
		return numState;
	}

	public int getStart() {
		return start;
	}

	public int getEnd() {
		return end;
	}

	public void setName(String s) {
		nameHMM = s;
	}

	public void setStateIdx(int i) {
		numState = i;
	}

	public void setDeb(int i) {
		start = i;
	}

	public void setFin(int i) {
		end = i;
	}

	public boolean isEqual(Lab l) {
		if (l.getState() != -1 && getState() != -1) {
			return l.getName().equals(getName()) && l.getState() == getState();
		} else {
			return l.getName().equals(getName());
		}
	}

    @Override
    public String toString() {
		String r = "";
		if (start >= 0 && end >= start)
			r += start + " " + end + ' ';
		r += nameHMM;
		if (numState >= 0)
			r += "[" + numState + ']';
		return r;
	}
}

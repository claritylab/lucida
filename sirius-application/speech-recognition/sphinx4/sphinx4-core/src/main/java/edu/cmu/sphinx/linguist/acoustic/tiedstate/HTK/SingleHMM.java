/*
 * Copyright 2007 LORIA, France.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.linguist.acoustic.tiedstate.HTK;


public class SingleHMM {
	private final HMMState[] states;
	private String name;
	public float[][] trans;
	// index of the transition macro
	private int transidx = -1; // in case the transitions is a macro
	// index of the transition in the transition pool
	public int trIdx = -1;
	private int nbEmittingStates;
	public HMMSet hmmset;

	public SingleHMM(int nbStates) {
		name = "";
		states = new HMMState[nbStates];
		nbEmittingStates = 0;
	}

	public void setName(String s) {
		name = s;
	}

	public String getName() {
		return name;
	}

	public String getBaseName() {
		int m = name.indexOf('-');
		if (m < 0)
			m = -1;
		String b = name.substring(m + 1);
		m = b.indexOf('+');
		if (m < 0)
			m = b.length();
		return b.substring(0, m);
	}

	public String getLeft() {
		int m = name.indexOf('-');
		if (m < 0)
			return "-";
		return name.substring(0, m);
	}

	public String getRight() {
		int m = name.indexOf('+');
		if (m < 0)
			return "-";
		return name.substring(m + 1);
	}

	public void setState(int idx, HMMState st) {
		if (states[idx] == null && st != null)
			nbEmittingStates++;
		states[idx] = st;
	}

	public boolean isEmitting(int idx) {
		return (states[idx] != null);
	}

	public void setTrans(float[][] tr) {
		trans = tr;
	}

	public void setTrans(int i) {
		trans = null;
		transidx = i;
	}

	public int getTransIdx() {
		return transidx;
	}

	public float getTrans(int i, int j) {
		if (trans == null) {
			trans = hmmset.transitions.get(transidx);
		}
		return trans[i][j];
	}

	/**
	 * may return null if the state is non-emitting
	 * 

	 * @return the HMM state
	 */
	public HMMState getState(int idx) {
		return states[idx];
	}

	public int getNstates() {
		return states.length;
	}

	public int getNbEmittingStates() {
		return nbEmittingStates;

	}
}

/*
 * Copyright 2007 LORIA, France.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.linguist.acoustic.tiedstate.HTK;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.*;

/**
 * 
 * @author Christophe Cerisara
 */

public class HMMSet {
	private GMMDiag g;
	private int nGaussians;
	float[][] trans;
	/**
	 * contains HMMState instances
	 */
	public final List<HMMState> states;
	public final List<float[][]> transitions = new ArrayList<float[][]>();
	public final Map<String, Integer> transNames = new HashMap<String, Integer>();

	public Iterator<SingleHMM> get1phIt() {
		Iterator<SingleHMM> it = new Iterator<SingleHMM>() {
			int cur;

            public void remove() {
			}

            public SingleHMM next() {
				for (;;) {
					if (cur >= hmms.size())
						return null;
					SingleHMM hmm = hmms.get(cur++);
					if (hmm.getName().indexOf('-') >= 0
							|| hmm.getName().indexOf('+') >= 0)
						continue;
					return hmm;
				}
			}

            public boolean hasNext() {
				return false;
			}
		};
		return it;
	}

	public Iterator<SingleHMM> get3phIt() {
		Iterator<SingleHMM> it = new Iterator<SingleHMM>() {
			int cur;

            public void remove() {
			}

            public SingleHMM next() {
				for (;;) {
					if (cur >= hmms.size())
						return null;
					SingleHMM hmm = hmms.get(cur++);
					if (!(hmm.getName().indexOf('-') >= 0 || hmm.getName()
							.indexOf('+') >= 0))
						continue;
					return hmm;
				}
			}

            public boolean hasNext() {
				return false;
			}
		};
		return it;
	}

	public int getStateIdx(HMMState st) {
		return st.gmmidx;
	}

	public int getHMMidx(SingleHMM hmm) {
		for (int i = 0; i < hmms.size(); i++) {
			SingleHMM h = hmms.get(i);
			if (h == hmm)
				return i;
		}
		return -1;
	}

	public int getNstates() {
		return gmms.size();
	}

	public String[] getHMMnames() {
		String[] rep = new String[hmms.size()];
		for (int i = 0; i < rep.length; i++) {
			SingleHMM h = hmms.get(i);
			rep[i] = h.getName();
		}
		return rep;
	}

	/**
	 * contains GMMDiag instances
	 */
	public final List<GMMDiag> gmms;
	/**
	 * contains HMM instances
	 */
	public final List<SingleHMM> hmms;

	public int getNhmms() {
		return hmms.size();
	}

	public int getNhmmsMono() {
		int n = 0;
		for (SingleHMM hmm : hmms) {
			if (!(hmm.getName().indexOf('-') >= 0 || hmm.getName().indexOf('+') >= 0))
				n++;
		}
		return n;
	}

	public int getNhmmsTri() {
		int n = 0;
		for (SingleHMM hmm : hmms) {
			if (hmm.getName().indexOf('-') >= 0
					|| hmm.getName().indexOf('+') >= 0)
				n++;
		}
		return n;
	}

	public int getHMMIndex(SingleHMM h) {
		return hmms.indexOf(h);
	}

	/**
	 * 

	 *            index of the HMM (begins at 0)

	 *            index of the state WITHIN the HMM ! (begins at 1, as in MMF)
	 * @return index of the state in the vector of all the states of the HMMSet
	 *         !
	 */
	public int getStateIdx(int hmmidx, int stateidx) {
		// TODO: store a table not to recalculate every time
		SingleHMM hmm;
		int nEmittingStates = 0;
		for (int i = 0; i < hmmidx; i++) {
			hmm = hmms.get(i);
			nEmittingStates += hmm.getNbEmittingStates();
		}
		hmm = hmms.get(hmmidx);
		for (int i = 1; i < stateidx; i++) {
			if (hmm.isEmitting(i))
				nEmittingStates++;
		}
		if (hmm.isEmitting(stateidx))
			return nEmittingStates; // Don't add 1 since states are counted from
									// 0
		else
			return -1;
	}

	public SingleHMM getHMM(int idx) {
		return hmms.get(idx);
	}

	public SingleHMM getHMM(String nom) {
		SingleHMM h = null;
		for (SingleHMM hmm : hmms) {
			h = hmm;
			if (h.getName().equals(nom))
				break;
		}
		return h;
	}

	public HMMSet() {
		states = new ArrayList<HMMState>();
		hmms = new ArrayList<SingleHMM>();
		gmms = new ArrayList<GMMDiag>();
	}

	public void loadHTK(String nomFich) {
		try {
			BufferedReader f = new BufferedReader(new FileReader(nomFich));
			String s;
			for (;;) {
				s = f.readLine();
				if (s == null)
					break;
				if (s.startsWith("~s")) {
					String nomEtat = s.substring(s.indexOf('"') + 1, s
							.lastIndexOf('"'));
					loadState(f, nomEtat, null);
				} else if (s.startsWith("~v")) {
					// variance floor: bypass
				} else if (s.startsWith("~t")) {
					String nomTrans = s.substring(s.indexOf('"') + 1, s
							.lastIndexOf('"'));
					loadTrans(f, nomTrans, null);
				} else if (s.startsWith("~h")) {
					String nomHMM = s.substring(s.indexOf('"') + 1, s
							.lastIndexOf('"'));
					if (nomHMM.toUpperCase().equals(nomHMM)) {
						System.out
								.println("WARNING: HMM is in lowercase, converting to upper");
					}
					hmms.add(loadHMM(f, nomHMM.toUpperCase(), gmms));
				}
			}
			f.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	private String[][] tiedHMMs;

	public void loadTiedList(String nomFich) {
		try {
			BufferedReader f = new BufferedReader(new FileReader(nomFich));
			String s;
			String[] ss;
			int ntiedstates = 0;
			for (;;) {
				s = f.readLine();
				if (s == null)
					break;
				ss = s.split(" ");
				if (ss.length >= 2) {
					// We have a tiedstate
					ntiedstates++;
				}
			}
			tiedHMMs = new String[ntiedstates][2];
			f.close();
			f = new BufferedReader(new FileReader(nomFich));
			for (int i = 0;;) {
				s = f.readLine();
				if (s == null)
					break;
				ss = s.split(" ");
				if (ss.length >= 2) {
					// We have a tiedstate
					tiedHMMs[i][0] = ss[0];
					tiedHMMs[i++][1] = ss[1];
				}
			}
			f.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * WARNING To be compliant with sphinx3 models, we remove the first
	 * non-emitting state !
	 * 



	 * @throws IOException

	 */
	private SingleHMM loadHMM(BufferedReader f, String n,
			List<GMMDiag> autresEtats) throws IOException {
		GMMDiag e = null;
		int curstate;
		String name = n;
		String s = "";

		while (!s.startsWith("<NUMSTATES>")) {
			s = f.readLine();
		}
		int nstates = Integer.parseInt(s.substring(s.indexOf(' ') + 1));
		// Compliance with sphinx3
		nstates--;
		SingleHMM theHMM = new SingleHMM(nstates);
		theHMM.setName(n);
		theHMM.hmmset = this;
		while (!s.startsWith("<STATE>"))
			s = f.readLine();
		while (s.startsWith("<STATE>")) {
			curstate = Integer.parseInt(s.substring(s.indexOf(' ') + 1));
			// Compliance with sphinx3
			curstate--;
			s = f.readLine();
			int gmmidx = -1;
			if (s.startsWith("~s")) {
				String nomEtat = s.substring(s.indexOf('"') + 1, s
						.lastIndexOf('"'));
				int i;
				for (i = 0; i < autresEtats.size(); i++) {
					e = autresEtats.get(i);
					if (e.nom.equals(nomEtat))
						break;
				}
				gmmidx = i;
				if (i == autresEtats.size()) {
					System.err.println("Error creatiing HMM : state " + name + " not found");
					System.exit(1);
				}
			} else {
				loadState(f, "", s);
				gmmidx = gmms.size() - 1;
				e = gmms.get(gmms.size() - 1);
			}
			HMMState st = new HMMState(e, new Lab(name, curstate));
			st.gmmidx = gmmidx;
			states.add(st);
			theHMM.setState(curstate - 1, st); // -1 because in HTK HMMs are counted from 1
			s = f.readLine();
			// t eliminates the gconst because it is then recalculated!
			if (s.startsWith("<GCONST>"))
				s = f.readLine();
		}
		if (s.startsWith("~t")) {
			// simple application of the
			String nomTrans = s.substring(s.indexOf('"') + 1, s
					.lastIndexOf('"'));
			int tridx = getTrans(nomTrans);
			theHMM.setTrans(tridx);
		} else {
			// The transitions are explicit
			if (!s.startsWith("<TRANSP>")) {
				System.err.println("Error reading model: missing transitions." + s);
				System.exit(1);
			}
			loadTrans(f, null, s);
			theHMM.setTrans(trans);
		}
		s = f.readLine();
		if (!s.startsWith("<ENDHMM>")) {
			System.err.println("Error reading model: missing ENDHMM." + s);
			System.exit(1);
		}
		return theHMM;
	}

	private int loadTrans(BufferedReader f, String nomEtat, String prem)
			throws IOException {
		String s;
		int nstates = 0;
		if (prem != null)
			s = prem;
		else
			s = f.readLine().trim();
		if (s.startsWith("<TRANSP>")) {
			nstates = Integer.parseInt(s.substring(s.indexOf(' ') + 1));
			// Compliance with sphinx3
			nstates--;
		} else {
			System.err.println("ERROR no TRANSP !");
			System.exit(1);
		}
		String[] ss;
		trans = new float[nstates][nstates];
		// Compliance with sphinx3
		f.readLine();
		for (int i = 0; i < nstates; i++) {
			s = f.readLine().trim();
			ss = s.split(" ");
			for (int j = 0; j < nstates; j++) {
				// Compliance with sphinx3
				trans[i][j] = Float.parseFloat(ss[j + 1]);
			}
		}
		if (nomEtat != null) {
			int tridx = transitions.size();
			transNames.put(nomEtat, tridx);
			transitions.add(trans);
			return tridx;
		} else {
			return -1;
			// Application can recover the transitions in the pool
		}
	}

	private int getTrans(String trnom) {
		int tridx = transNames.get(trnom);
		return tridx;
	}

	private void loadState(BufferedReader f, String nomEtat, String prem)
			throws IOException {
		nGaussians = 1;
		String s;
		if (prem != null)
			s = prem;
		else
			s = f.readLine().trim();
		if (s.startsWith("<NUMMIXES>")) {
			nGaussians = Integer.parseInt(s.substring(s.indexOf(' ') + 1));
			s = f.readLine().trim();
		}
		g = null;
		if (!s.startsWith("<MIXTURE>")) {
			// This model has single mixture
			if (nGaussians != 1) {
				System.err.println("Error loading model: number of mixtures is " + nGaussians
						+ " while state " + s + " has 1 mixture.");
				System.exit(1);
			}
			loadHTKGauss(f, 0, s);
			g.setWeight(0, 1f);
		} else {
			String[] ss;
			for (int i = 0; i < nGaussians; i++) {
				if (i > 0)
					s = f.readLine().trim();
				// Don't load GCONST
				if (s.startsWith("<GCONST>"))
					s = f.readLine().trim();
				ss = s.split(" ");
				if (Integer.parseInt(ss[1]) != i + 1) {
					System.err.println("Error reading model: mixture conflict "
							+ i + ' ' + s);
					System.exit(1);
				}
				loadHTKGauss(f, i, null);
				g.setWeight(i, Float.parseFloat(ss[2]));
			}
		}
		g.precomputeDistance();
		g.setNom(nomEtat);
		gmms.add(g);
	}

	/**
	 * Read until the last line of the file but it may leave one last line
	 * so it can loose GCONST.
     * @throws java.io.IOException
     */
	private void loadHTKGauss(BufferedReader f, int n, String prem)
			throws IOException {
		String s;
		String[] ss;
		if (prem != null) {
			// First line is taken into account
			s = prem;
		} else
			s = f.readLine().trim();
		if (s.startsWith("<GCONST>"))
			s = f.readLine().trim();
		if (s.startsWith("<RCLASS>"))
			s = f.readLine().trim();
		if (!s.startsWith("<MEAN>")) {
			System.err.println("Error loading model: can't find <MEAN> ! " + s);
			System.exit(1);
		}
		int ncoefs = Integer.parseInt(s.substring(s.indexOf(' ') + 1));
		if (g == null)
			g = new GMMDiag(nGaussians, ncoefs);
		s = f.readLine().trim();
		ss = s.split(" ");
		if (ss.length != ncoefs) {
			System.err.println("Error loading model: incorrect number of coefficients "
					+ ncoefs + ' ' + s + ' ' + ss[0] + ' ' + ss[39]);
			System.exit(1);
		}
		for (int i = 0; i < ncoefs; i++) {
			g.setMean(n, i, Float.parseFloat(ss[i]));
		}
		s = f.readLine().trim();
		if (!s.startsWith("<VARIANCE>")) {
			System.err.println("Error loading model: missing <VARIANCE> ! " + s);
			System.exit(1);
		}
		s = f.readLine().trim();
		ss = s.split(" ");
		if (ss.length != ncoefs) {
			System.err.println("Error loading model: incorrect number of coefficients "
					+ ncoefs + ' ' + s);
			System.exit(1);
		}
		for (int i = 0; i < ncoefs; i++) {
			g.setVar(n, i, Float.parseFloat(ss[i]));
		}
	}

    public GMMDiag findState(Lab l) {
        while (true) {
            HMMState s = null;
            int i;
            for (i = 0; i < states.size(); i++) {
                s = states.get(i);
                if (s.getLab().isEqual(l))
                    break;
            }
            if (i < states.size()) {
                return s.gmm;
            } else {
                if (tiedHMMs != null) {
                    // May be that state appears in the tied states
                    for (i = 0; i < tiedHMMs.length; i++) {
                        if (tiedHMMs[i][0].equals(l.getName())) {
                            break;
                        }
                    }
                    if (i < tiedHMMs.length) {
                        l = new Lab(tiedHMMs[i][1], l.getState());
                        continue;
                    }
                }
                System.err.println("WARNING: state is not found in hmmset " + l);
                return null;
            }
        }
    }
}

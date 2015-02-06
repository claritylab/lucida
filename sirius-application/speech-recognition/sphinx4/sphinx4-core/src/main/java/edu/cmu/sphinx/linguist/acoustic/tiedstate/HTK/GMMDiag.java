/*
 * Copyright 2007 LORIA, France.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */
package edu.cmu.sphinx.linguist.acoustic.tiedstate.HTK;

import edu.cmu.sphinx.util.LogMath;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.StringTokenizer;

/**
 * This a producer for observations, it outputs the log likelihoods for
 * guassians
 * 
 * @author Christophe Cerisara
 * 
 */

public class GMMDiag {
	public int nT;
	public String nom;
	public LogMath logMath;

	private int ncoefs;
	private int ngauss;
	protected float[] weights;
	protected float[][] means;
	protected float[][] covar;
	private float[] logPreComputedGaussianFactor;
	protected float[] loglikes;

	public GMMDiag() {
	}

	public GMMDiag(int ng, int nc) {
		ngauss = ng;
		ncoefs = nc;
		allocate();
	}

	public int getNgauss() {
		return ngauss;
	}

	public float getWeight(int i) {
		return (float) logMath.logToLinear(weights[i]);
	}

	public float getVar(int i, int j) {
		return -1f / (2f * covar[i][j]);
	}

	public void setWeight(int i, float w) {
		if (weights == null)
			weights = new float[ngauss];
		weights[i] = logMath.linearToLog(w);
	}

	public void setVar(int i, int j, float v) {
		if (v <= 0)
			// This is not a error, because you can use the GMM just to store
			// values and retrieve them later. 
			// TODO: good constant is not very clean, because we must still have variance > 0
			System.err.println("WARNING: setVar " + v);
		covar[i][j] = -1f / (2f * v);
	}

	public void setMean(int i, int j, float v) {
		means[i][j] = v;
	}

	public float getMean(int i, int j) {
		return means[i][j];
	}

	/**
	 * Saves in proprietary format
	 * 

	 */
	public void save(String name) {
		try {
			PrintWriter fout = new PrintWriter(new FileWriter(name));
			fout.println(ngauss + " " + ncoefs);
			for (int i = 0; i < ngauss; i++) {
				fout.println("gauss " + i + ' ' + getWeight(i));
				for (int j = 0; j < ncoefs; j++)
					fout.print(means[i][j] + " ");
				fout.println();
				for (int j = 0; j < ncoefs; j++)
					fout.print(getVar(i, j) + " ");
				fout.println();
			}
			fout.println(nT);
			fout.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	/**
	 * Load from text proprietary format
	 * 

	 */
	public void load(String name) {
		try {
			BufferedReader fin = new BufferedReader(new FileReader(name));
			String s = fin.readLine();
			String[] ss = s.split(" ");
			ngauss = Integer.parseInt(ss[0]);
			ncoefs = Integer.parseInt(ss[1]);
			allocate();
			for (int i = 0; i < ngauss; i++) {
				s = fin.readLine();
				ss = s.split(" ");
				if (!ss[0].equals("gauss") || Integer.parseInt(ss[1]) != i) {
					System.err.println("Error loading GMM " + s + ' ' + i);
					System.exit(1);
				}
				setWeight(i, Float.parseFloat(ss[2]));
				// means
				s = fin.readLine();
				ss = s.split(" ");
				for (int j = 0; j < ncoefs; j++) {
					setMean(i, j, Float.parseFloat(ss[j]));
				}
				// covariances
				s = fin.readLine();
				ss = s.split(" ");
				for (int j = 0; j < ncoefs; j++) {
					setVar(i, j, Float.parseFloat(ss[j]));
				}
			}
			s = fin.readLine();
			if (s != null) {
				// can be added to store the amount of data on which the GMM has been
				// learned
				nT = Integer.parseInt(s);
			}
			fin.close();
			precomputeDistance();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void saveHTK(String nomFich, String nomHMM) {
		saveHTK(nomFich, nomHMM, "<USER>");
	}

	public PrintWriter saveHTKheader(String nomFich, String parmKind) {
		try {
			PrintWriter fout = new PrintWriter(new FileWriter(nomFich));
			fout.println("~o");
			fout.println("<HMMSETID> tree");
			fout.println("<STREAMINFO> 1 " + getNcoefs());
			fout.println("<VECSIZE> " + getNcoefs() + "<NULLD>" + parmKind
					+ "<DIAGC>");
			fout.println("~r \"rtree_1\"");
			fout.println("<REGTREE> 1");
			fout.println("<TNODE> 1 " + getNgauss());
			return fout;
		} catch (IOException e) {
			e.printStackTrace();
			return null;
		}
	}

	public void saveHTKState(PrintWriter fout) {
		fout.println("<NUMMIXES> " + getNgauss());
		for (int i = 1; i <= getNgauss(); i++) {
			fout.println("<MIXTURE> " + i + ' ' + getWeight(i - 1));
			fout.println("<RCLASS> 1");
			fout.println("<MEAN> " + getNcoefs());
			for (int j = 0; j < getNcoefs(); j++) {
				fout.print(getMean(i - 1, j) + " ");
			}
			fout.println();
			fout.println("<VARIANCE> " + getNcoefs());
			for (int j = 0; j < getNcoefs(); j++) {
				fout.print(getVar(i - 1, j) + " ");
			}
			fout.println();
		}
	}

	public void saveHTKtailer(int nstates, PrintWriter fout) {
		fout.println("<TRANSP> " + nstates);
		// First state is non emitting
		for (int j = 0; j < nstates; j++)
			fout.print("0 ");
		fout.println();
		for (int i = 1; i < nstates - 1; i++) {
			for (int j = 0; j < i; j++)
				fout.print("0 ");
			fout.print("0.5 0.5");
			for (int j = i + 3; j < nstates; j++)
				fout.print("0 ");
		}
		fout.println();
		fout.println("0 0 0");
		fout.println("<ENDHMM>");
	}

	public void saveHTK(String nomFich, String nomHMM, String parmKind) {
		try {
			PrintWriter fout = new PrintWriter(new FileWriter(nomFich));
			fout.println("~o");
			fout.println("<HMMSETID> tree");
			fout.println("<STREAMINFO> 1 " + getNcoefs());
			fout.println("<VECSIZE> " + getNcoefs() + "<NULLD>" + parmKind
					+ "<DIAGC>");
			fout.println("~r \"rtree_1\"");
			fout.println("<REGTREE> 1");
			fout.println("<TNODE> 1 " + getNgauss());
			fout.println("~h \"" + nomHMM + '\"');
			fout.println("<BEGINHMM>");
			fout.println("<NUMSTATES> 3");
			fout.println("<STATE> 2");
			fout.println("<NUMMIXES> " + getNgauss());
			for (int i = 1; i <= getNgauss(); i++) {
				fout.println("<MIXTURE> " + i + ' ' + getWeight(i - 1));
				fout.println("<RCLASS> 1");
				fout.println("<MEAN> " + getNcoefs());
				for (int j = 0; j < getNcoefs(); j++) {
					fout.print(getMean(i - 1, j) + " ");
				}
				fout.println();
				fout.println("<VARIANCE> " + getNcoefs());
				for (int j = 0; j < getNcoefs(); j++) {
					fout.print(getVar(i - 1, j) + " ");
				}
				fout.println();
			}
			fout.println("<TRANSP> 3");
			fout.println("0 1 0");
			fout.println("0 0.7 0.3");
			fout.println("0 0 0");
			fout.println("<ENDHMM>");
			fout.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void loadHTK(String nom) {
		try {
			BufferedReader fin = new BufferedReader(new FileReader(nom));
			String s, s2;
			StringTokenizer st;
			ngauss = 0;
			ncoefs = 0;
			for (;;) {
				s = fin.readLine();
				if (s == null)
					break;
				if (s.contains("<MEAN>")) {
					ngauss++;
					if (ncoefs == 0) {
						st = new StringTokenizer(s);
						st.nextToken();
						ncoefs = Integer.parseInt(st.nextToken());
					}
				}
			}
			fin.close();
			allocate();
			fin = new BufferedReader(new FileReader(nom));
			for (int g = 0;;) {
				s = fin.readLine();
				if (s == null)
					break;
				if (s.contains("<MEAN>")) {
					s = fin.readLine();
					st = new StringTokenizer(s);
					for (int c = 0; st.hasMoreTokens(); c++) {
						s2 = st.nextToken();
						setMean(g, c, Float.parseFloat(s2));
					}
					s = fin.readLine();
					if (!s.contains("<VARIANCE>")) {
						fin.close();
						throw new IOException();
					}
					s = fin.readLine();
					st = new StringTokenizer(s);
					for (int c = 0; st.hasMoreTokens(); c++) {
						s2 = st.nextToken();
						setVar(g, c, Float.parseFloat(s2));
					}
					g++;
				}
			}
			fin.close();
			precomputeDistance();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public void loadScaleKMeans(String nom) {
		String s;
		String[] ss;
		int ng = 0;
		try {
			BufferedReader fin = new BufferedReader(new FileReader(nom));
			for (;; ng++) {
				s = fin.readLine();
				if (s == null)
					break;
			}
			ngauss = ng / 2;
			fin.close();
			fin = new BufferedReader(new FileReader(nom));
			s = fin.readLine();
			ss = s.split(" ");
			ncoefs = ss.length - 1;
			fin.close();
			fin = new BufferedReader(new FileReader(nom));
			allocate();
			nT = 0;
			for (int i = 0; i < ngauss; i++) {
				s = fin.readLine();
				ss = s.split(" ");
				weights[i] = Float.parseFloat(ss[0]);
				nT += weights[i];
				for (int j = 0; j < ncoefs; j++) {
					setMean(i, j, Float.parseFloat(ss[j + 1]));
				}
				s = fin.readLine();
				ss = s.split(" ");
				for (int j = 0; j < ncoefs; j++) {
					setVar(i, j, Float.parseFloat(ss[j]));
				}
			}
			for (int i = 0; i < ngauss; i++) {
				setWeight(i, weights[i] / nT);
			}
			fin.close();
			precomputeDistance();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	private void allocateWeights() {
		logMath = LogMath.getInstance();
		weights = new float[ngauss];
		for (int i = 0; i < ngauss; i++) {
			setWeight(i, 1f / ngauss);
		}
	}

	public void precomputeDistance() {
		for (int gidx = 0; gidx < ngauss; gidx++) {
			float fact = 0.0f;
			for (int i = 0; i < ncoefs; i++) {
				fact += logMath.linearToLog(getVar(gidx, i));
			}
			fact += logMath.linearToLog(2.0 * Math.PI) * ncoefs;
			logPreComputedGaussianFactor[gidx] = fact * 0.5f;
		}
	}

	private void allocate() {
		if (weights == null)
			allocateWeights();
		if (means == null) {
			loglikes = new float[ngauss];
			means = new float[ngauss][ncoefs];
			covar = new float[ngauss][ncoefs];
			logPreComputedGaussianFactor = new float[ngauss];
		}
	}

	/*
	 * Log likelihood calculation
	 */

	private static final float distFloor = -Float.MAX_VALUE;

	public void computeLogLikes(float[] data) {
		float logDval1gauss = 0f;
		for (int gidx = 0; gidx < ngauss; gidx++) {
			logDval1gauss = 0f;
			for (int i = 0; i < data.length; i++) {
				float logDiff = data[i] - means[gidx][i];
				logDval1gauss += logDiff * logDiff * covar[gidx][i];
			}
			logDval1gauss -= logPreComputedGaussianFactor[gidx];
			if (Float.isNaN(logDval1gauss)) {
				System.err.println("gs2 is Nan, converting to 0 debug " + gidx
						+ ' ' + logPreComputedGaussianFactor[gidx] + ' '
						+ means[gidx][0] + ' ' + covar[gidx][0]);
				logDval1gauss = LogMath.LOG_ZERO;
			}
			if (logDval1gauss < distFloor) {
				logDval1gauss = distFloor;
			}
			// Including apriori probability for each gaussian
			loglikes[gidx] = weights[gidx] + logDval1gauss;
		}
	}

	/**
	 * Calculate log probability of the observation
	 * must be called AFTER next() !
	 * 
	 * @return log likelihood
	 */
	public float getLogLike() {
		float sc = loglikes[0];
		for (int i = 1; i < ngauss; i++) {
			sc = logMath.addAsLinear(sc, loglikes[i]);
		}
		return sc;
	}

	/**
	 * must be called AFTER next()
	 * 
	 * @return best gaussian
	 */
	public int getWinningGauss() {
		int imax = 0;
		for (int i = 1; i < ngauss; i++) {
			if (loglikes[i] > loglikes[imax])
				imax = i;
		}
		return imax;
	}

	public int getNcoefs() {
		return ncoefs;
	}

	/*
	 * Manipulations with HMMs
	 */

	public GMMDiag getMarginal(boolean[] mask) {
		int nc = 0;
		for (boolean flag : mask)
			if (flag)
				nc++;
		GMMDiag g = new GMMDiag(getNgauss(), nc);
		int curc = 0;
		for (int j = 0; j < ncoefs; j++) {
			if (mask[j]) {
				for (int i = 0; i < ngauss; i++) {
					g.setMean(i, curc, getMean(i, j));
					g.setVar(i, curc, getVar(i, j));
				}
				curc++;
			}
		}
		for (int i = 0; i < ngauss; i++) {
			g.setWeight(i, getWeight(i));
		}
		g.precomputeDistance();
		return g;
	}

	/**
	 * 
	 * @param g  second GMM for the merge
	 * @param w1 weight of the first GMM for the merge
	 * @return gaussian
	 */
	public GMMDiag merge(GMMDiag g, float w1) {
		GMMDiag res = new GMMDiag(getNgauss() + g.getNgauss(), getNcoefs());
		for (int i = 0; i < getNgauss(); i++) {
			System.arraycopy(means[i], 0, res.means[i], 0, getNcoefs());
			System.arraycopy(covar[i], 0, res.covar[i], 0, getNcoefs());
			res.setWeight(i, getWeight(i) * w1);
		}
		for (int i = 0; i < g.getNgauss(); i++) {
			System.arraycopy(g.means[i], 0, res.means[ngauss + i], 0,
					getNcoefs());
			System.arraycopy(g.covar[i], 0, res.covar[ngauss + i], 0,
					getNcoefs());
			res.setWeight(ngauss + i, g.getWeight(i) * (1f - w1));
		}
		res.precomputeDistance();
		return res;
	}

	/**
	 * extracts ONE gaussian from the GMM
	 * 
	 * @param i position
	 * @return gaussian
	 */
	public GMMDiag getGauss(int i) {
		GMMDiag res = new GMMDiag(1, getNcoefs());
		System.arraycopy(means[i], 0, res.means[0], 0, getNcoefs());
		System.arraycopy(covar[i], 0, res.covar[0], 0, getNcoefs());
		res.setWeight(0, 1);
		res.precomputeDistance();
		return res;
	}

	public void setNom(String s) {
		nom = s;
	}

	/**
	 * 2 GMMs are considered to be equal when all of their parameters do not
	 * differ from more than 1%
	 * 

	 * @return if GMMs are equal
	 */
	public boolean isEqual(GMMDiag g) {
		if (getNgauss() != g.getNgauss())
			return false;
		if (getNgauss() != g.getNcoefs())
			return false;
		for (int i = 0; i < getNgauss(); i++) {
			if (isDiff(getWeight(i), g.getWeight(i)))
				return false;
			for (int j = 0; j < getNcoefs(); j++) {
				if (isDiff(getMean(i, j), g.getMean(i, j)))
					return false;
				if (isDiff(getVar(i, j), g.getVar(i, j)))
					return false;
			}
		}
		return true;
	}

	private boolean isDiff(float a, float b) {
        return Math.abs(1 - b / a) > 0.01;
	}

    @Override
    public String toString() {
		StringBuilder sb = new StringBuilder ();
		for (int i = 0; i < getNgauss(); i++) {
			sb.append(getMean(i, 0)).append(' ').append(getVar(i, 0)).append(
					'\n');
		}
		return sb.toString();
	}
}

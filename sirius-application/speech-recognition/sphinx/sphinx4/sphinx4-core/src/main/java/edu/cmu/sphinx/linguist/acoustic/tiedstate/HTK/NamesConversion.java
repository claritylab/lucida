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
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.StringTokenizer;

/**
 * HTK is case-sensitive, S4 is not.
 * 
 * One must then first convert the HMM names to upper-case, resolve conflicts,
 * and use the same conversion to convert the lexicons and grammar. This tool
 * does it.
 * 
 * @author Christophe Cerisara
 * 
 */
public class NamesConversion {
	final HashMap<String,String> phoneConv = new HashMap<String,String>();
	final HashMap<String,String> wordConv = new HashMap<String,String>();
	String left, base, right;
	
	public NamesConversion() {
	}

	void addInConv(String item, HashMap<String,String> conv) {
		if (!conv.containsKey(item)) {
			// new item
			String cand = item.toUpperCase();
			while (conv.containsValue(cand)) {
				// conflict !
				cand = cand+"_X";
			}
			conv.put(item,cand);
		}
	}
	
	void buildPhoneConversion(String MMFfile) {
		try  {
			BufferedReader bf = new BufferedReader(new FileReader(MMFfile));
			String s;
			for (;;) {
				s=bf.readLine();
				if (s==null) break;
				int i=s.indexOf("~h");
				if (i>=0) {
					i=s.indexOf('"');
					int j = s.lastIndexOf('"');
					String nom = s.substring(i+1,j);
					split3ph(nom);
					if (left!=null) addInConv(left,phoneConv);
					if (base!=null) addInConv(base,phoneConv);
					if (right!=null) addInConv(right,phoneConv);
				}
			}
			bf.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	void buildWordConversion(String lexFile) {
		try  {
			BufferedReader bf = new BufferedReader(new FileReader(lexFile));
			String s;
			for (;;) {
				s=bf.readLine();
				if (s==null) break;
				StringTokenizer st = new StringTokenizer(s);
				if (st.hasMoreTokens()) {
					String word = st.nextToken();
					addInConv(word,wordConv);
				}
			}
			bf.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	void split3ph(String nom) {
		int i = nom.indexOf('-');
		if (i>=0) {
			left = nom.substring(0,i);
		} else {left=null; i=-1;}
		String s = nom.substring(i+1);
		i = s.indexOf('+');
		if (i>=0) {
			right = s.substring(i+1);
		} else {right=null; i=s.length();}
		base = s.substring(0,i);
	}
	
	String conv3ph() {
		String rep;
		if (left!=null) {
			rep=conv1ph(left)+ '-';
		} else rep="";
		rep+=conv1ph(base);
		if (right!=null) {
			rep+= '+' +conv1ph(right);
		}
		if (rep.equals("null")) {
			System.err.println("detson error "+left+ ' ' +base+ ' ' +right);
			System.exit(1);
		}
		return rep;
	}
	String conv1ph(String p) {
		return phoneConv.get(p);
	}
	
	void convertMMF(String MMFfile) {
		try  {
			BufferedReader bf = new BufferedReader(new FileReader(MMFfile));
			PrintWriter pf = new PrintWriter(new FileWriter(MMFfile+".conv"));
			String s;
			for (;;) {
				s=bf.readLine();
				if (s==null) break;
				int i=s.indexOf("~h");
				if (i>=0) {
					i=s.indexOf('"');
					int j = s.lastIndexOf('"');
					String nom = s.substring(i+1,j);
					split3ph(nom);
					String newnom = conv3ph();
					pf.println("~h \""+newnom+ '\"');
				} else
					pf.println(s);
			}
			pf.close();
			bf.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	void convertWordGrammar(String gramFile) {
		try  {
			BufferedReader bf = new BufferedReader(new FileReader(gramFile));
			PrintWriter pf = new PrintWriter(new FileWriter(gramFile+".conv"));
			String s;
			// skip comments
			for (;;) {
				s=bf.readLine();
				if (s==null) {pf.close();bf.close();return;}
				pf.println(s);
				int i=s.indexOf("\\data\\");
				if (i==0) break;
			}
			// wait for 1-gram
			for (;;) {
				s=bf.readLine();
				if (s==null) {pf.close();bf.close();return;}
				pf.println(s);
				int i=s.indexOf("\\1-grams:");
				if (i==0) break;
			}
			// 1-grams:
			boolean fin=false;
			while (!fin) {
				s=bf.readLine();
				if (s==null) {pf.close();bf.close();return;}
				int i=s.indexOf("\\2-grams:");
				if (i==0) {
					pf.println(s); break;
				}
				i=s.indexOf("\\end\\");
				if (i==0) {fin=true; pf.println(s); break;}
				StringTokenizer st = new StringTokenizer(s);
				if (st!=null & st.hasMoreTokens()) {
					pf.print(st.nextToken()+ ' ');
					if (st.hasMoreTokens()) {
						String mot = st.nextToken();
						String newmot = wordConv.get(mot);
						if (newmot==null) {
							// when the word is not in the lexicon, we get null here.
							// we should then build a new converted item
							System.err.println("WARNING word "+mot+" not in lexicon !");
							addInConv(mot,wordConv);
							newmot = wordConv.get(mot);
						}
						pf.print(newmot+ ' ');
						while (st.hasMoreTokens())
							pf.print(st.nextToken()+ ' ');
					}
					pf.println();
				}
			}
			// 2-grams:
			while (!fin) {
				s=bf.readLine();
				if (s==null) {pf.close();bf.close();return;}
				int i=s.indexOf("\\3-grams:");
				if (i==0) {
					pf.println(s); break;
				}
				i=s.indexOf("\\end\\");
				if (i==0) {fin=true; pf.println(s); break;}
				StringTokenizer st = new StringTokenizer(s);
				if (st!=null & st.hasMoreTokens()) {
					pf.print(st.nextToken()+ ' ');
					if (st.hasMoreTokens()) {
						String mot = st.nextToken();
						String newmot = wordConv.get(mot);
						if (newmot==null) newmot=mot;
						pf.print(newmot+ ' ');
						if (st.hasMoreTokens()) {
							mot = st.nextToken();
							newmot = wordConv.get(mot);
							if (newmot==null) newmot=mot;
							pf.print(newmot+ ' ');
							while (st.hasMoreTokens())
								pf.print(st.nextToken()+ ' ');
						}
					}
					pf.println();
				}
			}
			// 3-grams:
			while (!fin) {
				s=bf.readLine();
				if (s==null) {pf.close();bf.close();return;}
				int i=s.indexOf("\\end\\");
				if (i==0) {fin=true; pf.println(s); break;}
				StringTokenizer st = new StringTokenizer(s);
				if (st!=null & st.hasMoreTokens()) {
					pf.print(st.nextToken()+ ' ');
					if (st.hasMoreTokens()) {
						String mot = st.nextToken();
						String newmot = wordConv.get(mot);
						if (newmot==null) newmot=mot;
						pf.print(newmot+ ' ');
						if (st.hasMoreTokens()) {
							mot = st.nextToken();
							newmot = wordConv.get(mot);
							if (newmot==null) newmot=mot;
							pf.print(newmot+ ' ');
							if (st.hasMoreTokens()) {
								mot = st.nextToken();
								newmot = wordConv.get(mot);
								if (newmot==null) newmot=mot;
								pf.print(newmot+ ' ');
								while (st.hasMoreTokens())
									pf.print(st.nextToken()+ ' ');
							}
						}
					}
					pf.println();
				}
			}
			pf.close();
			bf.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	void convertLexicon(String lexFile) {
		try  {
			BufferedReader bf = new BufferedReader(new FileReader(lexFile));
			PrintWriter pf = new PrintWriter(new FileWriter(lexFile+".conv"));
			String s;
			for (;;) {
				s=bf.readLine();
				if (s==null) break;
				StringTokenizer st = new StringTokenizer(s);
				if (st==null || !st.hasMoreTokens()) continue;
				String mot = st.nextToken();
				String newmot = wordConv.get(mot);
				if (newmot!=null) mot=newmot;
				pf.print(mot+ ' ');
				while (st.hasMoreTokens()) {
					String ph = st.nextToken();
					// format julius: delete the output string between [..]
					if (ph.charAt(0)=='[') {
						for (;;) {
							if (ph.endsWith("]")) break;
							ph = st.nextToken();
						}
						ph = st.nextToken();
					}
					split3ph(ph);
					String newnom = conv3ph();
					pf.print(newnom+ ' ');
				}
				pf.println();
			}
			pf.close();
			bf.close();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	// TODO: support without filler, which shall be loaded from the HTK lexicon in the future
	public static void main(String args[]) {
		String MMFfile = null;
		String lexFile = null;
		String fillerFile = null;
		String gramFile = null;

		for (int i=0;i<args.length;i++) {
			if (args[i].equals("-lex")) {
				lexFile = args[++i];
			} else if (args[i].equals("-gram")) {
				gramFile = args[++i];
			} else if (args[i].equals("-mmf")) {
				MMFfile = args[++i];
			} else if (args[i].equals("-filler")) {
				fillerFile = args[++i];
			}
		}
		// output = same files + extension ".conv"
		if (MMFfile!=null) {
			// conversion des phonemes et des mots
			NamesConversion nc = new NamesConversion();
			nc.buildPhoneConversion(MMFfile);
			nc.buildWordConversion(lexFile);
			System.out.println("converting phones in MMF to "+MMFfile+".conv");
			nc.convertMMF(MMFfile);
			if (lexFile!=null) {
				System.out.println("converting phones and words in lexicon to "+lexFile+".conv");
				nc.convertLexicon(lexFile);
			}
			if (fillerFile!=null) {
				System.out.println("converting phones in filler to "+fillerFile+".conv");
				nc.convertLexicon(fillerFile);
			}
			if (gramFile!=null) {
				System.out.println("converting words in gram to "+gramFile+".conv");
				nc.convertWordGrammar(gramFile);
			}
		}
	}
}

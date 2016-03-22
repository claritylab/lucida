package info.ephyra.nlp.semantics;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.questionanalysis.Term;
import info.ephyra.util.RegexConverter;

import java.io.Serializable;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>A <code>Predicate</code> represents a predicate-argument structure as defined in the PropBank project.<p>
 * 
 * <p>This class implements the interfaces <code>Comparable</code> and <code>Serializable</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-01-29
 */
public class Predicate implements Comparable<Predicate>, Serializable {
	/** Version number used during deserialization. */
	private static final long serialVersionUID = 20070501;
	
	/** Indicates a missing argument. */
	public static final String MISSING_ARG = "[missing]";
	
	/** Pattern to extract the verb from an annotated sentence. */
	private static final Pattern VERB_P = Pattern.compile("(?i)\\[TARGET ([^\\]]*+)]");
	/** Pattern to extract arguments ARG0-5 from an annotated sentence. */
	private static final Pattern ARG_P = Pattern.compile("(?i)\\[(ARG[0-5]) ([^\\]]*+)]");
	/** Pattern to extract modifier arguments from an annotated sentence. */
	private static final Pattern MOD_P = Pattern.compile("(?i)\\[(ARGM-\\w*+) ([^\\]]*+)]");
	
	/** Original sentence (optional). */
	private String sentence;
	/** Sentence annotated with semantic roles (optional). */
	private String annotated;
	
	/** Verb representing the predicate. */
	private String verb;
	/** Arguments with verb-specific roles. Often, ARG0 is the Agent and ARG1 the Patient or Theme. */
	private String[] args = new String[6];
	/** LOC modifier (location). */
	private String argLOC;
	/** CAU modifier (cause). */
	private String argCAU;
	/**	EXT modifier (extent). */
	private String argEXT;
	/** TMP modifier (time). */
	private String argTMP;
	/** DIS modifier (discourse connective, e.g. "also", "on the other hand"). */
	private String argDIS;
	/** PNC modifier (purpose). */
	private String argPNC;
	/** ADV modifier (sentence-level adverb, e.g. "unfortunately", and general-purpose). */
	private String argADV;
	/** MNR modifier (manner). */
	private String argMNR;
	/** NEG modifier (negation). */
	private String argNEG;
	/** DIR modifier (direction). */
	private String argDIR;
	/** MOD modifier (modal verb). */
	private String argMOD;
	/** Missing arguments, for predicates extracted from questions. */
	private String[] missingArgs = new String[0];
	
	/** Confidence score (from [0, 1]). */
	private float confidence = 1;
	
	/** Term for the verb. */
	private Term verbTerm;
	/** Terms in the arguments. */
	private Term[] argTerms;
	/** Similarity score of this predicate compared to another predicate (from [0, 1]). */
	private double simScore;
	/** Predicate this predicate was compared to. */
	private Predicate simPredicate;
	
	/**
	 * Creates a predicate data structure from a verb, arguments and their semantic roles.
	 * 
	 * @param verb the verb
	 * @param args the arguments
	 * @param roles the semantic roles
	 */
	public Predicate(String verb, String[] args, String[] roles) {
		this.verb = verb;
		appendAll(roles, args);
	}
	
	/**
	 * Creates a predicate data structure from an annotated sentence and sets the verb and argument terms.
	 * 
	 * @param sentence the original sentence
	 * @param annotated the sentence annotated with semantic roles
	 * @param terms the terms in the sentence
	 */
	public Predicate(String sentence, String annotated, Term[] terms) throws ParseException {
		this.sentence = sentence;
		this.annotated = annotated;
		
		// extract verb
		Matcher verbM = VERB_P.matcher(annotated);
		if (!verbM.find()) {
			MsgPrinter.printErrorMsg("Problem building predicate: " + "TARGET missing.");
			MsgPrinter.printErrorMsg("Sentence: " + sentence);
			MsgPrinter.printErrorMsg("Annotation: " + annotated);
			throw new ParseException("Predicate annotation could not be parsed.", 0);
		}
		verb = ASSERT.untokenize(verbM.group(1).trim(), sentence);
		if (verb == null) {
			MsgPrinter.printErrorMsg("Problem building predicate: " + "TARGET not in original sentence.");
			MsgPrinter.printErrorMsg("Sentence: " + sentence);
			MsgPrinter.printErrorMsg("Annotation: " + annotated);
			throw new ParseException("Predicate annotation could not be parsed.", 0);
		}
		
		ArrayList<String> rolesL = new ArrayList<String>();
		ArrayList<String> argsL = new ArrayList<String>();
		
		// extract arguments ARG0-5
		Matcher argM = ARG_P.matcher(annotated);
		while (argM.find()) {
			String role = argM.group(1).toUpperCase();
			String arg = ASSERT.untokenize(argM.group(2).trim(), sentence);
			if (arg == null) {
				MsgPrinter.printErrorMsg("Problem building predicate: " + role + " not in original sentence.");
				MsgPrinter.printErrorMsg("Sentence: " + sentence);
				MsgPrinter.printErrorMsg("Annotation: " + annotated);
			} else {
				rolesL.add(role);
				argsL.add(arg);
			}
		}
		
		// extract modifier arguments
		Matcher modM = MOD_P.matcher(annotated);
		while (modM.find()) {
			String role = modM.group(1).toUpperCase();
			String arg = ASSERT.untokenize(modM.group(2).trim(), sentence);
			if (arg == null) {
				MsgPrinter.printErrorMsg("Problem building predicate: " + role + " not in original sentence.");
				MsgPrinter.printErrorMsg("Sentence: " + sentence);
				MsgPrinter.printErrorMsg("Annotation: " + annotated);
			} else {
				rolesL.add(role);
				argsL.add(arg);
			}
		}
		
		String[] roles = rolesL.toArray(new String[rolesL.size()]);
		String[] args = argsL.toArray(new String[argsL.size()]);
		appendAll(roles, args);
		
		// set verb and argument terms
		setVerbTerm(terms);
		if (verbTerm == null) {
//			MsgPrinter.printErrorMsg("Problem building predicate: " + "No verb term found.");
//			MsgPrinter.printErrorMsg("Sentence: " + sentence);
//			MsgPrinter.printErrorMsg("Annotation: " + annotated);
			throw new ParseException("Predicate annotation could not be parsed.", 0);
		}
		setArgTerms(terms);
		if (argTerms.length == 0) {
//			MsgPrinter.printErrorMsg("Problem building predicate: " + "No argument terms found.");
//			MsgPrinter.printErrorMsg("Sentence: " + sentence);
//			MsgPrinter.printErrorMsg("Annotation: " + annotated);
			throw new ParseException("Predicate annotation could not be parsed.", 0);
		}
	}
	
	/**
	 * Sets the term for the verb.
	 * 
	 * @param terms the terms in the sentence the predicate was extracted from
	 */
	private void setVerbTerm(Term[] terms) {
		String tokenizedVerb = NETagger.tokenizeWithSpaces(verb);
		Pattern p = Pattern.compile("(^|\\W)" + RegexConverter.strToRegex(tokenizedVerb) + "($|\\W)");
		
		for (Term term : terms) {
			String tokenizedTerm = NETagger.tokenizeWithSpaces(term.getText());
			Matcher m = p.matcher(tokenizedTerm);
			if (m.find()) {
				verbTerm = term;
				break;
			}
		}
	}
	
	/**
	 * Sets the terms in the arguments.
	 * 
	 * @param terms the terms in the sentence the predicate was extracted from
	 */
	private void setArgTerms(Term[] terms) {
		List<String> tokenizedArgs = new ArrayList<String>();
		for (String arg : args) if (arg != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(arg));
		if (argLOC != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argLOC));
		if (argCAU != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argCAU));
		if (argEXT != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argEXT));
		if (argTMP != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argTMP));
		if (argDIS != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argDIS));
		if (argPNC != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argPNC));
		if (argADV != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argADV));
		if (argMNR != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argMNR));
		if (argNEG != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argNEG));
		if (argDIR != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argDIR));
		if (argMOD != null) tokenizedArgs.add(NETagger.tokenizeWithSpaces(argMOD));
		
		List<Term> argTerms = new ArrayList<Term>();
		Set<String> uniqueTerms = new HashSet<String>();  // used to eliminate duplicate terms
		for (Term term : terms) {
			String tokenizedTerm = NETagger.tokenizeWithSpaces(term.getText());
			Pattern p = Pattern.compile("(^|\\W)" + RegexConverter.strToRegex(tokenizedTerm) + "($|\\W)");
			for (String tokenizedArg : tokenizedArgs) {
				Matcher m = p.matcher(tokenizedArg);
				if (m.find()) {
					if (uniqueTerms.add(tokenizedTerm)) argTerms.add(term);
					break;
				}
			}
		}
		this.argTerms = argTerms.toArray(new Term[argTerms.size()]);
	}
	
	/**
	 * Drops all arguments that contain the given string.
	 * 
	 * @param s the string
	 * @return <code>true</code> iff an argument was dropped
	 */
	public boolean dropArgs(String s) {
		boolean dropped = false;
		ArrayList<String> droppedArgs = new ArrayList<String>();
		
		for (int i = 0; i < args.length; i++)
			if (args[i] != null && args[i].contains(s)) {args[i] = null; dropped = true; droppedArgs.add("ARG" + i);}
		if (argLOC != null && argLOC.contains(s)) {argLOC = null; dropped = true; droppedArgs.add("ARGM-LOC");}
		if (argCAU != null && argCAU.contains(s)) {argCAU = null; dropped = true; droppedArgs.add("ARGM-CAU");}
		if (argEXT != null && argEXT.contains(s)) {argEXT = null; dropped = true; droppedArgs.add("ARGM-EXT");}
		if (argTMP != null && argTMP.contains(s)) {argTMP = null; dropped = true; droppedArgs.add("ARGM-TMP");}
		if (argDIS != null && argDIS.contains(s)) {argDIS = null; dropped = true; droppedArgs.add("ARGM-DIS");}
		if (argPNC != null && argPNC.contains(s)) {argPNC = null; dropped = true; droppedArgs.add("ARGM-PNC");}
		if (argADV != null && argADV.contains(s)) {argADV = null; dropped = true; droppedArgs.add("ARGM-ADV");}
		if (argMNR != null && argMNR.contains(s)) {argMNR = null; dropped = true; droppedArgs.add("ARGM-MNR");}
		if (argNEG != null && argNEG.contains(s)) {argNEG = null; dropped = true; droppedArgs.add("ARGM-NEG");}
		if (argDIR != null && argDIR.contains(s)) {argDIR = null; dropped = true; droppedArgs.add("ARGM-DIR");}
		if (argMOD != null && argMOD.contains(s)) {argMOD = null; dropped = true; droppedArgs.add("ARGM-MOD");}
		
		if (droppedArgs.size() > 0) {
			// remember dropped arguments
			String[] missing = new String[droppedArgs.size() + missingArgs.length];
			droppedArgs.toArray(missing);
			for (int i = 0; i < missingArgs.length; i++)
				missing[droppedArgs.size() + i] = missingArgs[i];
			missingArgs = missing;
			
			// set terms in remaining arguments
			setArgTerms(argTerms);
		}
		
		return dropped;
	}
	
	/**
	 * Calculates a similarity score from [0,1] for this predicate and a given predicate.
	 * 
	 * @param p predicate to compare with
	 * @return similarity score
	 */
	public double simScore(Predicate p) {
		// calculate similarity score for the verbs
		Term pVerbTerm = p.getVerbTerm();
		if (verbTerm == null || pVerbTerm == null) return 0;
		String verbTermLemma = verbTerm.getLemma();
		double verbScore = pVerbTerm.simScore(verbTermLemma);
		if (verbScore == 0) return 0;
		
		// calculate similarity score for the arguments
		// (Jaccard coefficient)
		Term[] pArgTerms = p.getArgTerms();
		if (argTerms.length == 0 || pArgTerms.length == 0) return 0;
		double intersect = 0;
		int union = pArgTerms.length;
		for (Term argTerm : argTerms) {
			String argTermLemma = argTerm.getLemma();
			double argTermScore = 0;
			for (Term pArgTerm : pArgTerms)
				argTermScore = Math.max(argTermScore, pArgTerm.simScore(argTermLemma));
			if (argTermScore > 0) intersect += argTermScore; else union++;
		}
		double argScore = intersect / union;
		if (argScore == 0) return 0;
		
		// predicate similarity score is the product of verb score and argument score
		simScore = verbScore * argScore;
		
		// remember the predicate this predicate was compared to
		simPredicate = p;
		
		return simScore;
	}
	
	/**
	 * Compares two predicates by comparing their similarity scores.
	 * 
	 * @param p the predicate to be compared
	 * @return a negative integer, zero or a positive integer as this predicate is less than, equal to or greater than
	 *         the specified predicate
	 */
	public int compareTo(Predicate p) {
		double diff = simScore - p.simScore;
		
		if (diff < 0) return -1;
		else if (diff > 0) return 1;
		else return 0;
	}
	
	/**
	 * Returns a single-line string representation of the predicate.
	 * 
	 * @return single-line string representation
	 */
	public String toString() {
		// put missing arguments in a set for fast lookups
		HashSet<String> missingSet = new HashSet<String>();
		for (String missingArg : missingArgs) missingSet.add(missingArg);
		
		String s = verb + "(";
		for (int i = 0; i < args.length; i++) 
			if (args[i] != null) s += "ARG" + i + ": " + args[i] + ", ";
			else if (missingSet.contains("ARG" + i)) s += "ARG" + i + ": " + MISSING_ARG + ", ";
		if (argLOC != null) s += "ARGM-LOC: " + argLOC + ", ";
		else if (missingSet.contains("ARGM-LOC")) s += "ARGM-LOC: " + MISSING_ARG + ", ";
		if (argCAU != null) s += "ARGM-CAU: " + argCAU + ", ";
		else if (missingSet.contains("ARGM-CAU")) s += "ARGM-CAU: " + MISSING_ARG + ", ";
		if (argEXT != null) s += "ARGM-EXT: " + argEXT + ", ";
		else if (missingSet.contains("ARGM-EXT")) s += "ARGM-EXT: " + MISSING_ARG + ", ";
		if (argTMP != null) s += "ARGM-TMP: " + argTMP + ", ";
		else if (missingSet.contains("ARGM-TMP")) s += "ARGM-TMP: " + MISSING_ARG + ", ";
		if (argDIS != null) s += "ARGM-DIS: " + argDIS + ", ";
		else if (missingSet.contains("ARGM-DIS")) s += "ARGM-DIS: " + MISSING_ARG + ", ";
		if (argPNC != null) s += "ARGM-PNC: " + argPNC + ", ";
		else if (missingSet.contains("ARGM-PNC")) s += "ARGM-PNC: " + MISSING_ARG + ", ";
		if (argADV != null) s += "ARGM-ADV: " + argADV + ", ";
		else if (missingSet.contains("ARGM-ADV")) s += "ARGM-ADV: " + MISSING_ARG + ", ";
		if (argMNR != null) s += "ARGM-MNR: " + argMNR + ", ";
		else if (missingSet.contains("ARGM-MNR")) s += "ARGM-MNR: " + MISSING_ARG + ", ";
		if (argNEG != null) s += "ARGM-NEG: " + argNEG + ", ";
		else if (missingSet.contains("ARGM-NEG")) s += "ARGM-NEG: " + MISSING_ARG + ", ";
		if (argDIR != null) s += "ARGM-DIR: " + argDIR + ", ";
		else if (missingSet.contains("ARGM-DIR")) s += "ARGM-DIR: " + MISSING_ARG + ", ";
		if (argMOD != null) s += "ARGM-MOD: " + argMOD + ", ";
		else if (missingSet.contains("ARGM-MOD")) s += "ARGM-MOD: " + MISSING_ARG + ", ";
		if (s.endsWith(", ")) s = s.substring(0, s.length() - 2);
		s += ")";
		
		return s;
	}
	
	/**
	 * Returns a multi-line string representation of the predicate.
	 * 
	 * @return multi-line string representation
	 */
	public String toStringMultiLine() {
		String s = "TARGET: " + verb;
		for (int i = 0; i < args.length; i++) if (args[i] != null) s += "\nARG" + i + ": " + args[i];
		if (argLOC != null) s += "\nARGM-LOC: " + argLOC;
		if (argCAU != null) s += "\nARGM-CAU: " + argCAU;
		if (argEXT != null) s += "\nARGM-EXT: " + argEXT;
		if (argTMP != null) s += "\nARGM-TMP: " + argTMP;
		if (argDIS != null) s += "\nARGM-DIS: " + argDIS;
		if (argPNC != null) s += "\nARGM-PNC: " + argPNC;
		if (argADV != null) s += "\nARGM-ADV: " + argADV;
		if (argMNR != null) s += "\nARGM-MNR: " + argMNR;
		if (argNEG != null) s += "\nARGM-NEG: " + argNEG;
		if (argDIR != null) s += "\nARGM-DIR: " + argDIR;
		if (argMOD != null) s += "\nARGM-MOD: " + argMOD;
		for (String missingArg : missingArgs)
			s += "\n" + missingArg + ": " + MISSING_ARG;
		
		return s;
	}
	
	/**
	 * Returns the verb or an argument of the predicate.
	 * 
	 * @param id identifier of the verb or argument
	 * @return verb or argument, or <code>null</code> if the identifier is invalid or the argument does not exist
	 */
	public String get(String id) {
		if (id.equals("TARGET")) return verb;
		if (id.equals("ARG0")) return args[0];
		if (id.equals("ARG1")) return args[1];
		if (id.equals("ARG2")) return args[2];
		if (id.equals("ARG3")) return args[3];
		if (id.equals("ARG4")) return args[4];
		if (id.equals("ARG5")) return args[5];
		if (id.equals("ARGM-LOC")) return argLOC;
		if (id.equals("ARGM-CAU")) return argCAU;
		if (id.equals("ARGM-EXT")) return argEXT;
		if (id.equals("ARGM-TMP")) return argTMP;
		if (id.equals("ARGM-DIS")) return argDIS;
		if (id.equals("ARGM-PNC")) return argPNC;
		if (id.equals("ARGM-ADV")) return argADV;
		if (id.equals("ARGM-MNR")) return argMNR;
		if (id.equals("ARGM-NEG")) return argNEG;
		if (id.equals("ARGM-DIR")) return argDIR;
		if (id.equals("ARGM-MOD")) return argMOD;
		
		return null;
	}
	
	/**
	 * Returns all arguments that are not <code>null</code> in an array.
	 * 
	 * @return array containing the arguments
	 */
	public String[] getArgs() {
		ArrayList<String> argsList = new ArrayList<String>();
		
		for (String arg : args)
			if (arg != null) argsList.add(arg);
		if (argLOC != null) argsList.add(argLOC);
		if (argCAU != null) argsList.add(argCAU);
		if (argEXT != null) argsList.add(argEXT);
		if (argTMP != null) argsList.add(argTMP);
		if (argDIS != null) argsList.add(argDIS);
		if (argPNC != null) argsList.add(argPNC);
		if (argADV != null) argsList.add(argADV);
		if (argMNR != null) argsList.add(argMNR);
		if (argNEG != null) argsList.add(argNEG);
		if (argDIR != null) argsList.add(argDIR);
		if (argMOD != null) argsList.add(argMOD);
		
		return argsList.toArray(new String[argsList.size()]);
	}
	
	/**
	 * Returns all arguments that are not <code>null</code>, including missing arguments, in an array.
	 * 
	 * @return array containing the arguments
	 */
	public String[] getArgsMissing() {
		String[] args = getArgs();
		
		String[] argsMissing = new String[args.length + missingArgs.length];
		for (int i = 0; i < args.length; i++) argsMissing[i] = args[i];
		for (int i = 0; i < missingArgs.length; i++) argsMissing[args.length + i] = MISSING_ARG;
		
		return argsMissing;
	}
	
	/**
	 * Returns all semantic roles that are not <code>null</code> in an array.
	 * 
	 * @return array containing the semantic roles
	 */
	public String[] getRoles() {
		ArrayList<String> rolesList = new ArrayList<String>();
		
		for (int i = 0; i < args.length; i++)
			if (args[i] != null) rolesList.add("ARG" + i);
		if (argLOC != null) rolesList.add("ARGM-LOC");
		if (argCAU != null) rolesList.add("ARGM-CAU");
		if (argEXT != null) rolesList.add("ARGM-EXT");
		if (argTMP != null) rolesList.add("ARGM-TMP");
		if (argDIS != null) rolesList.add("ARGM-DIS");
		if (argPNC != null) rolesList.add("ARGM-PNC");
		if (argADV != null) rolesList.add("ARGM-ADV");
		if (argMNR != null) rolesList.add("ARGM-MNR");
		if (argNEG != null) rolesList.add("ARGM-NEG");
		if (argDIR != null) rolesList.add("ARGM-DIR");
		if (argMOD != null) rolesList.add("ARGM-MOD");
		
		return rolesList.toArray(new String[rolesList.size()]);
	}
	
	/**
	 * Returns all semantic roles that are not <code>null</code>, including missing roles, in an array.
	 * 
	 * @return array containing the semantic roles
	 */
	public String[] getRolesMissing() {
		String[] roles = getRoles();
		
		String[] rolesMissing = new String[roles.length + missingArgs.length];
		for (int i = 0; i < roles.length; i++) rolesMissing[i] = roles[i];
		for (int i = 0; i < missingArgs.length; i++) rolesMissing[roles.length + i] = missingArgs[i];
		
		return rolesMissing;
	}
	
	/**
	 * Returns the verb and all arguments that are not <code>null</code> in an array.
	 * 
	 * @return array containing the verb and the arguments
	 */
	public String[] getVerbArgs() {
		String[] args = getArgs();
		
		String[] verbArgs = new String[args.length + 1];
		verbArgs[0] = verb;
		for (int i = 0; i < args.length; i++)
			verbArgs[i + 1] = args[i];
		
		return verbArgs;
	}
	
	/**
	 * Sets the verb or an argument of the predicate.
	 * 
	 * @param id identifier of the verb or argument
	 * @param verbArg verb or argument to be set
	 * @return <code>true</code> iff the identifier is valid
	 */
	public boolean set(String id, String verbArg) {
		if (id.equals("TARGET")) verb = verbArg;
		else if (id.equals("ARG0")) args[0] = verbArg;
		else if (id.equals("ARG1")) args[1] = verbArg;
		else if (id.equals("ARG2")) args[2] = verbArg;
		else if (id.equals("ARG3")) args[3] = verbArg;
		else if (id.equals("ARG4")) args[4] = verbArg;
		else if (id.equals("ARG5")) args[5] = verbArg;
		else if (id.equals("ARGM-LOC")) argLOC = verbArg;
		else if (id.equals("ARGM-CAU")) argCAU = verbArg;
		else if (id.equals("ARGM-EXT")) argEXT = verbArg;
		else if (id.equals("ARGM-TMP")) argTMP = verbArg;
		else if (id.equals("ARGM-DIS")) argDIS = verbArg;
		else if (id.equals("ARGM-PNC")) argPNC = verbArg;
		else if (id.equals("ARGM-ADV")) argADV = verbArg;
		else if (id.equals("ARGM-MNR")) argMNR = verbArg;
		else if (id.equals("ARGM-NEG")) argNEG = verbArg;
		else if (id.equals("ARGM-DIR")) argDIR = verbArg;
		else if (id.equals("ARGM-MOD")) argMOD = verbArg;
		else return false;
		
		return true;
	}
	
	/**
	 * Appends a string to the verb or an argument of the predicate.
	 * 
	 * @param id identifier of the verb or argument
	 * @param verbArg string to be appended
	 * @return <code>true</code> iff the identifier is valid
	 */
	public boolean append(String id, String verbArg) {
		if (id.equals("TARGET")) {
			if (verb == null) verb = ""; else verb += "\t";
			verb += verbArg;
		} else if (id.equals("ARG0"))  {
			if (args[0] == null) args[0] = ""; else args[0] += "\t";
			args[0] += verbArg;
		} else if (id.equals("ARG1")) {
			if (args[1] == null) args[1] = ""; else args[1] += "\t";
			args[1] += verbArg;
		} else if (id.equals("ARG2")) {
			if (args[2] == null) args[2] = ""; else args[2] += "\t";
			args[2] += verbArg;
		} else if (id.equals("ARG3")) {
			if (args[3] == null) args[3] = ""; else args[3] += "\t";
			args[3] += verbArg;
		} else if (id.equals("ARG4")) {
			if (args[4] == null) args[4] = ""; else args[4] += "\t";
			args[4] += verbArg;
		} else if (id.equals("ARG5")) {
			if (args[5] == null) args[5] = ""; else args[5] += "\t";
			args[5] += verbArg;
		} else if (id.equals("ARGM-LOC")) {
			if (argLOC == null) argLOC = ""; else argLOC += "\t";
			argLOC += verbArg;
		} else if (id.equals("ARGM-CAU")) {
			if (argCAU == null) argCAU = ""; else argCAU += "\t";
			argCAU += verbArg;
		} else if (id.equals("ARGM-EXT")) {
			if (argEXT == null) argEXT = ""; else argEXT += "\t";
			argEXT += verbArg;
		} else if (id.equals("ARGM-TMP")) {
			if (argTMP == null) argTMP = ""; else argTMP += "\t";
			argTMP += verbArg;
		} else if (id.equals("ARGM-DIS")) {
			if (argDIS == null) argDIS = ""; else argDIS += "\t";
			argDIS += verbArg;
		} else if (id.equals("ARGM-PNC")) {
			if (argPNC == null) argPNC = ""; else argPNC += "\t";
			argPNC += verbArg;
		} else if (id.equals("ARGM-ADV")) {
			if (argADV == null) argADV = ""; else argADV += "\t";
			argADV += verbArg;
		} else if (id.equals("ARGM-MNR")) {
			if (argMNR == null) argMNR = ""; else argMNR += "\t";
			argMNR += verbArg;
		} else if (id.equals("ARGM-NEG")) {
			if (argNEG == null) argNEG = ""; else argNEG += "\t";
			argNEG += verbArg;
		} else if (id.equals("ARGM-DIR")) {
			if (argDIR == null) argDIR = ""; else argDIR += "\t";
			argDIR += verbArg;
		} else if (id.equals("ARGM-MOD")) {
			if (argMOD == null) argMOD = ""; else argMOD += "\t";
			argMOD += verbArg;
		} else return false;
		
		return true;
	}
	
	/**
	 * Sets the verb and/or arguments of the predicate.
	 * 
	 * @param ids identifiers of the verb and/or arguments
	 * @param verbArgs verb and/or arguments to be set
	 * @return <code>true</code> iff all identifiers are valid
	 */
	public boolean setAll(String[] ids, String[] verbArgs) {
		ArrayList<String> missingList = new ArrayList<String>();
		
		boolean valid = true;
		for (int i = 0; i < verbArgs.length; i++)
			if (verbArgs[i].equals(MISSING_ARG) && !ids[i].equals("TARGET")) {
				if (!missingList.contains(ids[i])) missingList.add(ids[i]);
			} else {
				if (!set(ids[i], verbArgs[i])) valid = false;
			}
		
		missingArgs = missingList.toArray(new String[missingList.size()]);
		
		return valid;
	}
	
	/**
	 * Appends strings to the verb and/or arguments of the predicate.
	 * 
	 * @param ids identifiers of the verb and/or arguments
	 * @param verbArgs strings to be appended
	 * @return <code>true</code> iff all identifiers are valid
	 */
	public boolean appendAll(String[] ids, String[] verbArgs) {
		ArrayList<String> missingList = new ArrayList<String>();
		for (String missingArg : missingArgs) missingList.add(missingArg);
		
		boolean valid = true;
		for (int i = 0; i < verbArgs.length; i++)
			if (verbArgs[i].equals(MISSING_ARG) && !ids[i].equals("TARGET")) {
				if (!missingList.contains(ids[i])) missingList.add(ids[i]);
			} else {
				if (!append(ids[i], verbArgs[i])) valid = false;
			}
		
		missingArgs = missingList.toArray(new String[missingList.size()]);
		
		return valid;
	}
	
	public String getSentence() {
		return sentence;
	}
	
	public String getAnnotated() {
		return annotated;
	}
	
	public String getVerb() {
		return verb;
	}
	
	public String getNumArg(int role) {
		return args[role];
	}
	
	public String[] getNumArgs() {
		return args;
	}
	
	public String getArgLOC() {
		return argLOC;
	}
	
	public String getArgCAU() {
		return argCAU;
	}
	
	public String getArgEXT() {
		return argEXT;
	}
	
	public String getArgTMP() {
		return argTMP;
	}
	
	public String getArgDIS() {
		return argDIS;
	}
	
	public String getArgPNC() {
		return argPNC;
	}
	
	public String getArgADV() {
		return argADV;
	}
	
	public String getArgMNR() {
		return argMNR;
	}
	
	public String getArgNEG() {
		return argNEG;
	}
	
	public String getArgDIR() {
		return argDIR;
	}
	
	public String getArgMOD() {
		return argMOD;
	}
	
	public String[] getMissingArgs() {
		return missingArgs;
	}
	
	public boolean hasMissingArgs() {
		return missingArgs.length > 0;
	}
	
	public float getConfidence() {
		return confidence;
	}
	
	public void setConfidence(float confidence) {
		this.confidence = confidence;
	}
	
	public Term getVerbTerm() {
		return verbTerm;
	}
	
	public Term[] getArgTerms() {
		return argTerms;
	}
	
	public double getSimScore() {
		return simScore;
	}
	
	public void setSimScore(double simScore) {
		this.simScore = simScore;
	}
	
	public Predicate getSimPredicate() {
		return simPredicate;
	}
	
	public void setSimPredicate(Predicate simPredicate) {
		this.simPredicate = simPredicate;
	}
}

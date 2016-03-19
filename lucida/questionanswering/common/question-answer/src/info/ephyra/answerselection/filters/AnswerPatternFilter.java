package info.ephyra.answerselection.filters;

import info.ephyra.answerselection.AnswerPattern;
import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.QuestionInterpretation;
import info.ephyra.search.Result;
import info.ephyra.util.FileUtils;
import info.ephyra.util.RegexConverter;
import info.ephyra.util.StringUtils;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;

/**
 * <p>The <code>AnswerPatternFilter</code> uses answer patterns to extract
 * factoid answers from text passages and to rank them. It is only applied if
 * the question could be interpreted, i.e. a <code>QuestionInterpretation</code>
 * is available.</p>
 * 
 * <p>In addition, this <code>Filter</code> provides methods to add, load, save
 * and assess the answer patterns.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-04-18
 */
public class AnswerPatternFilter extends Filter {
	/** Identifier for the pattern learning and matching approach. */
	public static final String ID = "Pattern learning and matching";
	
	/** For each PROPERTY the answer patterns that are used to extract it. */
	private static Hashtable<String, HashSet<AnswerPattern>> props =
		new Hashtable<String, HashSet<AnswerPattern>>();
	/**
	 * For each PROPERTY the number of text passages used to assess the answer
	 * patterns.
	 */
	private static Hashtable<String, Integer> nOfPassages =
		new Hashtable<String, Integer>();
	/** Maps tags in PROPERTY objects back to the original strings. */
	private static Hashtable<String, String> reverseMap =
		new Hashtable<String, String>();
	/** PROPERTY objects extracted from a <code>Result</code>. */
	private static ArrayList<String> extr;
	/** For each PROPERTY object the NE types. */
	private static ArrayList<String[]> types;
	/** For each PROPERTY object the sentence it was extracted from. */
	private static ArrayList<String> sents;
	/** For each PROPERTY object the answer pattern used to extract it. */
	private static ArrayList<AnswerPattern> aps;
	
	/**
	 * Replaces all TARGET objects in the sentence. The reverse mappings are
	 * stored in <code>reverseMap</code>.
	 * 
	 * @param sentence input sentence
	 * @param to the TARGET object of the question
	 * @param nes the NEs in the sentence
	 * @return sentence with TARGET tags or <code>null</code>, if the sentence
	 * 		   does not contain the TARGET
	 */
	private static String replaceTarget(String sentence, String to,
										String[][] nes) {
		HashSet<String> reps = new HashSet<String>();
		String tag, result = sentence;
		int id = 1;
		
		for (String[] neType : nes)
			for (String ne : neType)
				if (StringUtils.equalsCommonNorm(ne, to)) reps.add(ne);
		to = "(?i)" + RegexConverter.strToRegexWithBounds(to);
		Matcher m = Pattern.compile(to).matcher(result);
		while (m.find()) reps.add(m.group(0));  // get proper case
		
		// sort expressions by length
		String[] sorted = reps.toArray(new String[reps.size()]);
		StringUtils.sortByLengthDesc(sorted);
		
		for (String rep : sorted) {
			tag = "<TO_" + id++ + ">";  // add unique tag ID
			reverseMap.put(tag, rep);  // remember reverse mapping
			rep = RegexConverter.strToRegexWithBounds(rep);
			result = result.replaceAll(rep, tag);
		}
		
		return (result.equals(sentence)) ? null : result;
	}
	
	/**
	 * Replaces all CONTEXT objects in the sentence. The reverse mappings are
	 * stored in <code>reverseMap</code>.
	 * 
	 * @param sentence input sentence
	 * @param cos the CONTEXT objects of the question
	 * @param nes the NEs in the sentence
	 * @return sentence with CONTEXT tags
	 */
	private static String replaceContext(String sentence, String[] cos,
										 String[][] nes) {
		HashSet<String> reps = new HashSet<String>();
		String tag;
		int id = 1;
		
		for (String[] neType : nes)
			for (String ne : neType)
				for (String co : cos)
					if (StringUtils.equalsCommonNorm(ne, co)) reps.add(ne);
		for (String co : cos) {
			co = "(?i)" + RegexConverter.strToRegexWithBounds(co);
			Matcher m = Pattern.compile(co).matcher(sentence);
			while (m.find()) reps.add(m.group(0));  // get proper case
		}
		
		// sort expressions by length
		String[] sorted = reps.toArray(new String[reps.size()]);
		StringUtils.sortByLengthDesc(sorted);
		
		for (String rep : sorted) {
			tag = "<CO_" + id++ + ">";  // add unique tag ID
			reverseMap.put(tag, rep);  // remember reverse mapping
			rep = RegexConverter.strToRegexWithBounds(rep);
			sentence = sentence.replaceAll(rep, tag);
		}
		
		return sentence;
	}
	
	/**
	 * Replaces all NEs in the sentence. The reverse mappings are stored in
	 * <code>reverseMap</code>.
	 * 
	 * @param sentence input sentence
	 * @param nes the NEs in the sentence
	 * @return sentence with NE tags
	 */
	private static String replaceNes(String sentence, String[][] nes) {
		Hashtable<String, String> reps = new Hashtable<String, String>();
		String neType, tag;
		int id = 1;
		
		for (int i = 0; i < nes.length; i++) {
			neType = NETagger.getNeType(i);
			
			for (String ne : nes[i]) {
				tag = reps.get(ne);
				
				if (tag == null) tag = "<" + neType;
				else if (!tag.contains(neType))	tag += "_" + neType;
				
				reps.put(ne, tag);
			}
		}
		
		// sort expressions by length
		String[] sorted = reps.keySet().toArray(new String[reps.size()]);
		StringUtils.sortByLengthDesc(sorted);
		
		for (String rep : sorted) {
			tag = reps.get(rep) + "_" + id++ + ">";  // add unique tag ID
			reverseMap.put(tag, rep);  // remember reverse mapping
			rep = RegexConverter.strToRegexWithBounds(rep);
			sentence = sentence.replaceAll(rep, tag);
		}
		
		return sentence;
	}
	
	/**
	 * Prepares a sentence for answer extraction.
	 * 
	 * @param sentence input sentence
	 * @param to the TARGET object of the question
	 * @param cos the CONTEXT objects of the question
	 * @param nes the NEs in the sentence
	 * @return sentence ready for answer extraction or <code>null</code>, if
	 * 		   there is no TARGET object in the input sentence
	 */
	private static String prepSentence(String sentence, String to, String[] cos,
									   String[][] nes) {
		// initialize reverse map
		reverseMap = new Hashtable<String, String>();
		
		// replace TARGET and CONTEXT objects and NEs
		sentence = replaceTarget(sentence, to, nes);
		if (sentence == null) return null;
		sentence = replaceContext(sentence, cos, nes);
		sentence = replaceNes(sentence, nes);
		
		// add '#' at beginning and end of sentence
		sentence = "# " + sentence + " #";
		
		return sentence;
	}
	
	/**
	 * Gets the NE types that a PROPERTY object has in common with the answer
	 * pattern used to extract it.
	 * 
	 * @param pos PROPERTY object
	 * @param pattern answer pattern used to extract it
	 * @return NE types or <code>null</code> if the answer pattern does not
	 * 		   expect specific types
	 */
	private static String[] getNeTypes(String pos, AnswerPattern pattern) {
		ArrayList<String> neTypes = new ArrayList<String>();
		
		String[] propertyTypes = pattern.getPropertyTypes();
		if (propertyTypes == null) return null;
		
		for (String propertyType : propertyTypes)
			if (pos.contains(propertyType)) 
				neTypes.add(propertyType);
		
		return neTypes.toArray(new String[neTypes.size()]);
	}
	
	/**
	 * Replaces tags in an extracted PROPERTY object with the original strings
	 * stored in <code>reverseMap</code>.
	 * 
	 * @param po PROPERTY object
	 * @return PROPERTY object without tags
	 */
	private static String replaceTags(String po) {
		Pattern p = Pattern.compile("<(TO|CO|NE).*?>");
		Matcher m = p.matcher(po);
		
		while (m.find()) {
			String tag = m.group(0);
			String rep = reverseMap.get(tag);  // look up replacement
			if (rep != null) po = po.replace(tag, rep);
		}
		
		return po;
	}
	
	/**
	 * Applies the answer patterns to the answer string of the
	 * <code>Result</code> object to extract PROPERTY objects.
	 * 
	 * @param result a <code>Result</code> object
	 */
	private static void extractPos(Result result) {
		extr = new ArrayList<String>();
		types = new ArrayList<String[]>();
		sents = new ArrayList<String>();
		aps = new ArrayList<AnswerPattern>();
		
		// get interpretation and answer string
		QuestionInterpretation qi = result.getQuery().getInterpretation();
		String to = qi.getTarget();
//		String[] cos = qi.getContext();
		String[] cos = new String[0];  // CONTEXT objects are ignored
		String prop = qi.getProperty();
		String answer = result.getAnswer();
		// get answer patterns
		HashSet<AnswerPattern> patterns = props.get(prop);
		if (patterns == null) return;
		
		// tokenize interpretation
		to = NETagger.tokenizeWithSpaces(to);
		for (int i = 0; i < cos.length; i++)
			cos[i] = NETagger.tokenizeWithSpaces(cos[i]);
		// split answer string into sentences and tokenize sentences
		String[] originalSentences = OpenNLP.sentDetect(answer);
		String[][] tokens = new String[originalSentences.length][];
		String[] sentences = new String[originalSentences.length];
		for (int i = 0; i < originalSentences.length; i++) {
			tokens[i] = NETagger.tokenize(originalSentences[i]);
			sentences[i] = StringUtils.concatWithSpaces(tokens[i]);
		}
                
             /*   PrintWriter pw = null;   
                try {
                    pw = new PrintWriter(new FileOutputStream(new File("netagger_data.txt"),true));
                } catch (FileNotFoundException ex) {
                    System.out.println("File not found exception!!");
                }*/
                
		// extract named entities
		String[][][] nes = NETagger.extractNes(tokens);
                
               // pw.printf("%s ----- %s\n", tokens.toString(), nes.toString());
                
                /*PrintWriter pw2 = null;   
                try {
                    pw2 = new PrintWriter(new FileOutputStream(new File("regex_data.txt"),true));
                } catch (FileNotFoundException ex) {
                    System.out.println("File not found exception!!");
                }*/
		
		for (int i = 0; i < sentences.length; i++) {
			// prepare sentence for answer extraction
			sentences[i] = prepSentence(sentences[i], to, cos, nes[i]);
			if (sentences[i] == null) continue;
			
			for (AnswerPattern pattern : patterns) {
				// apply answer pattern
				String[] pos = pattern.apply(sentences[i]);
                                
                              //  pw2.printf("%s ----- %s ----- %s\n", pattern.getDesc(), sentences[i], pos);
				
				// get NE types of PROPERTY objects
				String[][] neTypes = new String[pos.length][];
				for (int j = 0; j < pos.length; j++)
					neTypes[j] = getNeTypes(pos[j], pattern);
				
				// replace tags and untokenize PROPERTY objects
				for (int j = 0; j < pos.length; j++) {
					pos[j] = replaceTags(pos[j]);
					pos[j] = OpenNLP.untokenize(pos[j], originalSentences[i]);
				}
				
				// store the PROPERTY objects, the sentences they were extracted
				// from, the patterns used to extract them and the NE types
				for (int j = 0; j < pos.length; j++) {
					extr.add(pos[j]);
					types.add(neTypes[j]);
					sents.add(originalSentences[i]);
					aps.add(pattern);
				}
			}
		}
             //   pw.close();
           //     pw2.close();
	}
	
	/**
	 * Adds an answer pattern for a specific PROPERTY.
	 * 
	 * @param expr pattern descriptor
	 * @param prop PROPERTY that the pattern extracts
	 * @return true, iff a new pattern was added
	 */
	public static boolean addPattern(String expr, String prop) {
		// get the answer patterns for the specified PROPERTY
		HashSet<AnswerPattern> patterns = props.get(prop);
		if (patterns == null) {  // new PROPERTY
			patterns = new HashSet<AnswerPattern>();
			props.put(prop, patterns);
			nOfPassages.put(prop, 0);
		}
		
		// if the pattern is not in the set, add it
		boolean added = patterns.add(new AnswerPattern(expr, prop));
		
		// print out new patterns
		if (added) MsgPrinter.printStatusMsg(prop + ": " + expr);
		
		return added;
	}
	
	/**
	 * Loads the answer patterns from a directory of PROPERTY files. The first
	 * line of each file is the total number of passages used to assess the
	 * patterns. It is followed by a list of pattern descriptors, along with
	 * their number of correct and wrong applications. The format of the
	 * descriptors is described in the documentation of the class
	 * <code>AnswerPattern</code>.
	 * 
	 * @param dir directory of the answer patterns
	 * @return true, iff the answer patterns were loaded successfully
	 */
	public static boolean loadPatterns(String dir) {
		File[] files = FileUtils.getFiles(dir);
		
		try {
			BufferedReader in;
			String prop, expr;
			int passages, correct, wrong;
			HashSet<AnswerPattern> patterns;
			
			for (File file : files) {
				MsgPrinter.printStatusMsg("  ...for " + file.getName());
				
				prop = file.getName();
				in = new BufferedReader(new FileReader(file));
				
				// total number of passages used to assess the patterns
				passages = Integer.parseInt(in.readLine().split(" ")[1]);
				nOfPassages.put(prop, passages);
				
				patterns = new HashSet<AnswerPattern>();
				while (in.ready()) {
					in.readLine();
					// pattern descriptor
					expr = in.readLine();
					// number of correct applications
					correct = Integer.parseInt(in.readLine().split(" ")[1]);
					// number of wrong applications
					wrong = Integer.parseInt(in.readLine().split(" ")[1]);
					
					try {
						patterns.add(new AnswerPattern(expr, prop,
													   correct, wrong));
					} catch (PatternSyntaxException pse) {
						MsgPrinter.printErrorMsg("Problem loading pattern:\n" +
												 prop + " " + expr);
						MsgPrinter.printErrorMsg(pse.getMessage());
					}
				}
				props.put(prop, patterns);
				
				in.close();
			}
			
			MsgPrinter.printStatusMsg("  ...done");
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Saves the answer patterns to resource files. A separate file is created
	 * for each PROPERTY. The first line is the total number of passages used to
	 * assess the answer patterns. It is followed by a list of pattern
	 * descriptors along with their number of correct and wrong applications.
	 * 
	 * @param dir directory of the answer patterns
	 * @return true, iff the answer patterns were saved successfully
	 */
	public static boolean savePatterns(String dir) {
		File file;
		PrintWriter out;
		
		try {
			for (String prop : props.keySet()) {
				// sort answer patterns
				HashSet<AnswerPattern> ps = props.get(prop);
				AnswerPattern[] patterns =
					ps.toArray(new AnswerPattern[ps.size()]);
				Arrays.sort(patterns);
				
				file = new File(dir + "/" + prop);
				out = new PrintWriter(file, "UTF-8");
				
				// total number of passages used to assess the patterns
				out.println("#passages: " + nOfPassages.get(prop));
				
				for (int i = patterns.length - 1; i >= 0; i--) {
					out.println();
					// pattern descriptor
					out.println(patterns[i].getDesc());
					// number of correct applications
					out.println("#correct: " + patterns[i].getCorrect());
					// number of wrong applications
					out.println("#incorrect: " + patterns[i].getWrong());
				}
				
				out.close();
			}
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Assesses the answer patterns by applying them to the answer string of the
	 * <code>Result</code> object and comparing the extracted answers to the
	 * regular expression <cdoe>regex</code>.
	 * 
	 * @param result <code>Result</code> object
	 * @param regex regular expression that describes a correct answer
	 */
	public static void assessPatterns(Result result, String regex) {
		// increment the number of passages used to assess the patterns
		String prop = result.getQuery().getInterpretation().getProperty();
		int n = (nOfPassages.get(prop) == null) ? 1 : nOfPassages.get(prop) + 1;
		nOfPassages.put(prop, n);
		
		// extract PROPERTY objects
		extractPos(result);
		
		// use the regular expression regex to assess the PROPERTY objects
		for (int i = 0; i < extr.size(); i++) {
			String po = extr.get(i);
			AnswerPattern ap = aps.get(i);
			
			if (po.matches("(?i)" + regex)) ap.incCorrect();
			else ap.incWrong();
		}
	}
	
	/**
	 * Drops answer patterns that have a support of <code>supportThresh</code>
	 * or less.
	 * 
	 * @param supportThresh the support threshold
	 */
	public static void dropLowSupport(float supportThresh) {
		// for each PROPERTY
		for (String prop : props.keySet()) {
			HashSet<AnswerPattern> patterns = props.get(prop);
			int n = nOfPassages.get(prop);
			HashSet<AnswerPattern> remaining = new HashSet<AnswerPattern>();
			
			// check the threshold for each answer pattern
			for (AnswerPattern pattern : patterns)
				if ((pattern.getCorrect() / (float) n) >= supportThresh)
					remaining.add(pattern);
			
			props.put(prop, remaining);
		}
	}
	
	/**
	 * Drops answer patterns that have a confidence of
	 * <code>confidenceThresh</code> or less.
	 * 
	 * @param confidenceThresh the confidence threshold
	 */
	public static void dropLowConfidence(float confidenceThresh) {
		// for each PROPERTY
		for (String prop : props.keySet()) {
			HashSet<AnswerPattern> patterns = props.get(prop);
			HashSet<AnswerPattern> remaining = new HashSet<AnswerPattern>();
			
			// check the threshold for each answer pattern
			for (AnswerPattern pattern : patterns)
				if (pattern.getConfidence() >= confidenceThresh)
					remaining.add(pattern);
			
			props.put(prop, remaining);
		}
	}
	
	/**
	 * Applies the answer patterns to the answer strings of the
	 * <code>Result</code> objects and creates a new <code>Result</code> for
	 * each extracted unique answer.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// extracted factoid answers and corresponding results
		Hashtable<String, Result> factoids = new Hashtable<String, Result>();
		
		for (Result result : results) {
			// only apply this filter to results for the pattern matching
			// approach
			Query query = result.getQuery();
			QuestionInterpretation qi = query.getInterpretation();
			if (!query.extractWith(ID) ||
					qi == null ||
					result.getScore() > Float.NEGATIVE_INFINITY)
				continue;
			
			// extract PROPERTY objects
			extractPos(result);
			
			// create new result for each unique normalized PROPERTY object
			for (int i = 0; i < extr.size(); i++) {
				String po = extr.get(i);
				String[] neTypes = types.get(i);
				String norm = StringUtils.normalize(po);
				String sentence = sents.get(i);
				float conf = aps.get(i).getConfidence();
				
				Result factoid = factoids.get(norm);
				if (factoid == null) {  // new answer
					// query, doc ID and sentence can be ambiguous
					factoid = new Result(po, result.getQuery(),
										 result.getDocID());
					factoid.setSentence(sentence);
					factoid.addExtractionTechnique(ID);
					factoids.put(norm, factoid);
				}
				if (neTypes != null)
					for (String neType : neTypes) factoid.addNeType(neType);
				factoid.incScore(conf);
			}
		}
		
		// keep old results
		Result[] newResults =
			factoids.values().toArray(new Result[factoids.size()]);
		Result[] allResults = new Result[results.length + newResults.length];
		for (int i = 0; i < results.length; i++)
			allResults[i] = results[i];
		for (int i = 0; i < newResults.length; i++)
			allResults[results.length + i] = newResults[i];
		
		return allResults;
	}
}

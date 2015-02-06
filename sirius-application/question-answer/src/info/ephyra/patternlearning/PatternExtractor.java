package info.ephyra.patternlearning;

import info.ephyra.answerselection.filters.AnswerPatternFilter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.questionanalysis.QuestionInterpretation;
import info.ephyra.questionanalysis.QuestionInterpreter;
import info.ephyra.search.Result;
import info.ephyra.util.RegexConverter;
import info.ephyra.util.StringUtils;

import java.util.HashSet;
import java.util.Hashtable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Extracts answer patterns from text passages and adds them to the
 * <code>AnswerPatternFilter</code>.
 * 
 * @author Nico Schlaefer
 * @version 2006-04-04
 */
public class PatternExtractor {
	/**
	 * Maximum number of NE and CONTEXT tags in a pattern (for time
	 * performance).
	 */
	private static final int MAX_TAGS = 5;
	
	/**
	 * Replaces all TARGET objects in the sentence.
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
		String result = sentence;
		
		for (String[] neType : nes)
			for (String ne : neType)
				if (StringUtils.equalsCommonNorm(ne, to)) reps.add(ne);
		reps.add(to);
		
		// sort expressions by length
		String[] sorted = reps.toArray(new String[reps.size()]);
		StringUtils.sortByLengthDesc(sorted);
		
		for (String rep : sorted) {
			rep = RegexConverter.strToRegexWithBounds(rep);
			result = result.replaceAll(rep, "<TO>");
		}
		
		return (result.equals(sentence)) ? null : result;
	}
	
	/**
	 * Replaces all PROPERTY objects in the sentence.
	 * 
	 * @param sentence input sentence
	 * @param as the answer to the question
	 * @param nes the NEs in the sentence
	 * @return sentence with PROPERTY tags or <code>null</code>, if the sentence
	 * 		   does not contain the answer
	 */
	private static String replaceProperty(String sentence, String as,
										  String[][] nes) {
		Hashtable<String, String> reps = new Hashtable<String, String>();
		String neType, tag, result = sentence;
		
		for (int i = 0; i < nes.length ; i++){
			neType = NETagger.getNeType(i);
			
			for (String ne : nes[i])
				if (StringUtils.equalsCommonNorm(ne, as)) {
					tag = reps.get(ne);
					
					if (tag == null) tag = "<PO_" + neType;
					else if (!tag.contains(neType)) tag += "_" + neType;
					
					reps.put(ne, tag);
				}
		}
		if (!reps.containsKey(as)) reps.put(as, "<PO");
		
		// sort expressions by length
		String[] sorted = reps.keySet().toArray(new String[reps.size()]);
		StringUtils.sortByLengthDesc(sorted);
		
		for (String rep : sorted) {
			tag = reps.get(rep) + ">";
			rep = RegexConverter.strToRegexWithBounds(rep);
			result = result.replaceAll(rep, tag);
		}
		
		return (result.equals(sentence)) ? null : result;
	}
	
	/**
	 * Replaces all CONTEXT objects in the sentence.
	 * 
	 * @param sentence input sentence
	 * @param cos the CONTEXT objects of the question
	 * @param nes the NEs in the sentence
	 * @return sentence with CONTEXT tags
	 */
	private static String replaceContext(String sentence, String[] cos,
										 String[][] nes) {
		HashSet<String> reps = new HashSet<String>();
		
		for (String[] neType : nes)
			for (String ne : neType)
				for (String co : cos)
					if (StringUtils.equalsCommonNorm(ne, co)) reps.add(ne);
		for (String co : cos) reps.add(co);
		
		// sort expressions by length
		String[] sorted = reps.toArray(new String[reps.size()]);
		StringUtils.sortByLengthDesc(sorted);
		
		for (String rep : sorted) {
			rep = RegexConverter.strToRegexWithBounds(rep);
			sentence = sentence.replaceAll(rep, "<CO>");
		}
		
		return sentence;
	}
	
	/**
	 * Replaces all NEs in the sentence.
	 * 
	 * @param sentence input sentence
	 * @param nes the NEs in the sentence
	 * @return sentence with NE tags
	 */
	private static String replaceNes(String sentence, String[][] nes) {
		Hashtable<String, String> reps = new Hashtable<String, String>();
		String neType, tag;
		
		for (int i = 0; i < nes.length; i++) {
			neType = NETagger.getNeType(i);
			
			for (String ne : nes[i]) {
				tag = reps.get(ne);
				
				if (tag == null) tag = "<" + neType;
				else if (!tag.contains(neType)) tag += "_" + neType;
				
				reps.put(ne, tag);
			}
		}
		
		// sort expressions by length
		String[] sorted = reps.keySet().toArray(new String[reps.size()]);
		StringUtils.sortByLengthDesc(sorted);
		
		for (String rep : sorted) {
			tag = reps.get(rep) + ">";
			rep = RegexConverter.strToRegexWithBounds(rep);
			sentence = sentence.replaceAll(rep, tag);
		}
		
		return sentence;
	}
	
	/**
	 * Prepares a sentence for pattern extraction.
	 * 
	 * @param sentence input sentence
	 * @param to the TARGET object of the question
	 * @param cos the CONTEXT objects of the question
	 * @param po the answer to the question
	 * @param nes the NEs in the sentence
	 * @return sentence ready for pattern extraction or <code>null</code>, if
	 * 		   there is no TARGET or PROPERTY object in the input sentence
	 */
	private static String prepSentence(String sentence, String to, String[] cos,
									   String po, String[][] nes) {
		// replace TARGET, PROPERTY and CONTEXT objects and NEs
		sentence = replaceTarget(sentence, to, nes);
		if (sentence == null) return null;
		sentence = replaceProperty(sentence, po, nes);
		if (sentence == null) return null;
		sentence = replaceContext(sentence, cos, nes);
		sentence = replaceNes(sentence, nes);
		
		// add '#' at beginning and end of sentence
		sentence = "# " + sentence + " #";
		
		// transform into regular expression
		sentence = RegexConverter.strToRegex(sentence);
		
		return sentence;
	}
	
	/**
	 * Extract basic answer patterns from the sentence.
	 * 
	 * @param sentence input sentence
	 * @return basic answer patterns
	 */
	private static String[] extractPatterns(String sentence) {
		String[] tokens = sentence.split(" ");
		HashSet<String> patterns = new HashSet<String>();
		
		// TARGET comes before PROPERTY
		String ap = "";
		for (int i = 0; i < tokens.length; i++) {
			if (tokens[i].equals("<TO>")) {
				ap = tokens[i];
			} else if (ap.length() > 0) {
				ap += " " + tokens[i];  // add to pattern
				
				if (tokens[i].matches("<PO.*>")) {
					ap += " " + tokens[i + 1];

					if (ap.split("<TO>", -1).length == 2 &&
						ap.split("<PO.*?>", -1).length == 2)
						// exactly one TARGET and PROPERTY tag
						patterns.add(ap);
					
					ap = "";
				}
			}
		}
		
		// PROPERTY comes before TARGET
		ap = "";
		for (int i = 0; i < tokens.length; i++) {
			if (tokens[i].matches("<PO.*>")) {
				ap = tokens[i - 1] + " " + tokens[i];
			} else if (ap.length() > 0) {
				ap += " " + tokens[i];  // add to pattern
				
				if (tokens[i].equals("<TO>")) {
					if (ap.split("<TO>", -1).length == 2 &&
						ap.split("<PO.*?>", -1).length == 2)
						// exactly one TARGET and PROPERTY tag
						patterns.add(ap);
					
					ap = "";
				}
			}
		}
		
		return patterns.toArray(new String[patterns.size()]);
	}
	
	/**
	 * Generates more generic patterns from the initial patterns.
	 * 
	 * @param patterns initial patterns
	 * @param prop PROPERTY that the patterns extract
	 * @return more generic patterns
	 */
	private static String[] generalizePatterns(String[] patterns, String prop) {
		HashSet<String> gens = new HashSet<String>();
		
		// if the PROPERTY tag is combined with NE types, replace the pattern
		// by applying the following generalizations:
		// - drop the token preceding/following the PROPERTY tag
		// - drop the NE types
		Pattern p = Pattern.compile("(<TO>.*?<PO_.*?>)|(<PO_.*?>.*?<TO>)");
		for (String pattern : patterns) {
			Matcher m = p.matcher(pattern);
			if (m.find()) {
				gens.add(m.group(0));
				gens.add(pattern.replaceFirst("<PO_.*?>", "<PO>"));
			} else gens.add(pattern);
		}
		
		// drop all tokens in between the TARGET and PROPERTY tag that are not
		// keywords or tags and that are not adjacent to a PROPERTY tag without
		// NE types and make tags optional
		patterns = gens.toArray(new String[gens.size()]);
		for (String pattern : patterns) {
			String[] tokens = pattern.split(" ");
			String gen = "";
			int nOfTags = 0;
			boolean dropped = false;  // true, iff last token was dropped
//			boolean keywords = false;  // true, iff pattern contains keywords
			
			for (int i = 0; i < tokens.length; i++) {
				if (tokens[i].matches("<TO>") ||
					tokens[i].matches("<PO.*>") ||
					(i > 0 && tokens[i - 1].matches("<PO>")) ||
					(i < tokens.length - 1 && tokens[i + 1].matches("<PO>"))) {
					// keep TARGET and PROPERTY tags and tokens that are
					// adjacent to a PROPERTY tag without NE types
					gen += tokens[i] + " ";
					dropped = false;
				} else if (tokens[i].matches("<.*>")) {
					// make tags optional
					gen += "(?:" + tokens[i] + " )?";  // greedy
					nOfTags++;
					dropped = false;
				} else if (QuestionInterpreter.lookupKeyword(tokens[i], prop)) {
					// keep keywords
					gen += tokens[i] + " ";
					dropped = false;
//					keywords = true;
				} else {
					// drop other tokens
					if (!dropped) gen += "[^<]*?";  // reluctant
					dropped = true;
				}
			}
			
//			if (keywords)  // patterns contains keywords
				if (nOfTags <= MAX_TAGS)  // at most MAX_TAGS NE or CONTEXT tags
					gens.add(gen.trim());
		}
		
		return gens.toArray(new String[gens.size()]);
	}
	
	/**
	 * Extracts answer patterns from the answer string of a <code>Result</code>
	 * object and adds them to the <code>AnswerPatternFilter</code>.
	 * 
	 * @param result <code>Result</code> object
	 * @param as the answer to the question
	 */
	public static void extract(Result result, String as) {
		// get interpretation and answer string
		QuestionInterpretation qi = result.getQuery().getInterpretation();
		String to = qi.getTarget();
//		String[] cos = qi.getContext();
		String[] cos = new String[0];  // CONTEXT objects are ignored
		String prop = qi.getProperty();
		String answer = result.getAnswer();
		
		// tokenize interpretation and provided answer, convert to lower-case
		to = NETagger.tokenizeWithSpaces(to).toLowerCase();
		for (int i = 0; i < cos.length; i++)
			cos[i] = NETagger.tokenizeWithSpaces(cos[i]).toLowerCase();
		as = NETagger.tokenizeWithSpaces(as).toLowerCase();
		// split answer string into sentences and tokenize sentences
		String[] sentences = OpenNLP.sentDetect(answer);
		String[][] tokens = new String[sentences.length][];
		for (int i = 0; i < sentences.length; i++) {
			tokens[i] = NETagger.tokenize(sentences[i]);
			sentences[i] = StringUtils.concatWithSpaces(tokens[i]);
		}
		// extract named entities
		String[][][] nes = NETagger.extractNes(tokens);
		// convert sentences and named entities to lower-case
		for (int i = 0; i < nes.length; i++) {
			sentences[i] = sentences[i].toLowerCase();
			for (int j = 0; j < nes[i].length; j++)
				for (int k = 0; k < nes[i][j].length; k++)
					nes[i][j][k] = nes[i][j][k].toLowerCase();
		}
		
		for (int i = 0; i < sentences.length; i++) {
			// prepare sentence for pattern extraction
			sentences[i] = prepSentence(sentences[i], to, cos, as, nes[i]);
			if (sentences[i] == null) continue;
			
			// extract patterns
			String[] patterns = extractPatterns(sentences[i]);
			// generalize patterns
			patterns = generalizePatterns(patterns, prop);
			
			// add patterns
			for (String pattern : patterns)
				AnswerPatternFilter.addPattern(pattern, prop);
		}
	}
}

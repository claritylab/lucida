package info.ephyra.questionanalysis;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.VerbFormConverter;
import info.ephyra.nlp.semantics.ontologies.WordNet;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javatools.PlingStemmer;

/**
 * This class provides methods that modify a question to facilitate pattern
 * matching and to anticipate the format of text passages that answer the
 * question.
 * 
 * @author Nico Schlaefer
 * @version 2006-06-18
 */
public class QuestionNormalizer {
	/**
	 * Replaces short forms of "is" and "are" that occur in combination with
	 * interrogatives.
	 * 
	 * @param question the question string
	 * @return modified question string
	 */
	private static String replaceShortForms(String question) {
		// only replace occurences of "'s" and "'re" in combination with
		// interrogatives
		Pattern p = Pattern.compile("(?i)(how|what|which|when|where|who|why)'" +
									"(s|re)");
		Matcher m = p.matcher(question);
		
		if (m.find()) {
			String original = m.group();
			
			String replaced = original.replace("'s", " is");
			replaced = replaced.replace("'re", " are");
			
			return question.replace(original, replaced);
		}
		
		return question;  // no such short forms in the question
	}
	
	/**
	 * Drops filler words from the question string.
	 * 
	 * @param question the question string
	 * @return modified question string
	 */
	private static String dropFillers(String question) {
		String fillers = "(approximate|approximately|one of|so-called)";
		
		return question.replaceAll(fillers + " ", "");
	}
	
	/**
	 * <p>Modifies the question string by applying the following rule:</p>
	 * 
	 * <p><code>is/are/was/were [...] gerund / past participle ->
	 * is/are/was/were gerund / past participle</code></p>
	 * 
	 * @param question question string
	 * @param tagged tagged question
	 * @return modified question strings
	 */
	private static String[] handleAuxIs(String question, String tagged) {
		Pattern p = Pattern.compile("(?i)(.* )?(is|are|was|were)/.*? " +
									"(\\S*)/vb(g|n).*");
		Matcher m = p.matcher(tagged);
		
		String[] results;
		if (m.matches()) {
			String aux = m.group(2);
			String verb = m.group(3);
			
			results = new String[1];
			results[0] = question.replaceFirst(verb, aux + " " + verb);
			results[0] = results[0].replaceFirst(aux + " ", "");
			
			return results;
		}
		
		return null;
	}
	
	/**
	 * <p>Modifies the question string by applying the following rule:</p>
	 * 
	 * <p><code>can/could/will/would/shall/should/may/might/must [...]
	 * infinitive -> can/could/will/would/shall/should/may/might/must
	 * infinitive</code></p>
	 * 
	 * @param question question string
	 * @param tagged tagged question
	 * @return modified question strings
	 */
	private static String[] handleAuxCanMay(String question, String tagged) {
		Pattern p = Pattern.compile("(?i)(.* )?(can|could|will|would|shall" +
									"|should|may|might|must)/.*? " +
									"(\\S*)/vb(\\W.*)?");
		Matcher m = p.matcher(tagged);
		
		String[] results;
		if (m.matches()) {
			String aux = m.group(2);
			String verb = m.group(3);
			
			results = new String[1];
			results[0] = question.replaceFirst(verb, aux + " " + verb);
			results[0] = results[0].replaceFirst(aux + " ", "");
			
			return results;
		}
		
		return null;
	}
	
	/**
	 * <p>Modifies the question string by applying the following rule:</p>
	 * 
	 * <p><code>have/has/had [...] past_participle -> has/have/had
	 * past_participle / simple_past</code></p>
	 * 
	 * @param question question string
	 * @param tagged tagged question
	 * @return modified question strings
	 */
	private static String[] handleAuxHasHad(String question, String tagged) {
		Pattern p = Pattern.compile("(?i)(.* )?(has|have|had)/.*? " +
									"(\\S*)/vbn.*");
		Matcher m = p.matcher(tagged);
		
		String[] results;
		if (m.matches()) {
			String aux = m.group(2);
			String verb = m.group(3);
			String[] sp = VerbFormConverter.pastParticipleToSimplePast(verb);
			
			results = new String[sp.length + 1];
			results[0] = question.replaceFirst(verb, aux + " " + verb);
			results[0] = results[0].replaceFirst(aux + " ", "");
			for (int i = 0; i < sp.length; i++) {
				results[i + 1] = question.replaceFirst(verb, sp[i]);
				results[i + 1] = results[i + 1].replaceFirst(aux + " ", "");
			}
			
			return results;
		}
		
		return null;
	}
	
	/**
	 * <p>Modifies the question string by applying the following rule:</p>
	 * 
	 * <p><code>do [...] infinitive -> infinitive</code></p>
	 * 
	 * @param question question string
	 * @param tagged tagged question
	 * @return modified question strings
	 */
	private static String[] handleAuxDo(String question, String tagged) {
		Pattern p = Pattern.compile("(?i)(.* )?do/.*? (\\S*)/vb(\\W.*)?");
		Matcher m = p.matcher(tagged);
		
		String[] results;
		if (m.matches()) {
			results = new String[1];
			results[0] = question.replaceFirst("do ", "");
			
			return results;
		}
		
		return null;
	}
	
	/**
	 * <p>Modifies the question string by applying the following rule:</p>
	 * 
	 * <p><code>does [...] infinitive -> 3rd person singular</code></p>
	 * 
	 * @param question question string
	 * @param tagged tagged question
	 * @return modified question strings
	 */
	private static String[] handleAuxDoes(String question, String tagged) {
		Pattern p = Pattern.compile("(?i)(.* )?does/.*? (\\S*)/vb(\\W.*)?");
		Matcher m = p.matcher(tagged);
		
		String[] results;
		if (m.matches()) {
			String verb = m.group(2);
			String tps = VerbFormConverter.infinitiveToThirdPersonS(verb);
			
			results = new String[1];
			results[0] = question.replaceFirst(verb, tps);
			results[0] = results[0].replaceFirst("does ", "");
			
			return results;
		}
		
		return null;
	}
	
	/**
	 * <p>Modifies the question string by applying the following rule:</p>
	 * 
	 * <p><code>did [...] infinitive -> simple_past</code></p>
	 * 
	 * @param question question string
	 * @param tagged tagged question
	 * @return modified question strings
	 */
	private static String[] handleAuxDid(String question, String tagged) {
		Pattern p = Pattern.compile("(?i)(.* )?did/.*? (\\S*)/vb(\\W.*)?");
		Matcher m = p.matcher(tagged);
		
		String[] results;
		if (m.matches()) {
			String verb = m.group(2);
			results = VerbFormConverter.infinitiveToSimplePast(verb);
			
			for (int i = 0; i < results.length; i++) {
				results[i] = question.replace(verb, results[i]);
				results[i] = results[i].replace("did ", "");
			}
			
			return results;
		}
		
		return null;
	}
	
	/**
	 * Removes the final punctuation mark and quotation marks from the question
	 * string.
	 * 
	 * @param question the question string
	 * @return modified question string
	 */
	private static String dropPunctuationMarks(String question) {
		// drop final punctuation mark
		question = question.replaceAll("(\\.|\\?|!)$", "");
		// drop quotation marks
		return question.replaceAll("\"", "");
	}
	
//	/**
//	 * Converts the first letter of the question string to lower case.
//	 * 
//	 * @param question the question string
//	 * @return modified question string
//	 */
//	private static String lowerFirstLetter(String question) {
//		if (question.length() > 0) {
//			String upper = question.substring(0, 1);  // get first letter
//			String lower = upper.toLowerCase();
//		
//			if (!lower.equals(upper))
//				return question.replaceFirst(upper, lower);
//		}
//		
//		return question;
//	}
	
	/**
	 * Normalizes a question string by removing abundant whitespaces, replacing
	 * short forms and dropping filler words.
	 * 
	 * @param question question string
	 * @return normalized question string
	 */
	public static String normalize(String question) {
		//remove leading and trailing whitespaces
		question = question.trim();
		//replace multiple whitespaces by a single blank
		question = question.replaceAll("\\s+", " ");
		// replace short forms of "is" and "are"
		question = replaceShortForms(question);
		// drop filler words
		question = dropFillers(question);
		
		return question;
	}
	
	/**
	 * Converts the verbs to infinitive and the nouns to their singular forms.
	 * 
	 * @param qn normalized question string
	 * @return stemmed question string
	 */
	public static String stemVerbsAndNouns(String qn) {
		// tokenize, tag POS and convert to lower case
		String[] tokens = OpenNLP.tokenize(qn);
		String[] pos = OpenNLP.tagPos(tokens);
		
		qn = qn.toLowerCase();
		for (int i = 0; i < tokens.length; i++)
			tokens[i] = tokens[i].toLowerCase();
		
		for (int i = 0; i < tokens.length; i++) {
			if (pos[i].startsWith("VB")) {
				String rep = WordNet.getLemma(tokens[i], WordNet.VERB);
				if (rep == null) rep = tokens[i];
				qn = qn.replace(tokens[i], rep);
			} else if (pos[i].startsWith("NN")) {
				String rep = PlingStemmer.stem(tokens[i]);
				qn = qn.replace(tokens[i], rep);
			}
		}
		
		// drop final punctuation mark and quotation marks
		qn = dropPunctuationMarks(qn);
		// convert the first letter to lower case
//		qn = lowerFirstLetter(qn);
		
		return qn;
	}
	
	/**
	 * Unstems a substring of the stemmed question string by mapping it to the
	 * normalized question string.
	 * 
	 * @param sub a substring of the stemmed question string
	 * @param stemmed the stemmed question string
	 * @param qn the normalized question string
	 * @return unstemmed string or <code>sub</code>, if it is not a substring of
	 * 		   <code>stemmed</code>
	 */
	public static String unstem(String sub, String stemmed, String qn) {
		String result = sub;
		
		// preprocess the normalized question string
		// - drop final punctuation mark and quotation marks
		qn = dropPunctuationMarks(qn);
		// - convert the first letter to lower case
//		qn = lowerFirstLetter(qn);
		
		String[] truncs = stemmed.split(sub, -1);
		if (truncs.length > 1) {  // substring occurs in stemmed string?
			int start = NETagger.tokenize(truncs[0]).length;
			int end = start + NETagger.tokenize(sub).length;
			String[] tokens = NETagger.tokenize(qn);
			
			result = tokens[start];
			for (int i = start + 1; i < end; i++) result += " " + tokens[i];
			
			result = OpenNLP.untokenize(result, qn);
		}
		
		return result;
	}
	
	/**
	 * <p>Handles auxiliary verbs by applying the rules specified in the
	 * documentations of the <code>handleAux...()</code> methods.</p>
	 * 
	 * @param qn normalized question string
	 * @return question strings with modified verbs
	 */
	public static String[] handleAuxiliaries(String qn) {
		String[] results = {qn};
		
		// tokenize, tag POS and convert to lower case
		String tokens = OpenNLP.tokenizeWithSpaces(qn);
		String tagged = OpenNLP.tagPos(tokens).toLowerCase();
		
		// is/are/was/were [...] gerund / past participle ->
		// is/are/was/were gerund / past participle
		results = handleAuxIs(qn, tagged);
		
		// can/could/will/would/shall/should/may/might/must [...] infinitive ->
		// can/could/will/would/shall/should/may/might/must infinitive
		if (results == null) results = handleAuxCanMay(qn, tagged);
			
		// have/has/had [...] past_participle ->
		// has/have/had past_participle / simple_past
		if (results == null) results = handleAuxHasHad(qn, tagged);
		
		// do [...] infinitive -> infinitive
		if (results == null) results = handleAuxDo(qn, tagged);
		
		// does [...] infinitive -> infinitive + "s"
		if (results == null) results = handleAuxDoes(qn, tagged);
		
		// did [...] infinitive -> simple_past
		if (results == null) results = handleAuxDid(qn, tagged);
		
		// none of the above rules applies
		if (results == null) results = new String[] {qn};
		
		for (int i = 0; i < results.length; i++) {
			// drop final punctuation mark and quotation marks
			results[i] = dropPunctuationMarks(results[i]);
			// convert the first letter to lower case
//			results[i] = lowerFirstLetter(results[i]);
		}
		
		return results;
	}
	
	/**
	 * Replaces certain expressions in a list question to transform it into a
	 * factoid question.
	 * 
	 * @param question a list question
	 * @return transformed question
	 */
	public static String transformList(String question) {
		question = normalize(question);
		
		String listPattern = ("(?i)^") +
			"(name|(what|which|who)( (is|are|was|were))?|list|give|provide|identify) " +
			"((a list of )?((the )?names of )?(all|every|a few|more|(the )?other|(the )?several|some( of)?|(the )?various) )?";
		Matcher m = Pattern.compile(listPattern).matcher(question);
		if (m.find()) {
			String match = m.group(0);
			
			String rep = m.group(1);
			if (rep.matches("(?i)(list|give|provide|identify)")) rep = "name";
			
			question = question.replaceFirst(match, rep + " ");
		}
		
		return question;
	}
}

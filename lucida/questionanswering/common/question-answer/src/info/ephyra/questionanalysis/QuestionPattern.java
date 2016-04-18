package info.ephyra.questionanalysis;

import info.ephyra.nlp.OpenNLP;

import java.util.ArrayList;
import java.util.Hashtable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A <code>QuestionPattern</code> is applied to a question to determine the
 * TARGET of the question, CONTEXT information and the PROPERTY the question
 * asks for.
 * 
 * @author Nico Schlaefer
 * @version 2006-04-20
 */
public class QuestionPattern {
	/** Maximum length of a TARGET object in tokens. */
	private static final int MAX_TARGET = 10;
	/** Maximum length of a CONTEXT object in tokens. */
	private static final int MAX_CONTEXT = 10;
	/** The <code>Pattern</code> that is applied to a question string. */
	private Pattern pattern;
	/** The PROPERTY that a question which matches this pattern asks for. */
	private String property;
	/** ID of the group that represents the TARGET of the question. */
	private int targetID;
	/** IDs of 0 to n groups that represent the CONTEXT of the question. */
	private int[] contextIDs;
	
	/**
	 * Creates a <code>QuestionPattern</code> from a descriptor that is a
	 * regular expression but additionally contains the following tags:
	 * <ul>
	 * <li>&lt;TO&gt; - exactly one TARGET tag</li>
	 * <li>&lt;CO&gt; - an arbitrary number of CONTEXT tags</li>
	 * </ul>
	 * 
	 * @param expr pattern descriptor
	 * @param prop PROPERTY that a question which matches the pattern asks for
	 */
	public QuestionPattern(String expr, String prop) {
		property = prop;  // PROPERTY that this pattern extracts
		
		// add ".*?" at the beginning of the expression
		expr = ".*?" + expr;  // reluctant
		// add ".*+" at the end of the expression, if it does not end with a
		// TARGET/CONTEXT object
//		if (!expr.matches(".*<(TO|CO)>(\\(.*?\\)\\?)?$"))
//			expr += ".*+";  // possessive
		
		// replace tags
		expr = replaceTargetTag(expr);
		expr = replaceContextTags(expr);
		
		// compile regular expression (case insensitive)
		pattern = Pattern.compile(expr, Pattern.CASE_INSENSITIVE);
	}
	
	/**
	 * Sets the <code>targetID</code> field and replaces the TARGET tag by a
	 * capturing group.
	 * 
	 * @param expr pattern descriptor
	 * @return descriptor without TARGET tag
	 */
	private String replaceTargetTag(String expr) {
		// compute the ID of the group that represents the TARGET object
		// - get string before TARGET tag
		String s = expr.split("<TO>")[0];
		// - count number of '(' not preceded by '\' or followed by '?:' and
		//	 number of CONTEXT tags
		targetID = s.split("(\\(|<CO>)", -1).length -
				   s.split("\\\\\\(", -1).length -
				   s.split("\\(\\?\\:").length +
				   s.split("\\\\\\(\\?\\:").length + 1;
		
		// replace TARGET tag
		expr = expr.replace("<TO>", "(.*?)");  // reluctant
		
		return expr;
	}
	
	/**
	 * Sets the <code>contextIDs</code> field and replaces the CONTEXT tags by
	 * capturing groups.
	 * 
	 * @param expr pattern descriptor
	 * @return descriptor without CONTEXT tags
	 */
	private String replaceContextTags(String expr) {
		// compute the IDs of the groups that represent the CONTEXT objects
		// - get strings between CONTEXT tags
		String[] ss = expr.split("<CO>", -1);
		contextIDs = new int[ss.length - 1];
		for (int i = 0; i < contextIDs.length; i++)
			// - count number of '(' not preceded by '\' or followed by '?:'
			contextIDs[i] = ss[i].split("\\(", -1).length -
							ss[i].split("\\\\\\(", -1).length -
							ss[i].split("\\(\\?\\:").length +
							ss[i].split("\\\\\\(\\?\\:").length +
							((i > 0) ? contextIDs[i - 1] + 1 : 1);
		
		// replace CONTEXT tags
		expr = expr.replace("<CO>", "(.*?)");  // reluctant
		
		return expr;
	}
	
	/**
	 * Ensures that the TARGET and CONTEXT objects are noun phrases and splits
	 * the objects along prepositions and punctuation marks.
	 * 
	 * @param qn normalized question string
	 * @param qi question interpretation
	 * @return modified question interpretation
	 */
	private QuestionInterpretation ensureNounPhrases(String qn,
			QuestionInterpretation qi) {
		// tag phrase chunks
		String[] tokens = OpenNLP.tokenize(qn);
		String[] pos = OpenNLP.tagPos(tokens);
		String[] chunks = OpenNLP.tagChunks(tokens, pos);
		Hashtable<String, String> tagTable = new Hashtable<String, String>();
		for (int i = 0; i < tokens.length; i++)
			tagTable.put(tokens[i], chunks[i]);
		
		// get TARGET and CONTEXT objects
		String target = qi.getTarget();
//		String[] context = qi.getContext();
		ArrayList<String> objects = new ArrayList<String>();
		objects.add(target);
//		for (String co : context) objects.add(co);
		
		ArrayList<String> newObjects = new ArrayList<String>();
		for (int i = 0; i < objects.size(); i++) {
			tokens = OpenNLP.tokenize(objects.get(i));
			
			// ensure that the object is a noun phrase
			if ((tagTable.containsKey(tokens[0]) &&
				!tagTable.get(tokens[0]).contains("NP")) ||
				(tagTable.containsKey(tokens[tokens.length - 1]) &&
				!tagTable.get(tokens[tokens.length - 1]).contains("NP")))
				return null;
			
			// split object along prepositions and punctuation marks
//			String delims = "(";
//			for (int j = 1; j < tokens.length - 1; j++) {
//				if (tagTable.containsKey(tokens[j]) &&
//					tagTable.containsKey(tokens[j - 1]) &&
//					tagTable.containsKey(tokens[j + 1]) &&
//					(tagTable.get(tokens[j]).equals("B-PP") ||
//					tagTable.get(tokens[j]).equals("O")) &&
//					tagTable.get(tokens[j - 1]).contains("NP") &&
//					tagTable.get(tokens[j + 1]).contains("NP")) {
//					if (delims.length() > 1) delims += "|";
//					delims += RegexConverter.strToRegexWithBounds(tokens[j]);
//				}
//			}
//			if (delims.length() > 1) {
//				delims += ")";
//				String[] subObjects = objects.get(i).split(delims);
//				for (String subObject : subObjects)
//					newObjects.add(subObject.trim());
//			} else
				newObjects.add(objects.get(i));
		}
		
		// update interpretation
		qi.setTarget(newObjects.get(0));
//		newObjects.remove(0);
//		qi.setContext(newObjects.toArray(new String[newObjects.size()]));
		
		return qi;
	}
	
	/**
	 * Formats the extracted TARGET and CONTEXT objects.
	 * 
	 * @param object TARGET or CONTEXT object
	 * @return formatted object
	 */
	private String formatObject(String object) {
		// drop preceding "a", "an", "the" and trim
		return object.replaceFirst("(?i)^(an?|the) ", "").trim();
	}
	
	/**
	 * Returns the PROPERTY that a question which matches the pattern asks for.
	 * 
	 * @return the PROPERTY
	 */
	public String getProperty() {
		return property;
	}
	
	/**
	 * Applies the pattern to a question. If the question matches the pattern,
	 * a <code>QuestionInterpretation</code> is returned, else
	 * <code>null</code>.
	 * 
	 * @param qn normalized question string
	 * @param stemmed stemmed question string
	 * @return interpretation of the question or <code>null</code>, if the
	 * 		   question does not match the pattern
	 */
	public QuestionInterpretation apply(String qn, String stemmed) {
		Matcher m = pattern.matcher(stemmed);
		
		if (m.matches()) {
			String target = m.group(targetID);
			if (target.length() == 0) return null;
			target = QuestionNormalizer.unstem(target, stemmed, qn);
			String[] context = new String[contextIDs.length];
			for (int i = 0; i < context.length; i++) {
				context[i] = m.group(contextIDs[i]);
				if (context[i].length() == 0) return null;
				context[i] = QuestionNormalizer.unstem(context[i], stemmed, qn);
			}
			
			// make sure that TARGET and CONTEXT objects are noun phrases
			QuestionInterpretation qi =
				new QuestionInterpretation(target, context, property);
			qi = ensureNounPhrases(qn, qi);
			if (qi == null) return null;
			
			target = qi.getTarget();
			target = formatObject(target);
			if (target.length() == 0 ||
				target.split(" ").length > MAX_TARGET)
				return null;
			qi.setTarget(target);
			context = qi.getContext();
			for (int i = 0; i < context.length; i++) {
				context[i] = formatObject(context[i]);
				if (context[i].length() == 0 ||
					context[i].split(" ").length > MAX_CONTEXT)
					return null;
			}
			qi.setContext(context);
			
			return qi;
		} else
			return null;  // question does not match the pattern
	}
}

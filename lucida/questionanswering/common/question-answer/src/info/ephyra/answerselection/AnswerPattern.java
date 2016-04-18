package info.ephyra.answerselection;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>An <code>AnswerPattern</code> is applied to a sentence to extract a
 * PROPERTY object of the type specified in the <code>property</code> field.
 * The sentence must contain a TARGET tag to indicate the object of which the
 * PROPERTY is wanted.</p>
 * 
 * <p>This class implements the interface <code>Comparable</code>. Note: it has
 * a natural ordering that is inconsistent with <code>equals()</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-01-29
 */
public class AnswerPattern implements Comparable<AnswerPattern> {
	/** Maximum distance between TARGET and PROPERTY in tokens. */
	private static final int MAX_DIST = 20;
	/** Maximum length of a PROPERTY object in tokens. */
	private static final int MAX_PROP = 10;
	/** The pattern descriptor from which the pattern is built. */
	private String desc;
	/** The <code>Pattern</code> that is applied to a sentence. */
	private Pattern pattern;
	/** The type of PROPERTY that is extracted with this pattern. */
	private String property;
	/** ID of the group that represents the PROPERTY to be extracted. */
	private int propertyID;
	/** ID of the group that covers the string between TARGET and PROPERTY. */
	private int distID;
	/** Counter for the number of correct applications of the pattern. */
	private int correct = 0;
	/** Counter for the number of wrong applications of the pattern. */
	private int wrong = 0;
	
	/**
	 * Creates an <code>AnswerPattern</code> from a descriptor that is a
	 * regular expression but additionally contains the following tags:
	 * <ul>
	 * <li>&lt;TO&gt; - exactly one TARGET tag</li>
	 * <li>&lt;CO&gt; - an arbitrary number of CONTEXT tags</li> 
	 * <li>&lt;PO(_NExyz)*&gt; - exactly one PROPERTY tag, optionally combined
	 * 	   with NE types</li>
	 * <li>&lt;NExyz(_NExyz)*&gt; - an arbitrary number of NE tags, which are
	 * 	   combinations of one or more NE types</li>
	 * </ul>
	 * 
	 * @param expr pattern descriptor
	 * @param prop PROPERTY that the pattern extracts
	 */
	public AnswerPattern(String expr, String prop) {
		desc = expr;  // pattern descriptor
		property = prop;  // PROPERTY that this pattern extracts
		
		// add group that covers the string between TARGET and PROPERTY
		expr = addDistGroup(expr);
		
		// replace tags
		expr = replaceTargetTag(expr);
		expr = replaceContextTags(expr);
		expr = replacePropertyTag(expr);
		expr = replaceNeTags(expr);
		
		// optimize pattern
		expr = optimizePattern(expr);
		
		// compile regular expression (case insensitive)
		pattern = Pattern.compile(expr, Pattern.CASE_INSENSITIVE);
	}
	
	/**
	 * <p>Creates an <code>AnswerPattern</code> from a descriptor by applying
	 * the constructor <code>AnswerPattern(String expr, String prop)</code>.</p>
	 * 
	 * <p>In addition, it sets the counters for the number of correct/wrong
	 * applications of the pattern.</p>
	 * 
	 * @param expr pattern descriptor
	 * @param prop PROPERTY that the pattern extracts
	 * @param correct number of correct applications
	 * @param wrong number of wrong applications
	 */
	public AnswerPattern(String expr, String prop, int correct, int wrong) {
		this(expr, prop);
		
		this.correct = correct;
		this.wrong = wrong;
	}
	
	/**
	 * Adds a capturing group that covers the string between the TARGET and
	 * the PROPERTY and sets the <code>distID</code> field. Required to measure
	 * the distance between TARGET and PROPERTY.
	 * 
	 * @param expr pattern descriptor
	 * @return descriptor with capturing group
	 */
	private String addDistGroup(String expr) {
		Matcher m = Pattern.compile("<PO.*?>").matcher(expr);
		m.find();
		String pTag = m.group(0);
		
		if (expr.startsWith("<TO>")) {  // TARGET comes before PROPERTY
			distID = 1;
			return expr.replace("<TO>", "<TO>(").replace(pTag, ")" + pTag);
		} else {  // PROPERTY comes before TARGET
			distID = 2;
			return expr.replace(pTag, pTag + "(").replace("<TO>", ")<TO>");
		}
	}
	
	/**
	 * Replaces the TARGET tag by a regular expression that matches TARGET tags
	 * with tag IDs.
	 * 
	 * @param expr pattern descriptor
	 * @return descriptor with a regular expression for TARGET tags
	 */
	private String replaceTargetTag(String expr) {
		return expr.replace("<TO>", "<TO_\\d*+>");  // possessive
	}
	
	/**
	 * Replaces CONTEXT tags by regular expressions that match CONTEXT tags with
	 * tag IDs.
	 * 
	 * @param expr pattern descriptor
	 * @return descriptor with regular expressions for CONTEXT tags
	 */
	private String replaceContextTags(String expr) {
		return expr.replace("<CO>", "<CO_\\d*+>");  // possessive
	}
	
	/**
	 * Sets the <code>propertyID</code> field and replaces the PROPERTY tag by a
	 * capturing group.
	 * 
	 * @param expr pattern descriptor
	 * @return descriptor without PROPERTY tag
	 */
	private String replacePropertyTag(String expr) {
		// compute the ID of the group that represents the PROPERTY object
		// - get string before PROPERTY tag
		String s = expr.split("<PO[^>]*+>")[0];
		// - count number of '(' not preceded by '\' or followed by '?:'
		propertyID = s.split("\\(", -1).length -
					 s.split("\\\\\\(", -1).length -
					 s.split("\\(\\?\\:").length +
					 s.split("\\\\\\(\\?\\:").length + 1;
		
		// replace PROPERTY tag
		if (expr.contains("<PO>"))  // without NE types
			// length of PROPERTY objects that are not NEs is restricted to
			// MAX_PROP tokens, reluctant
			// (Note: NEs within a PROPERTY object are counted as 1)
			expr = expr.replace("<PO>",
					"([^ ]++(?: [^ ]++){0," + (MAX_PROP - 1) + "}?)");
		else {  // with NE types
			Matcher m = Pattern.compile("<PO_([^>]++)>").matcher(expr);
			m.find();
			expr = expr.replaceFirst("<PO_[^>]++>",
					"\\(<" + m.group(1) + ">\\)");
		}
		
		return expr;
	}
	
	/**
	 * Replaces NE tags by regular expressions that match NE tags with at least
	 * one of the NE types.
	 * 
	 * @param expr pattern descriptor
	 * @return descriptor with regular expressions for NE tags
	 */
	private String replaceNeTags(String expr) {
		Matcher m = Pattern.compile("<(NE[^>]*+)>").matcher(expr);
		while (m.find()) {  // find next NE tag
			// get NE types
			String[] neTypes = m.group(1).split("_");
			
			// build regular expression
			String regex = "<(?:NE[a-zA-Z0-9]*+_)*?";  // reluctant
			if (neTypes.length > 1) regex += "(?:";
			regex += neTypes[0];
			for (int i = 1; i < neTypes.length; i++)
				regex += "|" + neTypes[i];
			if (neTypes.length > 1) regex += ")";
			regex += "[^>]*+>";  // possessive
			
			// replace NE tag
			expr = expr.replace(m.group(0), regex);
		}
		
		return expr;
	}
	
	/**
	 * Optimizes the pattern to improve its runtime performance.
	 * 
	 * @param expr pattern descriptor
	 * @return optimized pattern
	 */
	private String optimizePattern(String expr) {
		// use possessive quantifiers whenever possible
		Matcher m = Pattern.compile("(?:\\[\\^<\\]\\*\\?|" +
				"\\(\\?\\:\\S*+ \\)\\?)++.").matcher(expr);
		while (m.find()) {
			String rep = m.group(0);
			
			if (rep.endsWith("<") || rep.endsWith(")")) {  // next token is '<'
				rep = rep.replace("[^<]*?", "[^<]*+");
//			} else {  // next token not '<'
//				// open parentheses
//				String closing = "";
//				Matcher m2 = Pattern.compile("\\[\\^<\\]\\*\\?").matcher(rep);
//				m2.find();
//				while (m2.find()) {
//					rep = rep.replaceFirst("\\[\\^<\\]\\*\\?",
//							"\\(\\?\\:\\(\\?\\:\\[\\^< \\]\\*\\+ \\)\\*\\?\\|" +
//							"\\[\\^<\\]\\*\\+");
//					closing += ")";
//				}
//				
//				// close parentheses
//				rep = rep.substring(0, rep.length() - 1) + closing +
//					  rep.charAt(rep.length() - 1);
			}
			
			expr = expr.replace(m.group(0), rep);
		}
		
		// eat whole tokens, reluctant
		expr = expr.replace("[^<]*?", "(?:[^< ]*+ )*?");
		
		return expr;
	}
	
	/**
	 * Compares this object to another <code>AnswerPattern</code>. Two
	 * <code>AnswerPattern</code> objects are equal, iff the pattern descriptors
	 * are equal.
	 * 
	 * @param o the reference object with which to compare
	 * @return <code>true</code>, iff this object is the same as the
	 *         <code>o</code> argument
	 */
	public boolean equals(Object o) {
		if (!(o instanceof AnswerPattern)) return false;
		
		return desc.equals(((AnswerPattern) o).getDesc());
	}
	
	/**
	 * Compares two <code>AnswerPattern</code> objects by comparing the number
	 * of correct applications.
	 * 
	 * @param ap the <code>AnswerPattern</code> to be compared
	 * @return a negative integer, zero or a positive integer as this
	 *         <code>AnswerPattern</code> is less than, equal to or greater than
	 *         the specified <code>AnswerPattern</code>
	 */
	public int compareTo(AnswerPattern ap) {
		return correct - ap.getCorrect();
	}
	
	/**
	 * The hashcode of an <code>AnswerPattern</code> is the hashcode of its
	 * descriptor.
	 * 
	 * @return hashcode
	 */
	public int hashCode() {
		return desc.hashCode();
	}
	
	/**
	 * Returns the pattern descriptor.
	 * 
	 * @return pattern descriptor
	 */
	public String getDesc() {
		return desc;
	}
	
	/**
	 * Returns the type of PROPERTY that is extracted with this pattern.
	 * 
	 * @return the PROPERTY
	 */
	public String getProperty() {
		return property;
	}
	
	/**
	 * Returns the number of correct applications of the pattern.
	 * 
	 * @return number of correct applications
	 */
	public int getCorrect() {
		return correct;
	}
	
	/**
	 * Returns the number of wrong applications of the pattern.
	 * 
	 * @return number of wrong applications
	 */
	public int getWrong() {
		return wrong;
	}
	
	/**
	 * Calculates a confidence measure for the pattern by applying the formula
	 * <code>confidence = correct / (correct + wrong)</code>.
	 * 
	 * @return confidence in the pattern
	 */
	public float getConfidence() {
		return ((float) correct) / (correct + wrong);
	}
	
	/**
	 * Increments the number of correct applications by 1.
	 */
	public void incCorrect() {
		correct++;
	}
	
	/**
	 * Increments the number of wrong applications by 1.
	 */
	public void incWrong() {
		wrong++;
	}
	
	/**
	 * Returns the NE types that are allowed for a PROPERTY object to match the
	 * pattern.
	 * 
	 * @return NE types or <code>null</code> iff no specific types are expected
	 */
	public String[] getPropertyTypes() {
		Matcher m = Pattern.compile("<PO_([^>]++)>").matcher(desc);
		if (!m.find()) return null;
		
		String[] neTypes = m.group(1).split("_");
		return neTypes;
	}
	
	/**
	 * Applies the pattern to a sentence of space-delimited tokens containing
	 * a TARGET tag and optionally a number of CONTEXT and NE tags. For each
	 * match, a PROPERTY object is extracted.
	 * 
	 * @param sentence a sentence
	 * @return array of PROPERTY objects or an empty array, if the sentence does
	 *         not match the pattern
	 */
	public String[] apply(String sentence) {
            
                /*PrintWriter pw = null;   
                try {
                    pw = new PrintWriter(new FileOutputStream(new File("regex_data.txt"),true));
                } catch (FileNotFoundException ex) {
                    System.out.println("File not found exception!!");
                }*/
                
              //  pw.printf("%s ----- %s\n", pattern.pattern(), sentence);

		Matcher m = pattern.matcher(sentence);
		ArrayList<String> results = new ArrayList<String>();
		
		while (m.find()) {
			if (m.group(distID).split(" ").length <= MAX_DIST)
				// distance between TARGET and PROPERTY is restricted to
				// MAX_DIST tokens (Note: NEs are counted as 1)
				results.add(m.group(propertyID));
			
			// continue search right after the beginning of this match
			m.region(m.start() + 1, sentence.length());
		}
                
               // pw.close();
		
		return results.toArray(new String[results.size()]);
	}
}

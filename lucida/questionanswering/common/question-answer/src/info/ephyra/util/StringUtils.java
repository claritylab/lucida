package info.ephyra.util;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.indices.FunctionWords;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashSet;

/**
 * A collection of utilities for string processing.
 * 
 * @author Nico Schlaefer
 * @version 2007-05-05
 */
// TODO use Levenstein distance to identify similar tokens
public class StringUtils {
	/**
	 * Fraction of words that must occur in both strings for
	 * <code>equalsIntersect()</code> to be true.
	 */
	private static final float INTERSECT_THRESH = 0.33f;
	
	/**
	 * Checks if the first array of tokens is a subset if the second array.
	 * 
	 * @param tokens1 token array 1
	 * @param tokens2 token array 2
	 * 
	 * @return true, iff ss1 is a subset of ss2
	 */
	private static boolean isSubset(String[] tokens1, String[] tokens2) {
		boolean exists;
		for (String token1 : tokens1) {
			exists = false;
			for (String token2 : tokens2)
				if (token1.equals(token2)) {
					exists = true;
					break;
				}
			
			if (!exists) return false;
		}
		return true;
	}
	
	/**
	 * Checks if the tokens in the first string form a subset of the tokens in
	 * the second string.
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the tokens in s1 are a subset of the tokens in s2
	 */
	public static boolean isSubset(String s1, String s2) {
		if (s1 == null) return true;
		if (s2 == null) return false;
		
		String[] tokens1 = s1.split(" ");
		String[] tokens2 = s2.split(" ");
		
		return isSubset(tokens1, tokens2);
	}

	/**
	 * Checks if the tokens in the first string form a subset of the tokens in
	 * the second string. Function words and tokens of length less than 2 are
	 * ignored.
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the keywords in s1 are a subset of the tokens in s2
	 */
	public static boolean isSubsetKeywords(String s1, String s2) {
		if (s1 == null) return true;
		if (s2 == null) return false;
		
		String[] tokens1 = s1.split(" ");
		String[] tokens2 = s2.split(" ");
		
		// eliminate function words and tokens of length < 2 from tokens1
		ArrayList<String> tks1 = new ArrayList<String>();
		for (String token1 : tokens1)
			if (token1.length() > 1 && !FunctionWords.lookup(token1))
				tks1.add(token1);
		tokens1 = tks1.toArray(new String[tks1.size()]);
		
		return isSubset(tokens1, tokens2);
	}
	
	/**
	 * Concatenates an array of strings, using the given delimiter.
	 * 
	 * @param ss array of strings
	 * @param delim delimiter
	 * @return concatenated string
	 */
	public static String concat(String[] ss, String delim) {
		String s = "";
		
		if (ss.length > 0) s += ss[0];
		for (int i = 1; i < ss.length; i++) s += delim + ss[i];
		
		return s;
	}
	
	/**
	 * Concatenates an array of strings, using whitespaces as delimiters.
	 * 
	 * @param ss array of strings
	 * @return concatenated string
	 */
	public static String concatWithSpaces(String[] ss) {
		String s = "";
		
		if (ss.length > 0) s += ss[0];
		for (int i = 1; i < ss.length; i++) s += " " + ss[i];
		
		return s;
	}
	
	/**
	 * Concatenates an array of strings, using tabs as delimiters.
	 * 
	 * @param ss array of strings
	 * @return concatenated string
	 */
	public static String concatWithTabs(String[] ss) {
		String s = "";
		
		if (ss.length > 0) s += ss[0];
		for (int i = 1; i < ss.length; i++) s += "\t" + ss[i];
		
		return s;
	}
	
	/**
	 * Repeats string <code>s</code> <code>n</code> times.
	 * 
	 * @param s a string
	 * @param n number of repetitions
	 */
	public static String repeat(String s, int n) {
		String repeated = "";
		
		for (int i = 0; i < n; i++) repeated += s;
		
		return repeated;
	}
	
	/**
	 * Normalizes a string. Similar strings are mapped to equal normalizations.
	 * 
	 * @param s the string
	 * @return normalized string
	 */
	// TODO use noun and verb stemming (also for equals...Norm() methods)
	public static String normalize(String s) {
		// convert to lower-case
		s = s.toLowerCase();
		
		// tokenize
		String tokens[] = NETagger.tokenize(s);
		
		// stemm all tokens
		for (int i = 0; i < tokens.length; i++)
			tokens[i] = SnowballStemmer.stem(tokens[i]);
		
		return concatWithSpaces(tokens);
	}
	
	/**
	 * Compares the normalizations of the two strings, using the standard
	 * <code>String.equals()</code> method.
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the normalizations are equal
	 */
	public static boolean equalsNorm(String s1, String s2) {
		return normalize(s1).equals(normalize(s2));
	}
	
	/**
	 * Compares two strings. The strings are considered equal, iff one of the
	 * strings is a subset of the other string, i.e. iff all the tokens in the
	 * one string also occur in the other string.
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the strings are equal in the sense defined above 
	 */
	public static boolean equalsSubset(String s1, String s2) {
		return isSubset(s1, s2) || isSubset(s2, s1);
	}
	
	/**
	 * Compares the normalizations of the two strings, using the
	 * <code>equalsSubset()</code> method.
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the normalizations are equal
	 */
	public static boolean equalsSubsetNorm(String s1, String s2) {
		return equalsSubset(normalize(s1), normalize(s2));
	}
	
	/**
	 * Compares two strings. The strings are considered equal, iff the number of
	 * words that occur in both strings over the total number of words is at
	 * least <code>INTERSECT_FRAC</code>.
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the strings are equal in the sense defined above
	 */
	public static boolean equalsIntersect(String s1, String s2) {
		// tokenize both strings
		String[] tokens1 = s1.split(" ");
		String[] tokens2 = s2.split(" ");
		
		// number of common tokens and total number of tokens
		// (note that duplicates are not handled properly)
		int commonTokens = 0;
		int totalTokens = tokens2.length;
		for (String token1 : tokens1)
			for (String token2 : tokens2)
				if (token1.equals(token2)) commonTokens++; else totalTokens++;
		
		return ((float) commonTokens) / totalTokens >= INTERSECT_THRESH;
	}
	
	/**
	 * Compares the normalizations of the two strings, using the
	 * <code>equalsIntersect()</code> method.
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the normalizations are equal
	 */
	public static boolean equalsIntersectNorm(String s1, String s2) {
		return equalsIntersect(normalize(s1), normalize(s2));
	}
	
	/**
	 * Compares two strings. The strings are considered equal, iff they have a
	 * common token. Function words and tokens of length less than 2 are
	 * ignored.
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the strings are equal in the sense defined above
	 */
	public static boolean equalsCommon(String s1, String s2) {
		// tokenize both strings
		String[] tokens1 = s1.split(" ");
		String[] tokens2 = s2.split(" ");
		
		// eliminate function words and tokens of length < 2
		ArrayList<String> tks1 = new ArrayList<String>();
		for (String token1 : tokens1)
			if (token1.length() > 1 && !FunctionWords.lookup(token1))
				tks1.add(token1);
		HashSet<String> tks2 = new HashSet<String>();
		for (String token2 : tokens2)
			if (token2.length() > 1 && !FunctionWords.lookup(token2))
				tks2.add(token2);
		
		// check for common token
		for (String token : tks1) if (tks2.contains(token)) return true;
		
		return false;
	}
	
	/**
	 * Compares the normalizations of the two strings, using the same criterion
	 * as the <code>equalsCommon()</code> method.
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the normalizations are equal
	 */
	public static boolean equalsCommonNorm(String s1, String s2) {
		// convert to lower-case
		s1 = s1.toLowerCase();
		s2 = s2.toLowerCase();
		
		// tokenize
		String tokens1[] = NETagger.tokenize(s1);
		String tokens2[] = NETagger.tokenize(s2);
		
		// eliminate function words and tokens of length < 2, stemm all tokens
		ArrayList<String> tks1 = new ArrayList<String>();
		for (String token1 : tokens1)
			if (token1.length() > 1 && !FunctionWords.lookup(token1))
				tks1.add(SnowballStemmer.stem(token1));
		HashSet<String> tks2 = new HashSet<String>();
		for (String token2 : tokens2)
			if (token2.length() > 1 && !FunctionWords.lookup(token2))
				tks2.add(SnowballStemmer.stem(token2));
		
		// check for common token
		for (String token : tks1) if (tks2.contains(token)) return true;
		
		return false;
	}
	
	/**
	 * Compares two strings, using the same criterion as the <code>equalsCommonNorm()</code> method, but considers only words starting with a capital letter (proper nouns)
	 * 
	 * @param s1 string 1
	 * @param s2 string 2
	 * @return true, iff the proper nouns are equal
	 */
	public static boolean equalsCommonProp(String s1, String s2) {
		// convert to lower-case
		s1 = s1.toLowerCase();
		s2 = s2.toLowerCase();
		
		// tokenize
		String tokens1[] = NETagger.tokenize(s1);
		String tokens2[] = NETagger.tokenize(s2);
		
		// eliminate function words and tokens of length < 2, stemm all tokens
		ArrayList<String> tks1 = new ArrayList<String>();
		for (String token1 : tokens1)
			if (token1.length() > 1 && !FunctionWords.lookup(token1) && token1.substring(0, 1).matches("[A-Z]"))
				tks1.add(SnowballStemmer.stem(token1));
		HashSet<String> tks2 = new HashSet<String>();
		for (String token2 : tokens2)
			if (token2.length() > 1 && !FunctionWords.lookup(token2) && token2.substring(0, 1).matches("[A-Z]"))
				tks2.add(SnowballStemmer.stem(token2));
		
		// check for common token
		for (String token : tks1) if (tks2.contains(token)) return true;
		
		return false;
	}
	
	/**
	 * Replaces all substrings of <code>s</code> that match <code>s1</code> with
	 * <code>s2</code>. This method is similar to <code>String.replace()</code>,
	 * but it ignores the case of <code>s1</code>.
	 * 
	 * @param s the string
	 * @param s1 the substring to be replaced
	 * @param s2 the replacement for the substring
	 * @return modified string
	 */
	public static String replaceIgnoreCase(String s, String s1, String s2) {
		return s.replaceAll("(?i)" + RegexConverter.strToRegex(s1),
							RegexConverter.strToRegex(s2));
	}
	
	/**
	 * <p>Sorts an array of strings by their length in ascending order.</p>
	 * 
	 * <p>This sort is guaranteed to be stable: strings of equal length are not
	 * reordered.</p>
	 * 
	 * @param ss array of strings
	 */
	public static void sortByLength(String[] ss) {
		Comparator<String> lengthC = new Comparator<String>() {
			public int compare(String s1, String s2) {
					return s1.length() - s2.length();
			}
		};
		
		Arrays.sort(ss, lengthC);
	}
	
	/**
	 * <p>Sorts an array of strings by their length in descending order.</p>
	 * 
	 * <p>This sort is guaranteed to be stable: strings of equal length are not
	 * reordered.</p>
	 * 
	 * @param ss array of strings
	 */
	public static void sortByLengthDesc(String[] ss) {
		Comparator<String> lengthC = new Comparator<String>() {
			public int compare(String s1, String s2) {
					return s2.length() - s1.length();
			}
		};
		
		Arrays.sort(ss, lengthC);
	}
}

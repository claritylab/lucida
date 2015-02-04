package info.ephyra.util;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.SnowballStemmer;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashSet;
import java.util.Iterator;

/**
 * <p>A <code>Dictionary</code> that is based on a hash set and allows lookups
 * in constant time.</p>
 * 
 * <p>All words are converted to lower case, tokenized and stemmed. E.g. there
 * is no distinction between "Internet" and "internets".</p>
 * 
 * <p>This class implements the interface <code>Dictionary</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-02-06
 */
public class HashDictionary implements Dictionary {
	/** <code>HashSet</code> used to store the words. **/
	private HashSet<String> words;
	/** <code>HashSet</code> used to store the tokens of words. **/
	private HashSet<String> tokens;
	/** Maximum number of tokens of a word in the dictionary. */
	private int maxTokens = 1;
	
	/**
	 * Creates an empty <code>HashDictionary</code>.
	 */
	public HashDictionary() {
		this.words = new HashSet<String>();
		this.tokens = new HashSet<String>();
	}
	
	/**
	 * Creates a <code>HashDictionary</code> from a list of words in a file.
	 * 
	 * @param fileName file containing a list of words
	 * @throws IOException if the list could not be read from the file
	 */
	public HashDictionary(String fileName) throws IOException {
		this();
		
		if (fileName != null) {
			File file = new File(fileName);
			BufferedReader in = new BufferedReader(new FileReader(file));
			
			while (in.ready()) {
				// read and normalize word
				String word = in.readLine().trim();
				if (word.startsWith("//")) continue;  // skip comments
				word = NETagger.tokenizeWithSpaces(word.toLowerCase());
				word = SnowballStemmer.stemAllTokens(word);
				
				// add whole word
				if (word.length() > 0) words.add(word);
				
				// add tokens of word
				String[] tokens = word.split(" ");
				if (tokens.length > maxTokens) maxTokens = tokens.length;
				for (int p = 0; p < tokens.length; p++)
					if (tokens[p].length() > 0) this.tokens.add(tokens[p]);
			}
			
			in.close();
		}
	}
	
	/**
	 * Adds a word to the dictionary.
	 * 
	 * @param word the word to add
	 */
	public void add(String word) {
		if (word != null) {
			word = NETagger.tokenizeWithSpaces(word.trim().toLowerCase());
			word = SnowballStemmer.stemAllTokens(word);
			
			// add whole word
			if (word.length() > 0) words.add(word);
			
			// add tokens of word
			String[] tokens = word.split(" ");
			if (tokens.length > maxTokens) maxTokens = tokens.length;
			for (int p = 0; p < tokens.length; p++)
				if (tokens[p].length() > 0) this.tokens.add(tokens[p]);
		}
	}
	
	/**
	 * Looks up a word.
	 * 
	 * @param word the word to look up
	 * @return <code>true</code> iff the word was found
	 */
	public boolean contains(String word) {
		word = NETagger.tokenizeWithSpaces(word.trim().toLowerCase());
		word = SnowballStemmer.stemAllTokens(word);
		
		return words.contains(word);
	}
	
	/**
	 * Looks up a word token.
	 * 
	 * @param token the word token to look up
	 * @return <code>true</code> iff a word in the dictionary contains the token
	 */
	public boolean containsToken(String token) {
		token = SnowballStemmer.stem(token.trim().toLowerCase());
		
		return tokens.contains(token);
	}
	
	/**
	 * Does a fuzzy lookup for a word. The specified word w is considered as
	 * contained in the dictionary is there is a word W in the dictionary such
	 * that <code>LevenshteinDistance(w, W) &lt;= maxDistance</code>
	 * 
	 * @param word the word to look up
	 * @param maxDistance the maximum Levenshtein edit distance for fuzzy
	 *            comparison
	 * @return <code>true</code> iff the word was found
	 */
	public boolean fuzzyContains(String word, int maxDistance) {
		word = NETagger.tokenizeWithSpaces(word.trim().toLowerCase());
		word = SnowballStemmer.stemAllTokens(word);
		
		if (maxDistance == 0) return this.words.contains(word);
		else if (this.words.contains(word)) return true;
		
		Iterator<String> wordIter = this.words.iterator();
		while (wordIter.hasNext())
			if (getLevenshteinDistance(word, wordIter.next(), maxDistance, true, 1, 1) <= maxDistance) return true;
		
		return false;
	}
	
	/**
	 * Does a fuzzy lookup for a token. The specified token t is considered as
	 * contained in the dictionary is there is a token T in the dictionary such
	 * that <code>LevenshteinDistance(t, T) &lt;= maxDistance</code>
	 * 
	 * @param token the token to look up
	 * @param maxDistance the maximum Levenshtein edit distance for fuzzy
	 *            comparison
	 * @return <code>true</code> iff a word in the dictionary contains the token
	 */
	public boolean fuzzyContainsToken(String token, int maxDistance) {
		token = SnowballStemmer.stem(token.trim().toLowerCase());
		
		if (maxDistance == 0) return this.tokens.contains(token);
		else if (this.tokens.contains(token)) return true;
		
		Iterator<String> tokenIter = this.tokens.iterator();
		while (tokenIter.hasNext())
			if (getLevenshteinDistance(token, tokenIter.next(), maxDistance, true, 1, 1) <= maxDistance) return true;
		
		return false;
	}
	
	/**	compute the Levenshtein distance of two Strings
	 * @param	string1			the first String
	 * @param	string2			the second String
	 * @param	threshold		the maximum distance (computation will stop if specified value reached)
	 * @param	caseSensitive	use case sensitive or case insensitive comparison
	 * @param	insertCost		the cost for inserting a Character
	 * @param	deleteCost		the cost for deleting a Character
	 * @return the Levenshtein distance of the specified Strings, maximum the specified threshold plus one, soon as the minimum possible distance exceeds the threshold
	 * Note: a threshold of 0 will compute the entire editing distance, regardless of its value
	 */
	public static int getLevenshteinDistance(String string1, String string2, int threshold, boolean caseSensitive, int insertCost, int deleteCost) {
		
		//	Step 1
		int length1 = ((string1 == null) ? 0 : string1.length()); // length of string1
		int length2 = ((string2 == null) ? 0 : string2.length()); // length of string2
		
		//	Step 1.5
		if ((Math.abs(length1 - length2) > threshold) && (threshold > 0)) return (threshold + 1);
		
		// Step 2
		int[][] distanceMatrix = new int[length1 + 1][length2 + 1]; // matrix
		distanceMatrix[0][0] = 0;
		
		//	fill the matrix top-left to bottom-right instead of line-wise
		int limit = 1;
		int minLength = ((length1 > length2) ? length2 : length1); // the limit for the square computation
		
		//	variables for distance computation
		int cost; // cost in current step
		int substitutionCost = ((insertCost + deleteCost) / 2);
		int distance = 0; // minimum distance currently possible
		
		while (limit <= minLength) {
			distanceMatrix[limit][0] = (limit * insertCost);
			distanceMatrix[0][limit] = (limit * deleteCost);
			
			//	compute line
			for (int c = 1; c < limit; c++) {
				cost = getCost(string1.charAt(c - 1), string2.charAt(limit - 1), substitutionCost, caseSensitive);
				distance = min3(distanceMatrix[c - 1][limit] + deleteCost, distanceMatrix[c][limit - 1] + insertCost, distanceMatrix[c - 1][limit - 1] + cost);
				distanceMatrix[c][limit] = distance;
			}
			
			//	compute column
			for (int l = 1; l < limit; l++) {
				cost = getCost(string1.charAt(limit - 1), string2.charAt(l - 1), substitutionCost, caseSensitive);
				distance = min3(distanceMatrix[limit - 1][l] + deleteCost, distanceMatrix[limit][l - 1] + insertCost, distanceMatrix[limit - 1][l - 1] + cost);
				distanceMatrix[limit][l] = distance;
			}
			
			//	compute new corner
			cost = getCost(string1.charAt(limit - 1), string2.charAt(limit - 1), substitutionCost, caseSensitive);
			distance = min3(distanceMatrix[limit - 1][limit] + deleteCost, distanceMatrix[limit][limit - 1] + insertCost, distanceMatrix[limit - 1][limit - 1] + cost);
			if ((distance > threshold) && (threshold > 0)) return (threshold + 1);
			distanceMatrix[limit][limit] = distance;
			
			//	increment limit
			limit ++;
		}
		
		//	Step 2.5 (compute remaining columns)
		while (limit <= length1) {
			distanceMatrix[limit][0] = (limit * insertCost);
			
			//	compute column
			for (int l = 1; l <= length2; l++) {
				cost = getCost(string1.charAt(limit - 1), string2.charAt(l - 1), substitutionCost, caseSensitive);
				distance = min3(distanceMatrix[limit - 1][l] + deleteCost, distanceMatrix[limit][l - 1] + insertCost, distanceMatrix[limit - 1][l - 1] + cost);
				distanceMatrix[limit][l] = distance;
			}
			if ((distance > threshold) && (threshold > 0)) return (threshold + 1);
			
			//	increment limit
			limit ++;
		}
		
		//	Step 2.5b (compute remaining rows)
		while (limit <= length2) {
			distanceMatrix[0][limit] = (limit * deleteCost);
			
			//	compute line
			for (int c = 1; c <= length1; c++) {
				cost = getCost(string1.charAt(c - 1), string2.charAt(limit - 1), substitutionCost, caseSensitive);
				distance = min3(distanceMatrix[c - 1][limit] + deleteCost, distanceMatrix[c][limit - 1] + insertCost, distanceMatrix[c - 1][limit - 1] + cost);
				distanceMatrix[c][limit] = distance;
			}
			if ((distance > threshold) && (threshold > 0)) return (threshold + 1);
			
			//	increment limit
			limit ++;
		}
		
		// Step 7
		return distanceMatrix[length1][length2];
	}
	
	/**	compute edit cost for two chars
	 * @param	char1			the first char
	 * @param	char2			the second char
	 * @param	substCost		the cost for the substitution of one char with another one
	 * @param	caseSensitive	use case sensitive or case insensitive comparison for the Token's values 
	 * @return the edit cost for the two Tokens
	 */
	public static int getCost(char char1, char char2, int substCost, boolean caseSensitive) {
		if (char1 == char2) return 0;
		if (!caseSensitive && (Character.toLowerCase(char1) == Character.toLowerCase(char2))) return 0;
		return substCost;
	}
	
	/**	compute the minimum of three int variables (helper for Levenshtein)
	 * @param	x
	 * @param	y
	 * @param	z
	 * @return the minimum of x, y and z
	 */
	public static int min3(int x, int y, int z) {
		return Math.min(x, Math.min(y, z));
	}
	
	/**
	 * Returns an iterator over the dictionary entries.
	 * 
	 * @return iterator
	 */
	public Iterator<String> getIterator() {
		return words.iterator();
	}
	
	/**	
	 * Returns the maximum number of tokens of a word in the dictionary.
	 * 
	 * @return maximum number of tokens
	 */
	public int getMaxTokens() {
		return maxTokens;
	}
}

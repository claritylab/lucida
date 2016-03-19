package info.ephyra.answerselection.filters;

import info.ephyra.questionanalysis.KeywordExtractor;
import info.ephyra.search.Result;

/**
 * <p>Filters results by the number of keywords.</p>
 * 
 * <p>The score of each result is incremented by the number of keywords it
 * contains.</p>
 * 
 * <p>A result is dropped if <code>m < Floor(Sqrt(k - 1)) + 1</code>, where
 * <code>k</code> is the number of keywords in the query string and
 * <code>m</code> is the number of keywords that also occur in the result.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-09-14
 */
public class NumberOfKeywordsFilter extends Filter {
	/**
	 * Counts the number of words in the first array that occur in the second
	 * array. Does not distinguish between lower and upper case.
	 * 
	 * @param s1 string array 1
	 * @param s2 string array 2
	 * @return number of words in array 1 that occur in array 2
	 */
	private int getNumberOfMatches(String[] s1, String[] s2) {
		int count = 0;
		
		for (String word1 : s1)
			for (String word2 : s2)
				if (word1.equalsIgnoreCase(word2)) {
					count++;
					break;  // count each word in s1 only once
				}
		
		return count;
	}
	
	/**
	 * Filters a single <code>Result</code> object.
	 * 
	 * @param result result to filter
	 * @return result with manipulated score or <code>null</code> if the result
	 * 		   is dropped
	 */
	public Result apply(Result result) {
		String[] kws = result.getQuery().getAnalyzedQuestion().getKeywords();
		String[] wordsInResult = KeywordExtractor.tokenize(result.getAnswer());
		
		int k = kws.length;
		int m = getNumberOfMatches(kws, wordsInResult);
		
		if (m >= Math.floor(Math.sqrt(k - 1)) + 1) {
			result.incScore(m);  // manipulate score
			return result;  // keep result
		}
		
		return null;  // drop result
	}
}

package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.Arrays;

/**
 * <p>The <code>ScoreSorterFilter</code> sorts the results by their scores in
 * descending order. The sort is guaranteed to be stable.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-24
 */
public class ScoreSorterFilter extends Filter {
	/**
	 * Sorts the results by their scores in descending order.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return sorted array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// invert scores
		for (Result result : results) result.setScore(result.getScore() * -1);
		
		Arrays.sort(results);
		
		// restore scores
		for (Result result : results) result.setScore(result.getScore() * -1);
		
		return results;
	}
}

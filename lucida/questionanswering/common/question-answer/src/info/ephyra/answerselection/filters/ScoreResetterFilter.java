package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

/**
 * <p>Filter setting the score of each result to 0.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class ScoreResetterFilter extends Filter {
	/**
	 * Sets the score of the result to 0 and returns it.
	 * 
	 * @param result result to filter
	 * @return result with score 0
	 */
	public Result apply(Result result) {
		result.setScore(0f);
		return result;
	}
	
	/**
	 * Sets the scores of all results to 0 and returns them.
	 * 
	 * @param results results to filter
	 * @return results with score 0
	 */
	public Result[] apply(Result[] results) {
		for (Result r : results) r.setScore(0f);
		return results;
	}
}

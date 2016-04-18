package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

/**
 * <p>This filter simply increments the score of each result by the score of the
 * query that was used to obtain the result. This takes into consideration that
 * more specific queries in general return more valuable results than simple
 * "bags of words".</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-09-15
 */
public class QueryScoreFilter extends Filter {
	/**
	 * Filters a single <code>Result</code> object.
	 * 
	 * @param result result to filter
	 * @return result with manipulated score
	 */
	public Result apply(Result result) {
		result.incScore(result.getQuery().getScore());  // manipulate score
		
		return result;  // no results are dropped by this filter
	}
}

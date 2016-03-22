package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

/**
 * <p>Combines the extra scores of each <code>Result</code> and sets the main
 * score to their sum.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-10
 */
public class CombineScoresFilter extends Filter {
	
	/**
	 * add all the extra scores of the specified result to its main score and return it.
	 * 
	 * @param result a <code>Result</code> object
	 * @return the same <code>Result</code> object
	 */
	public Result apply(Result result) {
		float score = result.getScore();
		float[] scores = result.getExtraScores();
		for (float s : scores) score += s;
		result.setScore(score);
		return result;
	}
	
	/** @see info.ephyra.answerselection.filters.Filter#apply(info.ephyra.search.Result[])
	 */
	public Result[] apply(Result[] results) {
		for (Result r : results) this.apply(r);
		return results;
	}
}
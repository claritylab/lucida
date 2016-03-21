package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

/**
 * <p>The <code>NormalizedScoreSorterFilter</code> sorts the results by their
 * normalized scores in descending order. The sort is guaranteed to be stable.
 * </p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-24
 */
public class NormalizedScoreSorterFilter extends Filter {
	/**
	 * Sorts the results by their normalized scores in descending order.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return sorted array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// switch scores and normalized scores
		for (Result result : results) {
			float normScore = result.getNormScore();
			if (normScore == 0) continue;
			
			result.setNormScore(result.getScore());
			result.setScore(normScore);
		}
		
		// sort by normalized scores in descending order
		results = (new ScoreSorterFilter()).apply(results);
		
		// switch scores back
		for (Result result : results) {
			float normScore = result.getNormScore();
			if (normScore == 0) continue;
			
			result.setNormScore(result.getScore());
			result.setScore(normScore);
		}
		
		return results;
	}
}

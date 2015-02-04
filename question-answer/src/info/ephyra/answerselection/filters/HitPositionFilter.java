package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

/**
 * <p>Scores the results according to their hit positions. The score of each
 * result is modified by <code>scale * hitposition</code>, where the constant
 * <code>scale</code> specifies a scaling factor that should be negative.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-09-16
 */
public class HitPositionFilter extends Filter {
	/**
	 * Scaling factor to adjust the effect of this filter. Should be negative
	 * since a lower hit position should result in a higher score.
	 */
	private static final float scale = -0.001f;
	
	/**
	 * Filters a single <code>Result</code> object.
	 * 
	 * @param result result to filter
	 * @return result with manipulated score
	 */
	public Result apply(Result result) {
		result.incScore(scale * result.getHitPos());  // manipulate score
		
		return result;  // no results are dropped by this filter
	}
}

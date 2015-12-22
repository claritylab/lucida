package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.ArrayList;

/**
 * <p>A <code>Filter</code> is part of a pipeline for answer extraction and
 * selection. It manipulates an array of <code>Result</code> objects.</p>
 * 
 * <p>A filter can drop results, create new results from existing ones, or
 * modify results. It can process the results independently (in which case the
 * apply(Result) method should be implemented, or it can simultaneously process
 * a list of results (in which case the apply(Result[]) method should be
 * implemented).</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-03-07
 */
public abstract class Filter {
	/**
	 * Filters a single <code>Result</code> object.
	 * 
	 * @param result result to filter
	 * @return modified result or <code>null</code> if the result is dropped
	 */
	public Result apply(Result result) {
		return result;
	}
	
	/**
	 * Filters an array of <code>Result</code> objects.
	 * 
	 * @param results results to filter
	 * @return filtered results
	 */
	public Result[] apply(Result[] results) {
		ArrayList<Result> filtered = new ArrayList<Result>();
		
		for (Result result : results) {
			// filter results that do not have a score of
			// Float.NEGATIVE_INFINITY or Float.POSITIVE_INFINITY
			if (result.getScore() > Float.NEGATIVE_INFINITY &&
				result.getScore() < Float.POSITIVE_INFINITY)
				result = apply(result);
			
			if (result != null) filtered.add(result);
		}
		
		return filtered.toArray(new Result[filtered.size()]);
	}
}

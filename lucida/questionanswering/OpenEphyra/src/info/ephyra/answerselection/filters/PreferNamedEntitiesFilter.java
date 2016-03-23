package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.ArrayList;

/**
 * <p>The <code>PreferNamedEntitiesFilter</code> prefers factoid answers that
 * are named entities over other factoid answers.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-01-29
 */
public class PreferNamedEntitiesFilter extends Filter {
	/**
	 * If there are named entities among the factoid answers then answers that
	 * are not named entities are dropped.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return results that are named entities
	 */
	public Result[] apply(Result[] results) {
		// raw results returned by the searchers
		ArrayList<Result> rawResults = new ArrayList<Result>();
		// results that are named entities
		ArrayList<Result> namedEntities = new ArrayList<Result>();
		// results that are not named entities
		ArrayList<Result> notNamedEntities = new ArrayList<Result>();
		
		// get only named entities if there are any
		for (Result result : results)
			if (result.getScore() == Float.NEGATIVE_INFINITY ||
				result.getScore() == Float.POSITIVE_INFINITY) {
				rawResults.add(result);
			} else {
				if (result.isNamedEntity()) namedEntities.add(result);
				else notNamedEntities.add(result);
			}
		ArrayList<Result> preferred = (namedEntities.size() > 0)
			? namedEntities	: notNamedEntities;
		
		// keep raw results
		Result[] allResults = new Result[preferred.size() + rawResults.size()];
		preferred.toArray(allResults);
		for (int i = 0; i < rawResults.size(); i++)
			allResults[preferred.size() + i] = rawResults.get(i);
		
		return allResults;
	}
}

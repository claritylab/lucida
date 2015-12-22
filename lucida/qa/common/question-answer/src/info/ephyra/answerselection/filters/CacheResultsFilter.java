package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

/**
 * <p>Caches the results along with the query string of the first result. If the
 * query string is the same in a subsequent call, the cached results are
 * returned, otherwise the cache is updated.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter, Nico Schlaefer
 * @version 2008-02-10
 */
public class CacheResultsFilter  extends Filter {
	/**
	 * Resets the result array to a previous state if the query string matches.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return modified array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		//	catch empty result
		if (results.length == 0) return results;
		
		//	get query string
		String query = results[0].getQuery().getOriginalQueryString();
		
		// look up in cache
		Result[] res = this.cacheLookup(query);
		
		//	initializer call
		if (res == null) {
			int c = 0;
			Result[] copy = new Result[results.length];
			for (Result r : results)
				copy[c++] = r.getCopy();
			this.cache(query, copy);
			return results;
			
		//	lookup call
		} else return res;
	}
	
	private String lastQuery = null;
	private Result[] lastQueryResults = null;
	
	private void cache(String query, Result[] results) {
		this.lastQuery = query;
		this.lastQueryResults = results;
	}
	
	private Result[] cacheLookup(String query) {
		if (query.equals(this.lastQuery)) return this.lastQueryResults;
		else return null;
	}
}

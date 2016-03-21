package info.ephyra.search.searchers;

import info.ephyra.querygeneration.Query;
import info.ephyra.search.Result;
import info.ephyra.search.Search;

/**
 * <p>A <code>Searcher</code> queries an unstructured or (semi)structured
 * knowledge source and passes the results to the static class
 * <code>Search</code>.</p>
 * 
 * <p>It runs as a separate thread, so several queries can be performed in
 * parallel.</p>
 * 
 * <p>This class extends the class <code>Thread</code> and is abstract.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-11-01
 */
public abstract class Searcher extends Thread {
	/** Query that is performed. */
	protected Query query;
	/** The results found in the knowledge source. */
	protected Result[] results;
	
	/**
	 * Searches an unstructured or (semi)structured knowledge source and returns
	 * an array of search results.
	 * 
	 * @return search results or an empty array, if the search failed
	 */
	protected abstract Result[] doSearch();
	
	/**
	 * Performs the search and passes the results to the static class
	 * <code>Search</code>.
	 */
	public void run() {
		if (query != null) {
			// perform search
			Result[] results = doSearch();
			// pass results to class Search
			Search.addResults(results);

		}
	}
}

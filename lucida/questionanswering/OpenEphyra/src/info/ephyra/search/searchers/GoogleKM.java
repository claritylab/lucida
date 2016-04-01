package info.ephyra.search.searchers;

import info.ephyra.io.MsgPrinter;
import info.ephyra.search.Result;

import com.google.soap.search.GoogleSearch;
import com.google.soap.search.GoogleSearchFault;
import com.google.soap.search.GoogleSearchResult;
import com.google.soap.search.GoogleSearchResultElement;

/**
 * <p>A <code>KnowledgeMiner</code> that deploys the Google search engine to
 * search the Web.</p>
 * 
 * <p>It runs as a separate thread, so several queries can be performed in
 * parallel.</p>
 * 
 * <p>This class extends the class <code>KnowledgeMiner</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-29
 */
public class GoogleKM extends KnowledgeMiner {
	/** Google license key. */
	private static final String GOOGLE_KEY = "Enter your Google license key.";
	/** Maximum total number of search results. */
	private static final int MAX_RESULTS_TOTAL = 100;
	/** Maximum number of search results per query. */
	private static final int MAX_RESULTS_PERQUERY = 10;
	/** Number of retries if search fails. */
	private static final int RETRIES = 50;
	
	/**
	 * Returns the maximum total number of search results.
	 * 
	 * @return maximum total number of search results
	 */
	protected int getMaxResultsTotal() {
		return MAX_RESULTS_TOTAL;
	}
	
	/**
	 * Returns the maximum number of search results per query.
	 * 
	 * @return maximum total number of search results
	 */
	protected int getMaxResultsPerQuery() {
		return MAX_RESULTS_PERQUERY;
	}
	
	/**
	 * Queries the Google search engine and returns an array containing up to
	 * <code>MAX_RESULTS_PERQUERY</code> search results.
	 * 
	 * @return Google search results
	 */
	protected Result[] doSearch() {
		GoogleSearch search = new GoogleSearch();
		
		// set license key
		search.setKey(GOOGLE_KEY);
		// set search string
		search.setQueryString(query.getQueryString());
		// set language to English only
		search.setLanguageRestricts("English");
		// set hit position of first search result
		search.setStartResult(firstResult);
		// set maximum number of search results
		search.setMaxResults(maxResults);
		
		// perform search
		GoogleSearchResult googleResult = null;
		int retries = 0;
		while (googleResult == null)
			try {
				googleResult = search.doSearch();
			} catch (GoogleSearchFault e) {
				MsgPrinter.printSearchError(e);  // print search error message
				
				if (retries == RETRIES) {
					MsgPrinter.printErrorMsg("\nSearch failed.");
					System.exit(1);
				}
				retries++;
				
				try {
					GoogleKM.sleep(1000);
				} catch (InterruptedException ie) {}
			}
		
		// get snippets and URLs of the corresponding websites
		GoogleSearchResultElement[] elements = googleResult.getResultElements();
		String[] snippets = new String[elements.length];
		String[] urls = new String[elements.length];
		for (int i = 0; i < elements.length; i++) {
			snippets[i] = elements[i].getSnippet();
			urls[i] = elements[i].getURL();
		}
		
		// return results
		return getResults(snippets, urls, true);
	}
	
	/**
	 * Returns a new instance of <code>GoogleKM</code>. A new instance is
	 * created for each query.
	 * 
	 * @return new instance of <code>GoogleKM</code>
	 */
	public KnowledgeMiner getCopy() {
		return new GoogleKM();
	}
}

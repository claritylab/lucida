package info.ephyra.search.searchers;

import info.ephyra.io.MsgPrinter;
import info.ephyra.search.Result;

import java.math.BigInteger;

import com.yahoo.search.SearchClient;
import com.yahoo.search.WebSearchRequest;
import com.yahoo.search.WebSearchResult;

/**
 * <p>A <code>KnowledgeMiner</code> that deploys the Yahoo search engine to
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
public class YahooKM extends KnowledgeMiner {
	/** Yahoo application ID, allows 5,000 queries per day and IP address. */
	private static final String YAHOO_ID = "questionanswering";
	/** Maximum total number of search results. */
	private static final int MAX_RESULTS_TOTAL = 100;
	/** Maximum number of search results per query. */
	private static final int MAX_RESULTS_PERQUERY = 100;
	/** Number of retries if search fails. */
	private static final int RETRIES = 50;
	
	/**
	 * Returns a representation of the query string that is suitable for Yahoo.
	 * 
	 * @param qs query string
	 * @return query string for Yahoo
	 */
	public static String transformQueryString(String qs) {
		// drop parentheses
		qs = qs.replace("(", "");
		qs = qs.replace(")", "");
		
		return qs;
	}
	
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
	 * Queries the Yahoo search engine and returns an array containing up to
	 * <code>MAX_RESULTS_PERQUERY</code> search results.
	 * 
	 * @return Yahoo search results
	 */
	protected Result[] doSearch() {
		SearchClient client = new SearchClient(YAHOO_ID);
		
		// create request
		WebSearchRequest request =
			new WebSearchRequest(transformQueryString(query.getQueryString()));
		request.setLanguage("en");  // search for English pages only
		request.setStart(BigInteger.valueOf(firstResult));
		request.setResults(maxResults);
		
		// perform search
		WebSearchResult[] searchResults = null;
		int retries = 0;
		while (searchResults == null)
			try {
				searchResults = client.webSearch(request).listResults();
			} catch (Exception e) {
				MsgPrinter.printSearchError(e);  // print search error message
				
				if (retries == RETRIES) {
					MsgPrinter.printErrorMsg("\nSearch failed.");
					System.exit(1);
				}
				retries++;
				
				try {
					YahooKM.sleep(1000);
				} catch (InterruptedException ie) {}
			}
		
		// get snippets and URLs of the corresponding websites
		String[] snippets = new String[searchResults.length];
		String[] urls = new String[searchResults.length];
		String[] cacheUrls = new String[searchResults.length];
		for (int i = 0; i < searchResults.length; i++) {
			snippets[i] = searchResults[i].getSummary();
			urls[i] = searchResults[i].getUrl();
			if (searchResults[i].getCache() != null)
				cacheUrls[i] = searchResults[i].getCache().getUrl();
		}
		
		// set cache URLs and return results
		return getResults(snippets, urls, cacheUrls, true);
	}
	
	/**
	 * Returns a new instance of <code>YahooKM</code>. A new instance is created
	 * for each query.
	 * 
	 * @return new instance of <code>YahooKM</code>
	 */
	public KnowledgeMiner getCopy() {
		return new YahooKM();
	}
}

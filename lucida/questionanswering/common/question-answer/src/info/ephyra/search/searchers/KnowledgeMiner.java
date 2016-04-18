package info.ephyra.search.searchers;

import info.ephyra.querygeneration.Query;
import info.ephyra.search.Result;
import info.ephyra.search.Search;
import info.ephyra.util.HTMLConverter;

import java.util.ArrayList;

/**
 * <p>A <code>KnowledgeMiner</code> deploys a document retrieval system to
 * search an unstructured knowledge source, e.g. Google to search the World Wide
 * Web.</p>
 * 
 * <p>It runs as a separate thread, so several queries can be performed in
 * parallel.</p>
 * 
 * <p>This class extends the class <code>Searcher</code> and is abstract.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-29
 */
public abstract class KnowledgeMiner extends Searcher {
	/** The hit position of the first result to be fetched. */
	protected int firstResult;
	/** The maximum number of results to be fetched. */
	protected int maxResults;

	/**
	 * Returns the maximum total number of search results.
	 * 
	 * @return maximum total number of search results
	 */
	protected abstract int getMaxResultsTotal();
	
	/**
	 * Returns the maximum number of search results per query.
	 * 
	 * @return maximum total number of search results
	 */
	protected abstract int getMaxResultsPerQuery();
	
	/**
	 * Creates <code>Result</code> objects form an array of text passages and
	 * document IDs.
	 * 
	 * @param passages text passages
	 * @param docIDs IDs of the documents the text passages are from
	 * @param isHtml flag indicating that the passages are HTML code
	 * @return <code>Result</code> objects
	 */
	protected Result[] getResults(String[] passages, String[] docIDs,
			boolean isHtml) {
		return getResults(passages, docIDs, new String[docIDs.length], isHtml);
	}
	
	/**
	 * Creates <code>Result</code> objects form an array of text passages,
	 * document IDs and IDs of cached documents.
	 * 
	 * @param passages text passages
	 * @param docIDs IDs of the documents the text passages are from
	 * @param cacheIDs IDs of the documents in the search engine cache
	 * @param isHtml flag indicating that the passages are HTML code
	 * @return <code>Result</code> objects
	 */
	protected Result[] getResults(String[] passages, String[] docIDs,
			String[] cacheIDs, boolean isHtml) {
		ArrayList<Result> results = new ArrayList<Result>();
		
		for (int i = 0; i < passages.length; i++) {
			if (passages[i] != null) {
				String[] split;
				if (isHtml) {
					// convert to plain text and split around "..."
					passages[i] = HTMLConverter.htmlsnippet2text(passages[i]);
					split = passages[i].split("\\.\\.\\.");
				} else {
					// replace sequences of whitespaces by single blanks
					passages[i] = passages[i].replaceAll("\\s++", " ");
					split = new String[] {passages[i]};
				}
				
				for (String passage : split) {
					passage = passage.trim();
					
					if (passage.length() > 0) {
						Result result = new Result(passage, query, docIDs[i],
												   i + firstResult - 1);
						result.setCacheID(cacheIDs[i]);
						// result is never returned by the QA engine but is only
						// used to derive other results
						result.setScore(Float.NEGATIVE_INFINITY);
						
						results.add(result);
					}
				}
			}
		}
		
		return results.toArray(new Result[results.size()]);
	}
	
	/**
	 * <p>Sets the query, the hit position of the first result and the number of
	 * results to be fetched and starts the thread.</p>
	 * 
	 * <p>This method should be used instead of the inherited
	 * <code>start()</code> method without arguments.</p>
	 * 
	 * @param query <code>Query</code> object
	 * @param firstResult hit position of the first result
	 */
	protected void start(Query query, int firstResult) {
		this.query = query;
		this.firstResult = firstResult;
		this.maxResults = Math.min(getMaxResultsPerQuery(),
								   getMaxResultsTotal() - firstResult + 1);
		
		// wait until there are less than MAX_PENDING pending queries
		Search.waitForPending();
		
		start();
		
		// one more pending query
		Search.incPending();
	}
	
	/**
	 * <p>Returns a new instance of the <code>KnowledgeMiner</code>. A new
	 * instance is created for each query.</p>
	 * 
	 * <p>It does not necessarily return an exact copy of the current
	 * instance.</p>
	 * 
	 * @return new instance of the <code>KnowledgeMiner</code>
	 */
	public abstract KnowledgeMiner getCopy();
	
	/**
	 * <p>Creates <code>[MAX_RESULTS_TOTAL / MAX_RESULTS_PERQUERY]</code>
	 * threads that fetch up to <code>MAX_RESULTS_TOTAL</code> results.</p>
	 * 
	 * <p>This method should be used instead of the inherited
	 * <code>start()</code> method without arguments.</p>
	 * 
	 * @param query <code>Query</code> object
	 */
	public void start(Query query) {
		int firstResult = 1;
		
		while (firstResult <= getMaxResultsTotal()) {
			getCopy().start(query, firstResult);
			
			firstResult += getMaxResultsPerQuery();
		}
	}
}

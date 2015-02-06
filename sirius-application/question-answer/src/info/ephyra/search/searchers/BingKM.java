package info.ephyra.search.searchers;

import info.ephyra.search.Result;

import java.util.ArrayList;
import java.util.List;

import com.aliasi.util.Collections;
import com.google.code.bing.search.client.BingSearchClient;
import com.google.code.bing.search.client.BingSearchClient.SearchRequestBuilder;
import com.google.code.bing.search.client.BingSearchServiceClientFactory;
import com.google.code.bing.search.schema.AdultOption;
import com.google.code.bing.search.schema.SearchOption;
import com.google.code.bing.search.schema.SearchResponse;
import com.google.code.bing.search.schema.SourceType;
import com.google.code.bing.search.schema.web.WebResult;
import com.google.code.bing.search.schema.web.WebSearchOption;

/**
 * <p>A <code>KnowledgeMiner</code> that deploys the Bing search engine to
 * search the Web.</p>
 * 
 * <p>It runs as a separate thread, so several queries can be performed in
 * parallel.</p>
 * 
 * <p>This class extends the class <code>KnowledgeMiner</code>.</p>
 * 
 * @author James Zhang, Jeff Chen
 * @version 2011-10-05
 */
public class BingKM extends KnowledgeMiner {
	/** Bing Application ID. */
	private static final String BING_APP_ID = "1A128D5664A0CE797F3BB450785221091616619A";
	/** Maximum total number of search results. */
	private static final int MAX_RESULTS_TOTAL = 100;
	/** Maximum number of search results per query. */
	private static final int MAX_RESULTS_PERQUERY = 50;

	@Override
	protected int getMaxResultsTotal() {
		return MAX_RESULTS_TOTAL;
	}

	@Override
	protected int getMaxResultsPerQuery() {
		return MAX_RESULTS_PERQUERY;
	}

	/**
	 * Returns a new instance of <code>BingKM</code>. A new instance is created
	 * for each query.
	 * 
	 * @return new instance of <code>BingKM</code>
	 */
	@Override
	public KnowledgeMiner getCopy() {
		return new BingKM();
	}

	@Override
	protected Result[] doSearch() {
		// get a search client
		BingSearchServiceClientFactory factory = BingSearchServiceClientFactory
				.newInstance();
		BingSearchClient client = factory.createBingSearchClient();

		// configure search client
		SearchRequestBuilder builder = client.newSearchRequestBuilder();
		builder.withAppId(BING_APP_ID);
		builder.withQuery(query.getQueryString());
		builder.withSourceType(SourceType.WEB);
		builder.withVersion("2.0");
		builder.withMarket("en-us");
		builder.withAdultOption(AdultOption.MODERATE);
		builder.withSearchOption(SearchOption.ENABLE_HIGHLIGHTING);
		builder.withWebRequestCount((long) maxResults);
		builder.withWebRequestOffset((long) firstResult);
		builder.withWebRequestSearchOption(WebSearchOption.DISABLE_HOST_COLLAPSING);
		builder.withWebRequestSearchOption(WebSearchOption.DISABLE_QUERY_ALTERATIONS);

		// do the actual search here, and collect the results
		SearchResponse response = client.search(builder.getResult());
		List<WebResult> results = response.getWeb().getResults();
		ArrayList<String> snippets = new ArrayList<String>();
		ArrayList<String> urls = new ArrayList<String>();
		for (WebResult result : results) {
			snippets.add(result.getDescription());
			urls.add(result.getUrl());
		}

		// return results
		return getResults(Collections.toStringArray(snippets),
				Collections.toStringArray(urls), true);
	}
}

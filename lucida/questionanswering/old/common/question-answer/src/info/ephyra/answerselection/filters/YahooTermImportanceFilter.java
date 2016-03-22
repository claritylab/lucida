package info.ephyra.answerselection.filters;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.search.searchers.YahooKM;

import java.math.BigInteger;
import java.util.HashMap;

import com.yahoo.search.SearchClient;
import com.yahoo.search.WebSearchRequest;
import com.yahoo.search.WebSearchResult;

/**
 * <p>A web term importance filter that counts term frequencies in text snippets
 * retrieved with the Yahoo search engine.</p>
 * 
 * <p>This class extends the class <code>WebTermImportanceFilter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class YahooTermImportanceFilter extends WebTermImportanceFilter {
	
	/** Yahoo application ID, allows 5,000 queries per day and IP address. */
	private static final String YAHOO_ID = "questionanswering";
//	/** Maximum total number of search results. */
//	private static final int MAX_RESULTS_TOTAL = 600;
	/** Maximum number of search results per query. */
	private static final int MAX_RESULTS_PERQUERY = 100;
	/** Number of retries if search fails. */
	private static final int RETRIES = 60;
	
	/**
	 * @param normalizationMode
	 * @param tfNormalizationMode
	 * @param isCombined
	 */
	public YahooTermImportanceFilter(int normalizationMode, int tfNormalizationMode, boolean isCombined) {
		super(normalizationMode, tfNormalizationMode, isCombined);
	}
	
	/** @see info.ephyra.answerselection.filters.WebTermImportanceFilter#getTermCounters(java.lang.String[])
	 */
	@Override
	public HashMap<String, TermCounter> getTermCounters(String[] targets) {
		HashMap<String, TermCounter> termCounters = new HashMap<String, TermCounter>();
		for (String target : targets) {
			
			//	get snippets from yahoo
			SearchClient client = new SearchClient(YAHOO_ID);
			
			// create request
			WebSearchRequest request = new WebSearchRequest(target);
			request.setLanguage("en");  // search for English pages only
			request.setStart(BigInteger.valueOf(0));
			request.setResults(MAX_RESULTS_PERQUERY);
			
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
			
			//	parse yahoo snippets
			int lengthSum = 0;
			for (int i = 0; i < searchResults.length; i++) {
				
				String summary = searchResults[i].getSummary();
				if (summary != null) {
					
					//	tokenize and tag sentence
					String[] sentence = NETagger.tokenize(summary);
					lengthSum += sentence.length;
					
					//	scan sentence for NPs
					for (int s = 0; s < sentence.length; s++) {
						String term = SnowballStemmer.stem(sentence[s].toLowerCase());
						if (term.length() > 1) {
							if (!termCounters.containsKey(term))
								termCounters.put(term, new TermCounter());
							termCounters.get(term).increment();
						}
					}
				}
			}
			
		}
		return termCounters;
	}
}
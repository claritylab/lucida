package info.ephyra.answerselection.filters;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.querygeneration.Query;
import info.ephyra.search.Result;
import info.ephyra.util.FileCache;
import info.ephyra.util.HTMLConverter;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.HashSet;

/**
 * <p>A filter that fetches web documents that contain the given search engine
 * snippets.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-24
 */
public class WebDocumentFetcherFilter extends Filter {
	/** Forbidden document types. */
	private static final String FORBIDDEN_DOCS = "(DOC|PDF|PS)";
	
	/** Maximum number of documents to fetch. */
	private static final int MAX_DOCS = 200;
	/** Maximum number of documents fetched in parallel. */
	private static final int MAX_PENDING = 60;
	
	/** Enable caching of web documents. */
	private static final boolean CACHING = true;
	/** Cache directory where web documents are stored. */
	private static final String CACHE_DIR = "cache/docs";

	/** Documents fetched by the <code>WebDocumentFetcher</code> threads. */
	private ArrayList<Result> docs;
	/** Number of active <code>WebDocumentFetcher</code> threads. */
	private int pending;
	
	/**
	 * Delays the main thread until all documents have been fetched.
	 */
	private void waitForDocs() {
		synchronized (docs) {
			while (pending > 0)
				try {
					docs.wait();
				} catch (InterruptedException e) {}
		}
	}
	
	/**
	 * Delays a thread until there are less than MAX_PENDING pending fetchers.
	 */
	public void waitForPending() {
		synchronized (docs) {
			while (pending >= MAX_PENDING)
				try {
					docs.wait();
				} catch (InterruptedException e) {}
		}
	}
	
	/**
	 * Increments the number of pending fetchers by 1.
	 */
	public void incPending() {
		synchronized (docs) {
			pending++;
		}
	}
	
	/**
	 * Used by the <code>WebDocumentFetcher</code> threads to return the
	 * documents.
	 * 
	 * @param doc document that contains a snippet
	 * @param cached flag indicating that the document was fetched from the
	 *               search engine cache
	 */
	public void addDoc(Result doc, boolean cached) {
		synchronized (docs) {
			if (doc != null) {
				docs.add(doc);
				
				// if caching is enabled and the document is not from the search
				// engine cache, write document to local cache
				if (CACHING && !cached) {
					FileCache cache = new FileCache(CACHE_DIR);
					cache.write(doc.getDocID(), new String[] {doc.getAnswer()});
				}
			}
			
			pending--;
			docs.notify();  // signal that the fetcher is done
		}
	}
	
	/**
	 * Fetches the top <code>MAX_DOCS</code> documents containing the given
	 * search engine snippets. The original snippets are dropped.
	 * 
	 * @param results array of <code>Result</code> objects containing snippets
	 * @return array of <code>Result</code> objects containing entire documents
	 */
	public Result[] apply(Result[] results) {
		// documents containing the search engine snippets
		docs = new ArrayList<Result>();
		
		// start document fetchers
		HashSet<String> urls = new HashSet<String>();
		for (Result result : results) {
			// only apply this filter to results for the semantic parsing
			// approach
			Query query = result.getQuery();
			Predicate[] ps = query.getAnalyzedQuestion().getPredicates();
			if (!query.extractWith(FactoidsFromPredicatesFilter.ID) ||
					ps.length == 0 ||
					result.getScore() > Float.NEGATIVE_INFINITY)
				continue;
			
			// if result is not a web document then just make a copy
			if (!result.getDocID().contains(":")) {
				Result newResult = result.getCopy();
				newResult.setScore(0);
				docs.add(newResult);
				continue;
			}
			
			// fetch at most MAX_DOCS documents
			if (urls.size() >= MAX_DOCS) break;
			
			String url = result.getDocID();
			// no forbidden document type
			if (url.matches("(?i).*?" + FORBIDDEN_DOCS)) continue;
			// only HTTP connections
			try {
				URLConnection conn = (new URL(url)).openConnection();
				if (!(conn instanceof HttpURLConnection)) continue;
			} catch (IOException e) {
				continue;
			}
			// no duplicate document
			if (!urls.add(url)) continue;
			
			// if caching is enabled, try to read document from cache
			if (CACHING) {
				FileCache cache = new FileCache(CACHE_DIR);
				String[] entries = cache.read(url);
				
				if (entries != null) {
					StringBuilder sb = new StringBuilder();
					for (String entry : entries) {
						sb.append(entry);
						sb.append("\n");
					}
					String docText = sb.toString();
					
					Result doc = new Result(docText, result.getQuery(), url,
							result.getHitPos());
					doc.setScore(0);
					docs.add(doc);
					
					continue;
				}
			}
			
			(new WebDocumentFetcher()).start(this, result);
		}
		
		// wait until all fetchers are done
		waitForDocs();
		
		// keep old results
		Result[] newResults = docs.toArray(new Result[docs.size()]);
		Result[] allResults = new Result[results.length + newResults.length];
		for (int i = 0; i < results.length; i++)
			allResults[i] = results[i];
		for (int i = 0; i < newResults.length; i++)
			allResults[results.length + i] = newResults[i];
		
		return allResults;
	}
}

/**
 * <p>A thread that fetches a web document containing a given search engine
 * snippet.</p>
 * 
 * <p>This class extends the class <code>Thread</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-15
 */
class WebDocumentFetcher extends Thread {
	/** Number of retries if the HTTP connection fails. */
	private static final int RETRIES = 2;
	
	/** The <code>WebDocumentFetcherFilter</code> that started this thread. */
	private WebDocumentFetcherFilter filter;
	/** The search engine snippet. */
	private Result snippet;
	
	/**
	 * Sets the calling filter and the snippet and starts the thread.
	 * 
	 * @param filter the calling <code>WebDocumentFetcherFilter</code>
	 * @param snippet search engine snippet
	 */
	public void start(WebDocumentFetcherFilter filter, Result snippet) {
		this.filter = filter;
		this.snippet = snippet;
		
		// wait until there are less than MAX_PENDING pending fetchers
		filter.waitForPending();
		
		start();
		
		// one more pending fetcher
		filter.incPending();
	}
	
	/**
	 * Fetches the document text and returns it to the
	 * <code>WebDocumentFetcherFilter</code>.
	 */
	public void run() {
		// fetch document text, retry up to RETRIES times
		String docText = null;
		int retries = RETRIES;
		boolean cached = false;
		do {
			// fetch document and convert to plain text
			try {
				docText = HTMLConverter.url2text(snippet.getDocID());
				
				if (docText == null)
					MsgPrinter.printHttpError("Document " +
							snippet.getDocID() + " not available.");
			} catch (SocketTimeoutException e) {
				docText = null;
				MsgPrinter.printHttpError("Connection to " +
						snippet.getDocID() + " timed out.");
			}
			
			retries--;
			
			// retrieve cached document if original document unavailable
			if (docText == null && retries < 0 &&
					snippet.getCacheID() != null &&
					!snippet.getCacheID().equals(snippet.getDocID())) {
				MsgPrinter.printErrorMsg("\nCould not fetch original source, " +
						"trying cached source instead...");
				snippet.setDocID(snippet.getCacheID());
				retries = RETRIES;
				cached = true;
			}
		} while (docText == null && retries >= 0);
		
		// pass document to WebDocumentFetcherFilter
		if (docText != null) {
			Result doc = new Result(docText, snippet.getQuery(),
					snippet.getDocID(), snippet.getHitPos());
			doc.setScore(0);
			filter.addDoc(doc, cached);
		} else {
			MsgPrinter.printErrorMsg("\nCould not fetch document.");
			filter.addDoc(null, cached);
//			System.exit(1);
		}
	}
}

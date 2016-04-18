package info.ephyra.search.searchers;

import info.ephyra.io.MsgPrinter;
import info.ephyra.search.Result;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import lemurproject.indri.ParsedDocument;
import lemurproject.indri.QueryEnvironment;
import lemurproject.indri.ScoredExtentResult;

/**
 * <p>A <code>KnowledgeMiner</code> that deploys the Indri IR system to
 * search a local text corpus. In contrast to <code>IndriKM</code>, whole
 * documents are returned instead of paragraphs.</p>
 * 
 * <p>It runs as a separate thread, so several queries can be performed in
 * parallel.</p>
 * 
 * <p>This class extends the class <code>KnowledgeMiner</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-26
 */
public class IndriDocumentKM extends KnowledgeMiner {
	/** Maximum total number of search results. */
	private static final int MAX_RESULTS_TOTAL = 20;
	/** Maximum number of search results per query. */
	private static final int MAX_RESULTS_PERQUERY = 20;
	/** Maximum number of documents fetched at a time. */
	private static final int MAX_DOCS = 20;
	/**
	 * <p>Regular expression that matches characters that cause problems in
	 * Indri queries and thus should be removed from query strings.</p>
	 * 
	 * <p>Indri allows the following characters:
	 * <ul>
	 * <li>'\u0080'..'\u00ff'</li>
	 * <li>'a'..'z'</li>
	 * <li>'A'..'Z'</li>
	 * <li>'0'..'9'</li>
	 * <li>'_'</li>
	 * <li>'-'</li>
	 * <li>'.' (only allowed if in between digits)</li>
	 * <li>whitespaces</li>
	 * <li>'"'</li>
	 * </ul>
	 * However, for some of the special characters Indri fails to retrieve
	 * results and therefore they are excluded.
	 * </p>
	 */
	private static final String FORBIDDEN_CHAR = "[^\\w\\.\\s\"]";
	
	/** Directories of Indri indices. */
	private String[] indriDirs;
	/** URLs of Indri servers. */
	private String[] indriUrls;
	
	/**
	 * Returns a representation of the query string that is suitable for Indri.
	 * 
	 * @param qs query string
	 * @return query string for Indri
	 */
	public static String transformQueryString(String qs) {
		// drop characters that are not properly supported by Indri
		// ('.' is only allowed in between digits)
		qs = qs.replaceAll("&\\w++;", " ");
		qs = qs.replaceAll(FORBIDDEN_CHAR, " ");
		String dotsRemoved = "";
		for (int i = 0; i < qs.length(); i++)
			if (qs.charAt(i) != '.' ||
				(i > 0 && i < qs.length() - 1 &&
				 Character.isDigit(qs.charAt(i - 1)) &&
				 Character.isDigit(qs.charAt(i + 1))))
				 dotsRemoved += qs.charAt(i);
		qs = dotsRemoved;
		
		// replace ... OR ... by #or(... ...)
		Matcher m = Pattern.compile(
			"((\\([^\\(\\)]*+\\)|\\\"[^\\\"]*+\\\"|[^\\s\\(\\)]++) OR )++" +
			"(\\([^\\(\\)]*+\\)|\\\"[^\\\"]*+\\\"|[^\\s\\(\\)]++)").matcher(qs);
		while (m.find())
			qs = qs.replace(m.group(0), "#or(" + m.group(0) + ")");
		qs = qs.replace(" OR", "");
		
		// replace ... AND ... by #combine(... ...)
		m = Pattern.compile(
			"((\\([^\\(\\)]*+\\)|\\\"[^\\\"]*+\\\"|[^\\s\\(\\)]++) AND )++" +
			"(\\([^\\(\\)]*+\\)|\\\"[^\\\"]*+\\\"|[^\\s\\(\\)]++)").matcher(qs);
		while (m.find())
			qs = qs.replace(m.group(0), "#combine(" + m.group(0) + ")");
		qs = qs.replace(" AND", "");
		
		// replace "..." by #1(...)
		m = Pattern.compile("\"([^\"]*+)\"").matcher(qs);
		while (m.find())
			qs = qs.replace(m.group(0), "#1(" + m.group(1) + ")");
		
		return qs;
	}
	
	/**
	 * Creates a new Indri knowledge miner and sets the directories of indices
	 * or the URLs of servers.
	 * 
	 * @param locations directories of indices or URLs of servers
	 * @param isServers <code>true</code> iff the first parameter provides URLs
	 *                  of servers
	 */
	public IndriDocumentKM(String[] locations, boolean isServers) {
		if (isServers) indriUrls = locations;
		else indriDirs = locations;
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
	 * Queries the Indri indices or servers and returns an array containing up
	 * to <code>MAX_RESULTS_PERQUERY</code> search results.
	 * 
	 * @return Indri search results
	 */
	protected Result[] doSearch() {
		try {
			// create query environment
			QueryEnvironment env = new QueryEnvironment();
			
		    // add Indri indices or servers
			if (indriDirs != null && indriDirs.length > 0) {
				for (String indriDir : indriDirs) env.addIndex(indriDir);
			} else if (indriUrls != null && indriUrls.length > 0) {
				for (String indriUrl : indriUrls) env.addServer(indriUrl);
			} else {
				MsgPrinter.printErrorMsg("Directories of Indri indices or " +
						"URLs of Indri servers required.");
				System.exit(1);
			}
		    
		    // run an Indri query, returning up to MAX_RESULTS_PERQUERY results
		    ScoredExtentResult[] results =
		    	env.runQuery(transformQueryString(query.getQueryString()),
		    				 MAX_RESULTS_PERQUERY);
			
			// get documents and document numbers
			String[] docs = new String[results.length];
			for (int i = 0; i < results.length; i += MAX_DOCS) {
				// fetch MAX_DOCS documents at a time (for memory efficiency)
				ScoredExtentResult[] partResults =
					new ScoredExtentResult[Math.min(MAX_DOCS, results.length - i)];
				for (int j = i; j < i + partResults.length; j++)
					partResults[j-i] = results[j];
				
				ParsedDocument[] documents = env.documents(partResults);
				
				for (int j = 0; j < partResults.length; j++)
					docs[j+i] = documents[j].text;
			}
			String[] docNos = env.documentMetadata(results, "docno");
		    
		    // close query environment
		    env.close();
			
			// return results
			return getResults(docs, docNos, false);
		} catch (Exception e) {
			MsgPrinter.printSearchError(e);  // print search error message
			
			MsgPrinter.printErrorMsg("\nSearch failed.");
			System.exit(1);
			
			return null;
		}
	}
	
	/**
	 * Returns a new instance of <code>IndriDocumentKM</code>. A new instance is
	 * created for each query.
	 * 
	 * @return new instance of <code>IndriDocumentKM</code>
	 */
	public KnowledgeMiner getCopy() {
		if (indriDirs != null)
			return new IndriDocumentKM(indriDirs, false);
		else
			return new IndriDocumentKM(indriUrls, true);
	}
}

package info.ephyra.search;

import info.ephyra.answerselection.filters.HitPositionSorterFilter;
import info.ephyra.querygeneration.Query;
import info.ephyra.search.searchers.KnowledgeAnnotator;
import info.ephyra.search.searchers.KnowledgeMiner;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

/**
 * <p>The <code>Search</code> component queries several unstructured and
 * (semi)structured knowledge sources in parallel and aggregate the results.</p>
 * 
 * <p>Queries are instances of the class
 * <code>info.ephyra.querygeneration.Query</code>, results are instances of the
 * <code>Result</code> class in this package.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-29
 */
public class Search {
	/** The maximum number of parallel queries. */
	private static final int MAX_PENDING = 30;
	
	/**
	 * <code>KnowledgeAnnotators</code> used to query (semi)structured knowledge
	 * sources.
	 */
	private static ArrayList<KnowledgeAnnotator> kas =
		new ArrayList<KnowledgeAnnotator>();
	/**
	 * <code>KnowledgeMiners</code> used to query unstructured knowledge
	 * sources.
	 */
	private static ArrayList<KnowledgeMiner> kms =
		new ArrayList<KnowledgeMiner>();
	/** Results from different searches are aggregated in this field. */
	private static ArrayList<Result> results;
	/** Number pending of queries. */
	private static int pending;
	
	/**
	 * Searches the (semi)structured knowledge sources.
	 * 
	 * @param query query to be processed
	 */
	private static void queryKAs(Query query) {
		for (int i = 0; i < kas.size(); i++)
			kas.get(i).start(query);
	}
	
	/**
	 * Searches the unstructured knowledge sources.
	 * 
	 * @param query query to be processed
	 */
	private static void queryKMs(Query query) {
		for (int i = 0; i < kms.size(); i++)
			kms.get(i).start(query);
	}
	
	/**
	 * Delays the main thread until all queries have been completed.
	 */
	private static void waitForResults() {
		synchronized (results) {
			while (pending > 0)
				try {
					results.wait();
				} catch (InterruptedException e) {}
		}
	}
	
	/**
	 * Drops duplicates among results from <code>KnowledgeMiners</code>.
	 * 
	 * @param results results with duplicates
	 * @return results without duplicates
	 */
	private static ArrayList<Result> dropDuplicates(ArrayList<Result> results) {
		// sort results by their hit positions in ascending order
		Result[] sorted = results.toArray(new Result[results.size()]);
		sorted = (new HitPositionSorterFilter()).apply(sorted);
		
		Set<Result> noDups = new HashSet<Result>();
		ArrayList<Result> remaining = new ArrayList<Result>();
		for (Result result : sorted)
			if (result.getScore() != Float.NEGATIVE_INFINITY ||
					noDups.add(result))
				remaining.add(result);
		
		return remaining;
	}
	
	/**
	 * Registers a <code>KnowledgeAnnotator</code> for a (semi)structured
	 * knowledge source.
	 * 
	 * @param ka <code>KnowledgeAnnotator</code> to add
	 */
	public static void addKnowledgeAnnotator(KnowledgeAnnotator ka) {
		kas.add(ka);
	}
	
	/**
	 * Registers a <code>KnowledgeMiner</code> for an unstructured knowledge
	 * source.
	 * 
	 * @param km <code>KnowledgeMiner</code> to add
	 */
	public static void addKnowledgeMiner(KnowledgeMiner km) {
		kms.add(km);
	}
	
	/**
	 * Unregisters all <code>KnowledgeAnnotators</code>.
	 */
	public static void clearKnowledgeAnnotators() {
		kas.clear();
	}
	
	/**
	 * Unregisters all <code>KnowledgeMiners</code>.
	 */
	public static void clearKnowledgeMiners() {
		kms.clear();
	}
	
	/**
	 * Sends several alternative queries to all the searchers that have been
	 * registered and returns the aggregated results.
	 * 
	 * @param queries queries to be processed
	 * @return results returned by the searchers
	 */
	public static Result[] doSearch(Query[] queries) {
		results = new ArrayList<Result>();
		pending = 0;
		System.out.println("queries.length == " + queries.length);
		
		// send only the first query to the KnowledgeAnnotators
		if (queries.length > 0) queryKAs(queries[0]);
		
		// send all queries to the KnowledgeMiners
		for (Query query : queries) queryKMs(query);
		
		// wait until all queries have been completed
		waitForResults();
		
		// drop duplicates among results from KnowledgeMiners
		results = dropDuplicates(results);
		
		return results.toArray(new Result[results.size()]);
	}
	
	/**
	 * Delays a thread until there are less than MAX_PENDING pending queries.
	 */
	public static void waitForPending() {
		synchronized (results) {
			while (pending >= MAX_PENDING)
				try {
					results.wait();
				} catch (InterruptedException e) {}
		}
	}
	
	/**
	 * Increments the number of pending queries by 1.
	 */
	public static void incPending() {
		synchronized (results) {
			pending++;
		}
	}
	
	/**
	 * Used by <code>Searchers</code> to return the results found in the
	 * knowledge sources.
	 * 
	 * @param results results found in the knowledge sources
	 */
	public static void addResults(Result[] results) {
		synchronized (Search.results) {
			// Error handling for case when searcher finds no results
			if (results == null) {
				System.out.println("Search returned no results " +
					"for this query");
			} else {
				for (Result result : results) {
					Search.results.add(result);
				}
			}
			pending--;
			Search.results.notify();  // signal that the query is completed
		}
	}
}

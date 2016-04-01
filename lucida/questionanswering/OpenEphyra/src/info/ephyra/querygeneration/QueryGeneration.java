package info.ephyra.querygeneration;

import info.ephyra.io.Logger;
import info.ephyra.io.MsgPrinter;
import info.ephyra.querygeneration.generators.QueryGenerator;
import info.ephyra.questionanalysis.AnalyzedQuestion;

import java.util.ArrayList;

/**
 * Generates one or more <code>Queries</code> from a previously analyzed
 * question by applying a set of <code>QueryGenerators</code>.
 * 
 * @author Nico Schlaefer
 * @version 2006-30-10
 */
public class QueryGeneration {
	/** <code>QueryGenerator</code> objects used to generate the queries. */
	private static ArrayList<QueryGenerator> queryGenerators =
		new ArrayList<QueryGenerator>();
	
	/**
	 * Registers a <code>QueryGenerator</code>.
	 * 
	 * @param queryGenerator <code>QueryGenerator</code> to add
	 */
	public static void addQueryGenerator(QueryGenerator queryGenerator) {
		queryGenerators.add(queryGenerator);
	}
	
	/**
	 * Unregisters all <code>QueryGenerators</code>.
	 */
	public static void clearQueryGenerators() {
		queryGenerators.clear();
	}
	
	/**
	 * Applies the <code>QueryGenerators</code> to an analysed question and
	 * returns one or more queries that can be passed to the search module.
	 * 
	 * @param aq analyzed question
	 * @return <code>Query</code> objects
	 */
	public static Query[] getQueries(AnalyzedQuestion aq) {
		ArrayList<Query> results = new ArrayList<Query>();
		
		// apply query generators
		for (QueryGenerator queryGenerator : queryGenerators) {
			Query[] queries = queryGenerator.generateQueries(aq);
			for (Query query : queries) results.add(query);
		}
		
		// print and log query strings
		Query[] queries = results.toArray(new Query[results.size()]);
		MsgPrinter.printQueryStrings(queries);
		Logger.logQueryStrings(queries);
		
		return queries;
	}
}

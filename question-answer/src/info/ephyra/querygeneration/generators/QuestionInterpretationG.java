package info.ephyra.querygeneration.generators;

import info.ephyra.answerselection.filters.AnswerPatternFilter;
import info.ephyra.answerselection.filters.AnswerTypeFilter;
import info.ephyra.answerselection.filters.FactoidsFromPredicatesFilter;
import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.QuestionInterpretation;

import java.util.ArrayList;

/**
 * <p>The <code>QuestionInterpretationG</code> generates queries from question
 * interpretations. The query string is built from the TARGET, the CONTEXT and
 * the keywords in the question (for question answering only) or the answer
 * string (for pattern learning only).</p>
 * 
 * <p>This class extends the class <code>QueryGenerator</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-11
 */
public class QuestionInterpretationG extends QueryGenerator {
	/** The score assigned to queries created from question interpretations. */
	private static final float SCORE = 2;
	/** Answer extraction techniques for this query type. */
	private static final String[] EXTRACTION_TECHNIQUES = {
		AnswerTypeFilter.ID,
		AnswerPatternFilter.ID,
		FactoidsFromPredicatesFilter.ID
	};
	
	/**
	 * Creates queries from question interpretations. The query string is the
	 * concatenation of the target object, any context objects and the keywords
	 * in the question.
	 * 
	 * @param aq analyzed question
	 * @return <code>Query</code> objects
	 */
	public Query[] generateQueries(AnalyzedQuestion aq) {
		ArrayList<Query> queries = new ArrayList<Query>();
		
		QuestionInterpretation[] qis = aq.getInterpretations();
		String[] kws = aq.getKeywords();
		for (QuestionInterpretation qi : qis) {
			// create query string
			String queryString = queryString(qi.getTarget(),
											 qi.getContext(), kws);
			
			// create query, set answer types and question interpretation
			Query query = new Query(queryString, aq, SCORE);
			query.setExtractionTechniques(EXTRACTION_TECHNIQUES);
			query.setInterpretation(qi);
			
			queries.add(query);
		}
		
		return queries.toArray(new Query[queries.size()]);
	}
	
	/**
	 * Creates a query string from the interpretation of a question and the
	 * keywords in the question (for question answering only) or an answer
	 * string (for pattern learning only).
	 * 
	 * @param target TARGET object
	 * @param context CONTEXT objects
	 * @param kws keywords in the question string (for question answering) or
	 * 			  answer string (for pattern learning)
	 * @return query string
	 */
	public String queryString(String target, String[] context, String kws[]) {
		// include the TARGET object in the query string
		String queryString = "\"" + target + "\"";
		
		// include the CONTEXT objects (if any) in the query string
		for (String ct : context) queryString += " \"" + ct + "\"";
		
		// include the keywords (for question answering) or the answer string
		// (for pattern learning) in the query string
		for (String kw : kws) queryString += " " + kw;
		
		return queryString;
	}
}

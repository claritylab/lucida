package info.ephyra.querygeneration.generators;

import info.ephyra.answerselection.filters.AnswerPatternFilter;
import info.ephyra.answerselection.filters.AnswerTypeFilter;
import info.ephyra.answerselection.filters.FactoidsFromPredicatesFilter;
import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.Term;
import info.ephyra.questionanalysis.TermExpander;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Map;

/**
 * <p>The <code>BagOfTermsG</code> query generator creates a query from the
 * terms in the question string.</p>
 * 
 * <p>This class extends the class <code>QueryGenerator</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-11
 */
public class BagOfTermsG extends QueryGenerator {
	/** Score assigned to "bag of terms" queries. */
	private static final float SCORE = 1.5f;
	/** Answer extraction techniques for this query type. */
	private static final String[] EXTRACTION_TECHNIQUES = {
		AnswerTypeFilter.ID,
		AnswerPatternFilter.ID,
		FactoidsFromPredicatesFilter.ID
	};
	
	/**
	 * Forms a query string from the terms and individual keywords.
	 * 
	 * @param terms terms in the question
	 * @param kws keywords in the question
	 * @return query string
	 */
	private String getQueryString(Term[] terms, String[] kws) {
		ArrayList<String> phraseL = new ArrayList<String>();
		HashSet<String> normSet = new HashSet<String>();
		
		// get terms
		for (Term term : terms) {
			String text = term.getText();
			if (normSet.add(StringUtils.normalize(text))) {
				// add quotation marks for compound phrases
				if (text.matches(".*?\\s.*+"))
					text = "\"" + text + "\"";
				
				String phrase = text;
				
				// append expansions
				Map<String, Double> expMap = term.getExpansions();
				expMap = TermExpander.reduceExpansionsQuery(expMap, true);
				if (expMap != null && expMap.size() > 0) {
					String[] expansions =
						expMap.keySet().toArray(new String[expMap.size()]);
					phrase = "(" + phrase;
					for (String expansion : expansions) {
						// add quotation marks for compound phrases
						if (expansion.matches(".*?\\s.*+"))
							expansion = "\"" + expansion + "\"";
						
						phrase += " OR " + expansion;
					}
					phrase += ")";
				}
				
				phraseL.add(phrase);
			}
		}
		
		 // get individual keywords
		// - expand keywords (not supported by Web search engines!)
//		for (Term term : terms) {
//			String phrase;
//			Map<String, Double> expMap = term.getExpansions();
//			expMap = TermExpander.reduceExpansionsQuery(expMap, true);
//			boolean newKeyword = false;  // term/expansion contains new keyword?
//			
//			if (expMap.size() == 0) {
//				String[] keywords =
//					KeywordExtractor.getKeywords(term.getText());
//				List<String> uniqueL = new ArrayList<String>();
//				for (String keyword : keywords)
//					if (normSet.add(StringUtils.normalize(keyword)))
//						uniqueL.add(keyword);
//				String[] unique = uniqueL.toArray(new String[uniqueL.size()]);
//				phrase = StringUtils.concatWithSpaces(unique);
//				if (unique.length > 0) newKeyword = true;
//			} else {
//				// form AND query from keywords in term
//				String[] keywords =
//					KeywordExtractor.getKeywords(term.getText());
//				String and = StringUtils.concat(keywords, " AND ");
//				if (keywords.length > 1)
//					and = "(" + and + ")";
//				for (String keyword : keywords)
//					if (normSet.add(StringUtils.normalize(keyword)))
//						newKeyword = true;
//				
//				phrase = and;
//				
//				// append expansions
//				if (expMap != null && expMap.size() > 0) {
//					String[] expansions =
//						expMap.keySet().toArray(new String[expMap.size()]);
//					phrase = "(" + phrase;
//					for (String expansion : expansions) {
//						// form AND query from keywords in expansion
//						keywords = KeywordExtractor.getKeywords(expansion);
//						and = StringUtils.concat(keywords, " AND ");
//						if (keywords.length > 1)
//							and = "(" + and + ")";
//						for (String keyword : keywords)
//							if (normSet.add(StringUtils.normalize(keyword)))
//								newKeyword = true;
//						
//						phrase += " OR " + and;
//					}
//					phrase += ")";
//				}
//			}
//			
//			// add phrase to the query if the term or one of its expansions has
//			// multiple tokens and thus the keyword query is different from the
//			// term query
//			if (newKeyword) phraseL.add(phrase);
//		}
		// - do not expand keywords
//		for (String kw : kws)
//			if (normSet.add(StringUtils.normalize(kw)))
//				phraseL.add(kw);
		
		// build query string
		String[] phrases = phraseL.toArray(new String[phraseL.size()]);
		String queryString = StringUtils.concatWithSpaces(phrases);
		
		return queryString;
	}
	
	/**
	 * Generates a "bag of terms" query from the terms in the question string.
	 * 
	 * @param aq analyzed question
	 * @return <code>Query</code> objects
	 */
	public Query[] generateQueries(AnalyzedQuestion aq) {
		// only generate a query if the answer type is known, predicates could
		// be extracted or the question is not a factoid question
		String[] ats = aq.getAnswerTypes();
		Predicate[] ps = aq.getPredicates();
		if (ats.length == 0 && ps.length == 0 && aq.isFactoid())
			return new Query[0];
		
		// create query string
		Term[] terms = aq.getTerms();
		String[] kws = aq.getKeywords();
		String queryString = getQueryString(terms, kws);
		
		// create query, set answer types
		Query[] queries = new Query[1];
		queries[0] = new Query(queryString, aq, SCORE);
		queries[0].setExtractionTechniques(EXTRACTION_TECHNIQUES);
		
		return queries;
	}
}

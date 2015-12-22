package info.ephyra.querygeneration.generators;

import info.ephyra.answerselection.filters.AnswerPatternFilter;
import info.ephyra.answerselection.filters.AnswerTypeFilter;
import info.ephyra.answerselection.filters.FactoidsFromPredicatesFilter;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.Term;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.HashSet;

/**
 * <p>The <code>PredicateG</code> query generator creates queries from the
 * predicates in the question string.</p>
 * 
 * <p>This class extends the class <code>QueryGenerator</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-11
 */
public class PredicateG extends QueryGenerator {
	/** Score assigned to queries created from predicates. */
	private static final float SCORE = 2;
	/** Words that should not be part of a query string. */
	private static final String IGNORE = "(names?|give|tell|list)";
	/** Answer extraction techniques for this query type. */
	private static final String[] EXTRACTION_TECHNIQUES = {
		AnswerTypeFilter.ID,
		AnswerPatternFilter.ID,
		FactoidsFromPredicatesFilter.ID
	};
	
	/**
	 * Forms a query string from the predicates, terms and individual keywords.
	 * 
	 * @param predicates predicates in the question
	 * @param terms terms in the question
	 * @param kws keywords in the question
	 * @return query string
	 */
	public String getQueryString(Predicate[] predicates, Term[] terms,
			String[] kws) {
		ArrayList<String> phraseL = new ArrayList<String>();
		HashSet<String> normSet = new HashSet<String>();
		
		// get predicate verbs and arguments
		for (Predicate predicate : predicates) {
			String[] verbArgs = predicate.getVerbArgs();
			for (String verbArg : verbArgs) {
				String[] parts = verbArg.split("\t");
				for (String part : parts)
					if (!part.matches("(?i)" + IGNORE) &&  // no words in IGNORE
							!FunctionWords.lookup(part) &&  // no function words
							normSet.add(StringUtils.normalize(part))) {
						// drop quotation marks
						String noQuotes = part.replace("\"", "");
						// add quotation marks for compound phrases
						if (noQuotes.matches(".*?\\s.*+"))
							noQuotes = "\"" + noQuotes + "\"";
						
						String phrase = noQuotes;
						
//						// append expansions
//						Map<String, Double> expMap =
//							TermExpander.expandPhrase(part, terms);
//						if (expMap.size() > 0) {
//							String[] expansions =
//								expMap.keySet().toArray(new String[expMap.size()]);
//							phrase = "(" + phrase;
//							for (String expansion : expansions) {
//								// drop quotation marks
//								expansion = expansion.replace("\"", "");
//								// add quotation marks for compound phrases
//								if (expansion.matches(".*?\\s.*+"))
//									expansion = "\"" + expansion + "\"";
//								
//								phrase += " OR " + expansion;
//							}
//							phrase += ")";
//						}
						
						phraseL.add(phrase);
					}
			}
		}

		// get terms
//		for (Term term : terms) {
//			String text = term.getText();
//			if (normSet.add(StringUtils.normalize(text))) {
//				// add quotation marks for compound phrases
//				if (text.matches(".*?\\s.*+"))
//					text = "\"" + text + "\"";
//				
//				String phrase = text;
//				
//				// append expansions
//				Map<String, Double> expMap = term.getExpansions();
//				expMap = TermExpander.reduceExpansionsQuery(expMap, true);
//				if (expMap != null && expMap.size() > 0) {
//					String[] expansions =
//						expMap.keySet().toArray(new String[expMap.size()]);
//					phrase = "(" + phrase;
//					for (String expansion : expansions) {
//						// add quotation marks for compound phrases
//						if (expansion.matches(".*?\\s.*+"))
//							expansion = "\"" + expansion + "\"";
//						
//						phrase += " OR " + expansion;
//					}
//					phrase += ")";
//				}
//				
//				phraseL.add(phrase);
//			}
//		}
		
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
		
		// include context keywords in the query string
		for (String kw : kws)
			if (!StringUtils.equalsCommonNorm(queryString, kw))
				queryString += " " + kw;
		
		return queryString;
	}
	
	/**
	 * Generates queries from predicate-argument structures extracted from the
	 * question string.
	 * 
	 * @param aq analyzed question
	 * @return <code>Query</code> objects
	 */
	public Query[] generateQueries(AnalyzedQuestion aq) {
		// only generate a query if predicates could be extracted
		Predicate[] ps = aq.getPredicates();
		if (ps.length == 0) return new Query[0];
		
		// create query string
		Term[] terms = aq.getTerms();
		String[] kws = aq.getKeywords();
		String queryString = getQueryString(ps, terms, kws);
		
		// create query, set answer types and predicates
		Query[] queries = new Query[1];
		queries[0] = new Query(queryString, aq, SCORE);
		queries[0].setExtractionTechniques(EXTRACTION_TECHNIQUES);
		
		return queries;
	}
}

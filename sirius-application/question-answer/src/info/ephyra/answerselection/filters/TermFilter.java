package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.search.Result;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

/**
 * <p>Drops answer candidates for the 'other' questions in TREC 13-16 that
 * contain answers to previous factoid or list questions.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class TermFilter extends Filter {
	private final static HashSet<String> previousResultTerms = new HashSet<String>();
	
	/**
	 * Sets the previous results so that answer candidates for an 'other'
	 * question can be filtered out if they are too close to one of them.
	 * 
	 * @param results the results for factoid and list questions
	 *                (<code>null</code> will clear previous results)
	 */
	public static void setPreviousResultsTerms(String[] results) {
		previousResultTerms.clear();
		if (results != null) for (String r : results) previousResultTerms.add(r);
	}
	
	/**
	 * Filters out snippets that are likely to contain the answer to a
	 * previously asked factoid or list question. This is to prevent wasting
	 * result length with information redundant to the factoid and list
	 * questions.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return filtered array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// raw results returned by the searchers
		HashMap<String, Integer> termCounters = new HashMap<String, Integer>();
		for (Result r : results) {
//			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String text = r.getAnswer();
				
				//	tokenize and tag sentence
				String[] sentence = NETagger.tokenize(text);
				
				//	scan sentence for NPs
				for (int i = 0; i < sentence.length; i++) {
					String term = SnowballStemmer.stem(sentence[i].toLowerCase());
					if (term.length() > 1) {
						Integer count = (termCounters.containsKey(term) ? termCounters.get(term) : new Integer(0));
						termCounters.put(term, new Integer(count.intValue() + 1));
					}
				}
//			}
		}
		
		
		ArrayList<Result> rawResults = new ArrayList<Result>();
		
		HashSet<String> found = new HashSet<String>();
		found.addAll(previousResultTerms);
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String text = r.getAnswer();
				
				//	tokenize and tag sentence
				String[] sentence = NETagger.tokenize(text);
				
				int numberOfTerms = 0;
				int numberOfKeyTerms = 0;
				HashSet<String> resFound = new HashSet<String>();
				
				//	scan sentence for NPs
				for (int i = 0; i < sentence.length; i++) {
					String term = SnowballStemmer.stem(sentence[i].toLowerCase());
//					String term = sentence[i].toLowerCase();
					
					if (!found.contains(term) && !resFound.contains(term)) {
						resFound.add(term);
						
						//	count only terms that are contained in at least one percent of the results
						Integer count = (termCounters.containsKey(term) ? termCounters.get(term) : new Integer(0));
						if (count.intValue() > (results.length / 100))
							if ((term.length() > 1) && !StringUtils.isSubsetKeywords(term, r.getQuery().getAnalyzedQuestion().getQuestion()) && !FunctionWords.lookup(term)) numberOfKeyTerms ++;
							
						if ((term.length() > 1) && !StringUtils.isSubsetKeywords(term, r.getQuery().getAnalyzedQuestion().getQuestion()) && !FunctionWords.lookup(term)) numberOfTerms ++;
					}
				}
				
//				if ((numberOfTerms > (1 + results.length / 100)) && (numberOfKeyTerms > Math.floor(Math.sqrt(results.length / 100)))) {
//30.50% freeze	if ((numberOfTerms > (1 + results.length / 100)) && (numberOfKeyTerms != 0)) {
				if (numberOfTerms != 0) {
//					found.addAll(resFound);
//					r.incScore(numberOfTerms * (((float) results.length) / ((float) sentence.length)));
					rawResults.add(r);
				}
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

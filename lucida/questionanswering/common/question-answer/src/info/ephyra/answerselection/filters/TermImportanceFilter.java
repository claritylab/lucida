package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.search.Result;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.HashMap;

/**
 * <p>Prefers answer candidates for definitional questions that contain common
 * keywords.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class TermImportanceFilter extends Filter {
	/**
	 * Increments the score of each result snippet for each word in it according
	 * to the number of result snippets containing this particular word. This is
	 * sort of a centrality measure, which favors snippets that provide
	 * information given frequently and thus likely to be more important with
	 * regard to the target.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return filtered array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// raw results returned by the searchers
		HashMap<String, Integer> termCounters = new HashMap<String, Integer>();
		ArrayList<Result> rawResults = new ArrayList<Result>();
		
		int lengthSum = 0;
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String text = r.getAnswer();
				
				//	tokenize and tag sentence
				String[] sentence = NETagger.tokenize(text);
				lengthSum += sentence.length;
				
				//	scan sentence for NPs
				for (int i = 0; i < sentence.length; i++) {
					String term = SnowballStemmer.stem(sentence[i].toLowerCase());
					if (term.length() > 1) {
						Integer count = (termCounters.containsKey(term) ? termCounters.get(term) : new Integer(0));
						termCounters.put(term, new Integer(count.intValue() + 1));
					}
				}
			}
		}
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String text = r.getAnswer();
				
				//	tokenize sentence
				String[] sentence = NETagger.tokenize(text);
				float importance = 0;
				
				//	scan sentence for NPs
				for (int i = 0; i < sentence.length; i++) {
					String term = sentence[i];
					if ((term.length() > 1) && !StringUtils.isSubsetKeywords(term, r.getQuery().getAnalyzedQuestion().getQuestion()) && !FunctionWords.lookup(term)) {
//					if (term.length() > 1) {
						term = SnowballStemmer.stem(term.toLowerCase());
						Integer count = (termCounters.containsKey(term) ? termCounters.get(term) : new Integer(0));
						if (count.intValue() > Math.floor(Math.sqrt(results.length / 100)))
							importance += count.intValue();
//						if (count.intValue() > (results.length / 100))
//							importance += (((float) count.intValue()) / ((float) results.length));
					}
				}
				
				if (importance > 0) {
					r.incScore(importance);
					rawResults.add(r);
//					r.incScore((float) Math.sqrt(importance));
				}
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

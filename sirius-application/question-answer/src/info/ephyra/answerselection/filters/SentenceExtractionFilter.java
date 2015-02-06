package info.ephyra.answerselection.filters;

import info.ephyra.nlp.LingPipe;
import info.ephyra.search.Result;

import java.util.ArrayList;

/**
 * <p>Splits the answer strings of results that are not from knowledge
 * annotators into sentences and creates a new result for each sentence.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-06-03
 */
public class SentenceExtractionFilter extends Filter {
	/**
	 * Splits the answer strings of the results into sentences and creates a
	 * new result for each sentence.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		ArrayList<Result> newResults = new ArrayList<Result>();
		
		for (Result result : results) {
			// do not apply this filter to results from knowledge annotators
			if (result.getScore() == Float.POSITIVE_INFINITY) {
				newResults.add(result);
				continue;
			}
			
			// split the answer string into sentences
			String answer = result.getAnswer();
			String[] sentences = LingPipe.sentDetect(answer);
			
			// create a new result for each sentence
			for (String sentence : sentences) {
				Result newResult = result.getCopy();
				result.setAnswer(sentence);
				newResults.add(newResult);
			}
		}
		
		return newResults.toArray(new Result[newResults.size()]);
	}
}

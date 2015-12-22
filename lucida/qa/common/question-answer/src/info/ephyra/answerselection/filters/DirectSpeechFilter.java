package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.ArrayList;

/**
 * <p>Drops answer candidates for definitional questions that consist of direct
 * speech.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-10
 */
public class DirectSpeechFilter extends Filter {
	/**
	 * Throws out snippets that consist of direct speech.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// raw results returned by the searchers
		ArrayList<Result> rawResults = new ArrayList<Result>();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String text = r.getAnswer();
				if (!text.startsWith("''") && !text.startsWith("´´") && !text.startsWith("``"))
					rawResults.add(r);
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

/**
 * <p>Drops unnecessary characters from answer candidates for definitional
 * questions to improve precision. This may be problematic if the exact answer
 * string has to appear in the supporting document.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter, Nico Schlaefer
 * @version 2008-02-15
 */
public class UnnecessaryCharactersFilter extends Filter {
	public Result[] apply(Result[] results) {
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String sentence = r.getAnswer();
				sentence = sentence.replaceAll("(\\'|\\\"|\\`|\\_)", "");
				r.setAnswer(sentence);
			}
		}
		
		return results;
	}
}

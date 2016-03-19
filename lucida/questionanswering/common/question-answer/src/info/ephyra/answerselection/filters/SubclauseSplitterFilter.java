package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.ArrayList;

/**
 * <p>Splits sentences into subclauses in order to facilitate the detection of
 * redundant information.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class SubclauseSplitterFilter extends Filter {
	/**
	 * Splits sentences into individual subclauses in order to facilitate
	 * subsequent filtering. The idea is that redundancy detection is easier for
	 * shorter snippets than for longer ones.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// raw results returned by the searchers
		ArrayList<Result> rawResults = new ArrayList<Result>();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String sentence = r.getAnswer();
				String[] sentences = sentence.split("(\\b(although|but|how|until|what|when|where|which|who|whom|why)\\b)");
				if (sentences.length != 0) {
					r.setAnswer(sentences[0]);
					rawResults.add(r);
					for (int s = 1; s < sentences.length; s++) {
						Result newRes = new Result(sentences[s], r.getQuery(), r.getDocID(), r.getHitPos());
						newRes.setScore(r.getScore());
						rawResults.add(newRes);
					}
				} else rawResults.add(r);
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

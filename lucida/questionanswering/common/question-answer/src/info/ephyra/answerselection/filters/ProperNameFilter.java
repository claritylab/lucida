package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.search.Result;

import java.util.ArrayList;

/**
 * <p>Drops answer candidates for definitional questions that are enumerations
 * of proper names.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class ProperNameFilter extends Filter {
	/**
	 * Filter out result snippets that contain too many proper names. This is to
	 * get rid of enumerations of named entities that happen to include the
	 * target. This might, for instance, be the track list of a compilation LP,
	 * which has a song by the target artist on it.
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
				
				//	tokenize and tag sentence
				String[] sentence = NETagger.tokenize(text);
				
				int upperCase = 0;
				int lowerCase = 0;
				
				//	scan sentence for tokens in upper case
				for (int i = 1; i < sentence.length; i++) {
					String term = sentence[i];
					if (term.matches("[A-Z]++.*+")) {
						upperCase ++;
						if (FunctionWords.lookup(term.toLowerCase())) upperCase += 2;//sentence.length;
					}
					else if (term.matches("[a-z]++.*+")) lowerCase ++;
					else if (term.matches("[0-9]++")) lowerCase ++;
				}
				
				if (upperCase < lowerCase) rawResults.add(r);
//				else System.out.println("ProperNameFilter: " + text);
				
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

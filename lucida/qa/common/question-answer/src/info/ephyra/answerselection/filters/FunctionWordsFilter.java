package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.search.Result;

/**
 * <p>A filter that drops a result if the answer string contains only function
 * words.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-03-05
 */
public class FunctionWordsFilter extends Filter {
	/**
	 * Filters a single <code>Result</code> object.
	 * 
	 * @param result result to filter
	 * @return result or <code>null</code>
	 */
	public Result apply(Result result) {
		String[] tokens = NETagger.tokenize(result.getAnswer());
		for (String token : tokens)
			if (!FunctionWords.lookup(token))
				return result;
		
		return null;
	}
}

package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.search.Result;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;

/**
 * <p>The <code>FactoidSubsetFilter</code> checks a set of factoid answers for
 * subset relations. If a factoid answer is a subset of another factoid answer
 * (i.e. its tokens are a subset of the tokens of another answer), then the
 * former is dropped and its score is tranferred to the latter.</p>
 * 
 * <p>The filter is only applied if the longer answer is a named entity that has
 * been extracted with a pattern-based or list-based tagger and thus is properly
 * chunked. This is to avoid that malformatted answers which contain additional
 * tokens (such as "1879 and") are preferred over properly chunked ones (e.g.
 * "1879").</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-03-05
 */
public class FactoidSubsetFilter extends Filter {
	/**
	 * <p>Drops results that are subsets of other results and transfers their
	 * scores to the remaining results.</p>
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return array of <code>Result</code> objects that are not subsets
	 */
	public Result[] apply(Result[] results) {
		// sort results by their scores in ascending order
		results = (new ReverseScoreSorterFilter()).apply(results);
		// sort results by their lengths in ascending order (stable)
		results = (new ResultLengthSorterFilter()).apply(results);
		
		// normalize answer strings
		String[] norms = new String[results.length];
		for (int i = 0; i < results.length; i++)
			if (results[i].getScore() != Float.POSITIVE_INFINITY &&
					results[i].getScore() != Float.NEGATIVE_INFINITY)
				norms[i] = StringUtils.normalize(results[i].getAnswer());
		
		// check for subset relations, aggregate answers
		for (int i = 0; i < results.length - 1; i++) {
			if (results[i].getScore() != Float.POSITIVE_INFINITY &&
					results[i].getScore() != Float.NEGATIVE_INFINITY)
				for (int j = results.length - 1; j > i; j--)
					if (results[j].getScore() != Float.POSITIVE_INFINITY &&
							results[j].getScore() != Float.NEGATIVE_INFINITY &&
							results[j].isNamedEntity() &&
							!NETagger.allModelType(results[j].getNeTypes()) &&
							StringUtils.isSubsetKeywords(norms[i], norms[j])) {
						// longer answer is a NE not extracted with a
						// model-based tagger
						results[j].incScore(results[i].getScore());
						results[i] = null;
						break;
					}
		}
		
		// get remaining results
		ArrayList<Result> remaining = new ArrayList<Result>();
		for (Result result : results)
			if (result != null) remaining.add(result);
		
		return remaining.toArray(new Result[remaining.size()]);
	}
}

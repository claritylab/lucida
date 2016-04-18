package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.search.Result;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;

/**
 * <p>A variant of the <code>DuplicateFilter</code> for answer candidates that
 * are text snippets.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class DuplicateSnippetFilter extends Filter {
	/**
	 * Filters duplicate results and increments the scores of the remaining
	 * results by the scores of the dropped results.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return array of <code>Result</code> objects without duplicates
	 */
	public Result[] apply(Result[] results) {
		
		// sort results by their scores in descending order
		results = (new ScoreSorterFilter()).apply(results);
		
		// return remaining results
		ArrayList<Result> rawResults = new ArrayList<Result>();
		HashSet<String> contained = new HashSet<String>();
		
		// drop duplicates
		for (Result res : results) {
			String text = res.getAnswer();
			if (text != null) {
				
				//	remove meaningless drivel
				text = text.toLowerCase().trim();
				text = text.replaceAll("(\\'|\\\"|\\`|\\_)", "");
				text = SnowballStemmer.stemAllTokens(text);
				
				//	produce and store keywords for subset elimination
				String[] tokens = NETagger.tokenize(text);
				HashSet<String> keywords = new HashSet<String>();
				for (String term : tokens)
					if ((term.length() > 1) && !FunctionWords.lookup(term))
						keywords.add(term);
				
				//	produce term string
				ArrayList<String> sortedKeywords = new ArrayList<String>(keywords);
				Collections.sort(sortedKeywords);
				StringBuffer keywordString = new StringBuffer();
				for (String term : sortedKeywords)
					keywordString.append(" " + term);
				
				//	check if same keywords contained in previous snippet
				if (contained.add(keywordString.toString().trim()))
					rawResults.add(res);
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

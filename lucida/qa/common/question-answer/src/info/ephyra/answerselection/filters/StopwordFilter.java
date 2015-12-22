package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.nlp.semantics.ontologies.WordNet;
import info.ephyra.search.Result;

/**
 * <p>A filter that drops a result if the answer string
 * <ul>
 *   <li>contains only function words and single characters (except digits)</li>
 *   <li>contains an interrogative</li>
 *   <li>contains a single bracket or quotation mark</li>
 *   <li>is an adverb</li>
 * </ul></p>
 * 
 * <p>This filter is not applied to answer strings that have been extracted with
 * a rule- or list-based NE tagger since these answers are assumed to be
 * properly formatted.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-17
 */
public class StopwordFilter extends Filter {
	/**
	 * Drops a result if the answer string contains only function words and
	 * single characters (except digits).
	 * 
	 * @param result result to filter
	 * @return result or <code>null</code>
	 */
	private Result filterFunctionWords(Result result) {
		if (result == null) return null;
		
		String[] tokens = NETagger.tokenize(result.getAnswer());
		for (String token : tokens)
			if ((token.length() > 1 || token.matches("\\d")) &&
					!FunctionWords.lookup(token))
				return result;
		
		return null;
	}
	
	/**
	 * Drops a result if the answer string contains an interrogative.
	 * 
	 * @param result result to be filtered
	 * @return result or <code>null</code>
	 */
	private Result filterInterrogatives(Result result) {
		if (result == null) return null;
		String answer = result.getAnswer();
		
		if (answer.matches("(?i).*?(^|\\W)" +
				"(what|which|when|where|who|why|how)($|\\W).*+"))
			return null;
		
		return result;
	}
	
	/**
	 * Drops a result if the answer string contains a single bracket or
	 * quotation mark.
	 * 
	 * @param result result to be filtered
	 * @return result or <code>null</code>
	 */
	private Result filterSingleBracket(Result result) {
		if (result == null) return null;
		String answer = result.getAnswer();
		
		// quotation marks
		if (answer.matches("[^\"]*?\"[^\"]*+")) return null;
		// brackets
		if (answer.matches("[^\\(]*?\\).*+") ||
			answer.matches(".*?\\([^\\)]*+")) return null;
		if (answer.matches("[^\\{]*?\\}.*+") ||
			answer.matches(".*?\\{[^\\}]*+")) return null;
		if (answer.matches("[^\\[]*?\\].*+") ||
			answer.matches(".*?\\[[^\\]]*+")) return null;
		
		return result;
	}
	
	/**
	 * Drops a result if the answer string is an adverb.
	 * 
	 * @param result result to filter
	 * @return result or <code>null</code>
	 */
	private Result filterAdverbs(Result result) {
		if (result == null) return null;
		String answer = result.getAnswer();
		
		if (answer.matches("(?i).*?ly") && WordNet.isAdverb(answer))
			return null;
		
		return result;
	}
	
	/**
	 * Filters a single <code>Result</code> object.
	 * 
	 * @param result result to filter
	 * @return result or <code>null</code>
	 */
	public Result apply(Result result) {
		// only apply the filter to factoid answers
		if (result.getScore() <= 0) return result;
		// do not apply the filter if the answer is a NE that was extracted with
		// a rule- or list-based tagger
		if (result.isNamedEntity() &&
				!NETagger.allModelType(result.getNeTypes())) return result;
		
		result = filterFunctionWords(result);
		result = filterInterrogatives(result);
		result = filterSingleBracket(result);
		result = filterAdverbs(result);
		
		return result;
	}
}

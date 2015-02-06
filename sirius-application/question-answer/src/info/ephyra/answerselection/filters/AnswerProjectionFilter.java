package info.ephyra.answerselection.filters;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.search.Result;
import info.ephyra.util.RegexConverter;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.Hashtable;

/**
 * <p>The <code>AnswerProjectionFilter</code> projects answers from the Web onto
 * a corpus.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-24
 */
public class AnswerProjectionFilter extends Filter {
	/**
	 * Answer extraction techniques in the order of their preference for answer
	 * projection.
	 */
	private static final String[] EXTRACTION_TECHNIQUES = {
		FactoidsFromPredicatesFilter.ID,
		AnswerPatternFilter.ID,
		AnswerTypeFilter.ID
	};
	
	/** Results retrieved from the corpus. */
	private Result[] resultsCorp;
	
	/**
	 * Creates the filter and sets the results retrieved from the corpus.
	 * 
	 * @param resultsCorp results retrieved from the corpus
	 */
	public AnswerProjectionFilter(Result[] resultsCorp) {
		this.resultsCorp = resultsCorp;
	}
	
	/**
	 * Checks if the answer extraction techniques used to extract the first
	 * result have a higher preference for answer projection than the techniques
	 * used for the second result.
	 * 
	 * @param result1 first result
	 * @param result2 second result
	 * @return <code>true</code> iff the techniques used for the first result
	 *         have a higher preference
	 */
	private boolean hasHigherPreference(Result result1, Result result2) {
		for (String extractionTechnique : EXTRACTION_TECHNIQUES) {
			if (result2.extractedWith(extractionTechnique)) return false;
			if (result1.extractedWith(extractionTechnique)) return true;
		}
		return false;
	}
	
	/**
	 * Projects Web answers onto the corpus.
	 * 
	 * @param results array of <code>Result</code> objects from the Web
	 * @return array of <code>Result</code> objects from the corpus
	 */
	public Result[] apply(Result[] results) {
		// split corpus results into factoid answers and raw results
		Hashtable<String, Result> factoids = new Hashtable<String, Result>();
		Hashtable<String, Result> sentences = new Hashtable<String, Result>();
		ArrayList<String> normSentences = new ArrayList<String>();
		Filter sorter = new HitPositionSorterFilter();
		resultsCorp = sorter.apply(resultsCorp);  // sort by hit position
		for (Result resultCorp : resultsCorp) {
			if (resultCorp.getScore() > 0) {
				// factoid answer
				String norm = StringUtils.normalize(resultCorp.getAnswer());
				Result factoid = factoids.get(norm);
				if (factoid != null) {
					if (hasHigherPreference(resultCorp, factoid)) {
						factoids.put(norm, resultCorp);
						String[] neTypes = factoid.getNeTypes();
						if (neTypes != null)
							for (String neType : neTypes)
								resultCorp.addNeType(neType);
					} else {
						String[] neTypes = resultCorp.getNeTypes();
						if (neTypes != null)
							for (String neType : neTypes)
								factoid.addNeType(neType);
					}
				} else {
					factoids.put(norm, resultCorp);
				}
			} else {
				// raw result
				String[] sents = OpenNLP.sentDetect(resultCorp.getAnswer());
				for (String sent : sents) {  // one result for each sentence
					String norm = StringUtils.normalize(sent);
					if (!sentences.containsKey(norm)) {
						Result sentence = resultCorp.getCopy();
						sentence.setAnswer(sent);
						sentences.put(norm, sentence);
						normSentences.add(norm);
					}
				}
			}
		}
		
		// project web results onto corpus
		ArrayList<Result> projected = new ArrayList<Result>();
		for (Result resultWeb : results) {
			if (resultWeb.getScore() <= 0) continue;  // only project factoids
			String norm = StringUtils.normalize(resultWeb.getAnswer());
			
			// Answer projection rules:
			// - first try to find a matching factoid answer extracted from the
			//   corpus, only if this attempt fails browse the raw results
			// - a named entity from a model-based tagger is projected only if
			//   the same named entity was extracted from the corpus (this takes
			//   the poor performance of the model-based NE taggers on the noisy
			//   Web data into account)
			// - if a factoid answer was extracted from the corpus with more
			//   than one technique, then the first extraction technique in
			//   'EXTRACTION_TECHNIQUES' determines the supporting document
			Result factoid = factoids.get(norm);
			if (factoid != null &&
					(!NETagger.allModelType(resultWeb.getNeTypes()) ||
					factoid.isNamedEntity())) {
				// factoid answer also extracted from corpus:
				// if web answer not a named entity from a model-based tagger or
				// corpus answer also a named entity
				// -> project answer
				Result result = resultWeb.getCopy();
				result.setAnswer(factoid.getAnswer());
				result.setDocID(factoid.getDocID());
				result.setSentence(factoid.getSentence());
				projected.add(result);
			} else if (!NETagger.allModelType(resultWeb.getNeTypes())) {
				// factoid answer not extracted from corpus:
				// if answer not a named entity from a model-based tagger
				// -> browse sentences for answer
				String normRegex = RegexConverter.strToRegexWithBounds(norm);
				
				for (String normSentence : normSentences) {
					String[] truncs = normSentence.split(normRegex, -1);
					
					if (truncs.length > 1) {  // sentence contains answer?
						// undo normalization
						Result sentence = sentences.get(normSentence);
						String sent = sentence.getAnswer();
						int start = truncs[0].split(" ", -1).length - 1;
						int end = start + norm.split(" ").length;
						String[] tokens = NETagger.tokenize(sent);
						String answer = tokens[start];
						for (int i = start + 1; i < end; i++)
							answer += " " + tokens[i];
						answer = OpenNLP.untokenize(answer, sent);
						
						if (norm.equals(StringUtils.normalize(answer))) {
							Result result = resultWeb.getCopy();
							result.setAnswer(answer);
							result.setDocID(sentence.getDocID());
							result.setSentence(sentence.getAnswer());
							projected.add(result);
							break;
						} else {
							MsgPrinter.printErrorMsg("\nNormalization could " +
									"not be undone:\n" + norm);
						}
					}
				}
			}
		}
		
		return projected.toArray(new Result[projected.size()]);
	}
}

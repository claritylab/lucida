package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.querygeneration.Query;
import info.ephyra.search.Result;
import info.ephyra.util.StringUtils;

import java.util.Hashtable;

/**
 * <p>The <code>AnswerTypeFilter</code> extracts factoid answers of the expected
 * answer types from text passages. This is a high-recall but low-precision
 * extraction approach.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-04-14
 */
public class AnswerTypeFilter extends Filter {
	/** Identifier for the answer type testing approach. */
	public static final String ID = "Answer type testing";
	
	/**
	 * Extracts NEs of particular types from the answer strings of the
	 * <code>Result</code> objects and creates a new <code>Result</code> for
	 * each extracted unique answer.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// extracted factoid answers and corresponding results
		Hashtable<String, Result> factoids = new Hashtable<String, Result>();
		
		for (Result result : results) {
			// only apply this filter to results for the answer type testing
			// approach
			Query query = result.getQuery();
			String[] answerTypes = query.getAnalyzedQuestion().getAnswerTypes();
			if (!query.extractWith(ID) ||
					answerTypes.length == 0 ||
					result.getScore() > Float.NEGATIVE_INFINITY)
				continue;
			
			// split answer string into sentences and tokenize sentences
			String answer = result.getAnswer();
			String[] sentences = OpenNLP.sentDetect(answer);
			String[][] tokens = new String[sentences.length][];
			for (int i = 0; i < sentences.length; i++)
				tokens[i] = NETagger.tokenize(sentences[i]);
			
			for (String answerType : answerTypes) {
				// get IDs of the taggers for the most specific NE type that can
				// be tagged
				String[] neTypes = answerType.split("->");
				int neIds[] = new int[0];
				for (String neType : neTypes) {
					int[] thisIds = NETagger.getNeIds(neType);
					if (thisIds.length > 0) neIds = thisIds;
				}
				
				// extract NEs of that type
				for (int neId : neIds) {
					String neType = NETagger.getNeType(neId);
					String[][] nes = NETagger.extractNes(tokens, neId);
					
					for (int i = 0; i < sentences.length; i++) {
						// untokenize NEs
						for (int j = 0; j < nes[i].length; j++)
							nes[i][j] = OpenNLP.untokenize(nes[i][j],
									sentences[i]);
						
						// create new result for each unique normalized NE
						for (String ne : nes[i]) {
							String norm = StringUtils.normalize(ne);
							
							Result factoid = factoids.get(norm);
							if (factoid == null) {  // new answer
								// query, doc ID and sentence can be ambiguous
								factoid = new Result(ne, result.getQuery(),
													   result.getDocID());
								factoid.setSentence(sentences[i]);
								factoid.addExtractionTechnique(ID);
								factoids.put(norm, factoid);
							}
							factoid.addNeType(neType);
							factoid.incScore(1);
							// TODO consider query score, #keywords, hit pos
						}
					}
				}
			}
		}
		
		// keep old results
		Result[] newResults =
			factoids.values().toArray(new Result[factoids.size()]);
		Result[] allResults = new Result[results.length + newResults.length];
		for (int i = 0; i < results.length; i++)
			allResults[i] = results[i];
		for (int i = 0; i < newResults.length; i++)
			allResults[results.length + i] = newResults[i];
		
		return allResults;
	}
}

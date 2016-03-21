package info.ephyra.answerselection.filters;

import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.querygeneration.Query;
import info.ephyra.search.Result;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.Hashtable;
import java.util.Map;

/**
 * <p>The <code>FactoidsFromPredicatesFilter</code> extracts factoid answers
 * from predicate-argument structures.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-24
 */
public class FactoidsFromPredicatesFilter extends Filter {
	/** Identifier for the semantic parsing approach. */
	public static final String ID = "Semantic parsing";
	
	/**
	 * Extracts factoids from the predicates withing the answer strings of the
	 * <code>Result</code> objects and creates a new <code>Result</code> for
	 * each extracted unique answer.
	 * 
	 * @param results array of <code>Result</code> objects containing predicates
	 * @return array of <code>Result</code> objects containing factoids
	 */
	public Result[] apply(Result[] results) {
		// old results that are passed along the pipeline
		ArrayList<Result> oldResults = new ArrayList<Result>();
		// extracted factoid answers and corresponding results
		Hashtable<String, Result> factoids = new Hashtable<String, Result>();
		// extracted factoid answers and maximum weights of predicates
		Hashtable<String, Double> maxScores = new Hashtable<String, Double>();
		
		for (Result result : results) {
			// only apply this filter to results for the semantic parsing
			// approach
			Query query = result.getQuery();
			Predicate[] ps = query.getAnalyzedQuestion().getPredicates();
			if (!query.extractWith(ID) ||
					ps.length == 0 ||
					result.getScore() != 0) {
				oldResults.add(result);
				continue;
			}
			
			Predicate p = result.getPredicate();
			Predicate questionP = p.getSimPredicate();
			double simScore = p.getSimScore();
			Map<String, String[]> nes = result.getNes();
			
			// get answer strings
			ArrayList<String> answers = new ArrayList<String>();
			if (nes != null) {
				// take entities of the expected types as factoid answers
				// - allow entities in all arguments
				for (String ne : nes.keySet())
					for (String arg : p.getArgs())
						if (arg.contains(ne)) {
							answers.add(ne);
							break;
						}
				// - allow entities in missing arguments only
//				for (String ne : nes.keySet())
//					for (String missing : questionP.getMissingArgs()) {
//						String arg = p.get(missing);
//						if (arg != null && arg.contains(ne)) {
//							answers.add(ne);
//							break;
//						}
//					}
			} else {
				// or if the answer type is unknown, take the whole missing
				// arguments as factoid answers
				for (String missing : questionP.getMissingArgs()) {
					String arg = p.get(missing);
					if (arg != null) answers.add(arg);
				}
			}
			
			// create result objects
			for (String answer : answers) {
				String norm = StringUtils.normalize(answer);
				
				Result factoid = factoids.get(norm);
				if (factoid == null) {  // new answer
					// query, doc ID and sentence can be ambiguous
					factoid = new Result(answer, result.getQuery(),
										 result.getDocID());
					factoid.setSentence(result.getSentence());
					factoid.addExtractionTechnique(ID);
					factoids.put(norm, factoid);
					maxScores.put(norm, simScore);
				} else if (simScore > maxScores.get(norm)) {
					// remember document ID of predicate with highest score
					factoid.setDocID(result.getDocID());
					maxScores.put(norm, simScore);
				}
				if (nes != null)
					for (String neType : nes.get(answer))
						factoid.addNeType(neType);
				factoid.incScore((float) simScore);
			}
		}
		
		// keep old results
		Result[] newResults =
			factoids.values().toArray(new Result[factoids.size()]);
		Result[] allResults = new Result[oldResults.size() + newResults.length];
		oldResults.toArray(allResults);
		for (int i = 0; i < newResults.length; i++)
			allResults[oldResults.size() + i] = newResults[i];
		
		return allResults;
	}
}

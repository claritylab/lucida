package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.List;
import java.util.Set;

/**
 * <p>The <code>ScoreCombinationFilter</code> combines the normalized scores of
 * an answer extracted with several techniques into a single score.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-19
 */
public class ScoreCombinationFilter extends Filter {
	/**
	 * Combines normalized scores using CombMIN.
	 * 
	 * @param scores scores from all extractors that got the answer
	 * @param totalExtractors total number of extractors
	 */
	protected float combMIN(float[] scores, int totalExtractors) {
		if (scores.length < totalExtractors) return 0;
		
		float min = Float.MAX_VALUE;
		for (float score : scores) min = Math.min(min, score);
		
		return min;
	}
	
	/**
	 * Combines normalized scores using CombMED.
	 * 
	 * @param scores scores from all extractors that got the answer
	 * @param totalExtractors total number of extractors
	 */
	protected float combMED(float[] scores, int totalExtractors) {
		Arrays.sort(scores);
		int zeroAnswers = totalExtractors - scores.length;
		int m = totalExtractors / 2 - zeroAnswers;
		float med;
		if (totalExtractors % 2 == 1) {
			med = (m >= 0 ? scores[m] : 0);
		} else {
			med = (m >= 0 ? scores[m] : 0);
			med += (m - 1 >= 0 ? scores[m - 1] : 0);
			med /= 2;
		}
		
		return med;
	}
	
	/**
	 * Combines normalized scores using CombMAX.
	 * 
	 * @param scores scores from all extractors that got the answer
	 * @param totalExtractors total number of extractors
	 */
	protected float combMAX(float[] scores, int totalExtractors) {
		float max = 0;
		for (float score : scores) max = Math.max(max, score);
		
		return max;
	}
	
	/**
	 * Combines normalized scores using CombSUM.
	 * 
	 * @param scores scores from all extractors that got the answer
	 * @param totalExtractors total number of extractors
	 */
	protected float combSUM(float[] scores, int totalExtractors) {
		float sum = 0;
		for (float score : scores) sum += score;
		
		return sum;
	}
	
	/**
	 * Combines normalized scores using CombANZ.
	 * 
	 * @param scores scores from all extractors that got the answer
	 * @param totalExtractors total number of extractors
	 */
	protected float combANZ(float[] scores, int totalExtractors) {
		float sum = combSUM(scores, totalExtractors);
		float anz = sum / scores.length;
		
		return anz;
	}
	
	/**
	 * Combines normalized scores using CombANZ.
	 * 
	 * @param scores scores from all extractors that got the answer
	 * @param totalExtractors total number of extractors
	 */
	protected float combMNZ(float[] scores, int totalExtractors) {
		float sum = combSUM(scores, totalExtractors);
		float mnz = sum * scores.length;
		
		return mnz;
	}
	
	/**
	 * Combines normalized scores using complementary probabilities.
	 * 
	 * @param scores scores from all extractors that got the answer
	 * @param totalExtractors total number of extractors
	 */
	protected float combCP(float[] scores,
			int totalExtractors) {
		float compProb = 1;
		for (float score : scores)
			compProb *= 1 - score;
		float combined = 1 - compProb;
		
		return combined;
	}
	
	/**
	 * Filters an array of <code>Result</code> objects.
	 * 
	 * @param results results to filter
	 * @return filtered results
	 */
	public Result[] apply(Result[] results) {
		// all results that pass the filter
		List<Result> filtered = new ArrayList<Result>();
		
		// sort results by their scores in descending order
		results = (new ScoreSorterFilter()).apply(results);
		
		// separate factoid answers by extractors
		List<Result> factoids = new ArrayList<Result>();
		Hashtable<String, Hashtable<String, Result>> allExtractors =
			new Hashtable<String, Hashtable<String, Result>>();
		for (Result result : results) {
			// only merge factoid answers
			if (result.getScore() <= 0 ||
					result.getScore() == Float.POSITIVE_INFINITY) {
				filtered.add(result);
				continue;
			}
			// make sure that answers come from a single extractor
			String[] extractors = result.getExtractionTechniques();
			if (extractors == null || extractors.length != 1) {
				filtered.add(result);
				continue;
			}
			String extractor = extractors[0];
			
			factoids.add(result);
			Hashtable<String, Result> sameExtractor =
				allExtractors.get(extractor);
			if (sameExtractor == null) {
				sameExtractor = new Hashtable<String, Result>();
				allExtractors.put(extractor, sameExtractor);
			}
			String norm = StringUtils.normalize(result.getAnswer());
			sameExtractor.put(norm, result);
		}
		
		// merge answers from different extractors
		String[] extractors =
			allExtractors.keySet().toArray(new String[allExtractors.size()]);
		Set<String> covered = new HashSet<String>();
		for (Result result : factoids) {
			String norm = StringUtils.normalize(result.getAnswer());
			if (!covered.add(norm)) continue;
			
			// get all extractors for the result and the normalized scores
			ArrayList<String> exs = new ArrayList<String>();
			ArrayList<Float> scores = new ArrayList<Float>();
			for (String extractor : extractors) {
				Result r = allExtractors.get(extractor).get(norm);
				if (r != null) {
					exs.add(extractor);
					scores.add(r.getNormScore());
				}
			}
			
			// set extractors
			result.setExtractionTechniques(exs.toArray(new String[exs.size()]));
			// combine their normalized scores
			float[] scoresA = new float[scores.size()];
			for (int i = 0; i < scoresA.length; i++) scoresA[i] = scores.get(i);
			int totalExtractors = extractors.length;
			float combinedScore =
//				combMIN(scoresA, totalExtractors);
//				combMED(scoresA, totalExtractors);
//				combMAX(scoresA, totalExtractors);
//				combSUM(scoresA, totalExtractors);
//				combANZ(scoresA, totalExtractors);
				combMNZ(scoresA, totalExtractors);
//				combCP(scoresA, totalExtractors);
			result.setScore(combinedScore);
			result.setNormScore(combinedScore);
			
			filtered.add(result);
		}
		
		return filtered.toArray(new Result[filtered.size()]);
	}
}

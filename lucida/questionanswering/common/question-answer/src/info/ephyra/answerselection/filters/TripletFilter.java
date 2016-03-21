package info.ephyra.answerselection.filters;

import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.search.Result;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.HashSet;

/**
 * <p>Scores answer candidates for definitional questions according to the
 * number of novel NP-VP-NP triplets they contain.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class TripletFilter extends Filter {
	/**
	 * Increments the score of each result snippet according to the number of
	 * NP-VP-NP triplets it is the first to contain. This is meant to prefer
	 * snippets that provide new information over those that repeat information
	 * from previous snippets.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return modified array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// raw results returned by the searchers
		ArrayList<Result> rawResults = new ArrayList<Result>();
		
		HashSet<String> found = new HashSet<String>();
//		HashMap<String, Result> resultsByTriplets = new HashMap<String, Result>();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String stemmedQuestion = SnowballStemmer.stemAllTokens(r.getQuery().getAnalyzedQuestion().getQuestion());
				
				String text = r.getAnswer();
				
				//	tokenize and tag sentence
				if (!text.endsWith(".")) text += ".";
				String[] sentence = OpenNLP.tokenize(text); 
				String[] posTags = OpenNLP.tagPos(sentence);
				String[] chunkTags = OpenNLP.tagChunks(sentence, posTags);
				chunkTags = OpenNLP.joinNounPhrases(sentence, chunkTags);
				
				int tripStart = -1;
				int index = 0;
				
				int numberOfTriplets = 0;
				
				//	scan sentence for NP-VP-NP triplets
				while (index < sentence.length) {
					
					//	find start of first NP
					while ((index < sentence.length) && !"B-NP".equals(chunkTags[index])) index++;
					
					if (index < sentence.length) {
						tripStart = index;
						int i = 1;
						
						//	find start of VP
						while (((index + i) < sentence.length) && !"B-VP".equals(chunkTags[index + i])) {
							if ("B-NP".equals(chunkTags[index + i])) i = sentence.length;
							else if ("O".equals(chunkTags[index + i])) i = sentence.length;
							else i++;
						}
						i++;
						
						//	find start of second NP
						while (((index + i) < sentence.length) && !"B-NP".equals(chunkTags[index + i])) {
							if ("B-VP".equals(chunkTags[index + i])) i = sentence.length;
							else if ("O".equals(chunkTags[index + i])) i = sentence.length;
							else if ("B-SBAR".equals(chunkTags[index + i])) i = sentence.length;
							else i++;
						}
						
						//	complete second NP
						i++;
						while (((index + i) < sentence.length) && "I-NP".equals(chunkTags[index + i])) i++;
						
						//	remember NP-VP-NP triplet
						if ((index + i) < sentence.length) {
							String trip = "";
							for (int s = tripStart; s < (tripStart + i); s++) trip += " " + sentence[s];
							trip = SnowballStemmer.stemAllTokens(trip.trim());
							
							if (!found.contains(trip)) {
								found.add(trip);
								if (!StringUtils.isSubsetKeywords(trip, stemmedQuestion)) {
									//System.out.println("Triplet:\n  " + trip);
//									Result newRes = new Result(trip, r.getQuery(), r.getDocID(), r.getHitPos());
//									newRes.setScore(r.getScore() + 1);
//									rawResults.add(newRes);
									numberOfTriplets ++;
								}
							}
							
//							if (!StringUtils.isSubsetKeywords(trip, r.getQuery().getQuestion())) {
//								if (resultsByTriplets.containsKey(trip)) {
//									Result res = resultsByTriplets.get(trip);
//									res.setScore(res.getScore() + 1);
//								} else resultsByTriplets.put(trip, r);
//							}
						}
						index++;
					}
				}
				
				if (numberOfTriplets != 0) {
					r.incScore(numberOfTriplets);	//	20060724_2x runs
//					r.incScore(numberOfTriplets * (((float) results.length) / ((float) sentence.length)));	//	20060725_0x runs
					rawResults.add(r);
				}
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

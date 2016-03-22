package info.ephyra.answerselection.filters;

import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.search.Result;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.HashSet;

/**
 * <p>Scores answer candidates for definitional questions according to the
 * number of novel noun phrases they contain.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class NounPhraseFilter extends Filter {
	/**
	 * Increments the score of each result snippet according to the number of
	 * noun phrases it is the first to contain. This is meant to prefer snippets
	 * that provide new information over those that repeat information from
	 * previous snippets.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return modified array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// raw results returned by the searchers
		ArrayList<Result> rawResults = new ArrayList<Result>();
		
		HashSet<String> found = new HashSet<String>();
		//HashMap<String, Result> resultsByNPs = new HashMap<String, Result>();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String stemmedQuestion = SnowballStemmer.stemAllTokens(r.getQuery().getAnalyzedQuestion().getQuestion());
				
				String text = r.getAnswer();
				
				//	tokenize and tag sentence
				String[] sentence = OpenNLP.tokenize(text);
				String[] posTags = OpenNLP.tagPos(sentence);
				String[] chunkTags = OpenNLP.tagChunks(sentence, posTags);
				
				String np = null;
				
				int numberOfNPs = 0;
				
				//	scan sentence for NPs
				for (int i = 0; i < sentence.length; i++) {
					
					//	start of NP
					if ("B-NP".equals(chunkTags[i])) {
						np = sentence[i];
						
					//	NP continues
					} else if ("I-NP".equals(chunkTags[i]) && (np != null)) {
						np += " " + sentence[i];
						
					//	not part of a NP, remember collected NP if any
					} else if (np != null) {
						
						np = SnowballStemmer.stemAllTokens(np);
						
						if (!found.contains(np)) {
							found.add(np);
							if (!StringUtils.isSubsetKeywords(np, stemmedQuestion)) {
//								Result newRes = new Result(np, r.getQuery(), r.getDocID(), r.getHitPos());
//								newRes.setScore(r.getScore());
//								rawResults.add(newRes);
								numberOfNPs ++;
							}
						}
						
//						if (!StringUtils.isSubsetKeywords(np, r.getQuery().getQuestion())) {
//							if (resultsByNPs.containsKey(np)) {
//								Result res = resultsByNPs.get(np);
//								res.setScore(res.getScore() + 1);
//							} else resultsByNPs.put(np, r);
//						}
//						
						np = null;
					}
				}
				
				//	remember last NP if any
				if (np != null) {
					
					np = SnowballStemmer.stemAllTokens(np);
					
					if (!found.contains(np)) {
						found.add(np);
						if (!StringUtils.isSubsetKeywords(np, stemmedQuestion)) {
//							Result newRes = new Result(np, r.getQuery(), r.getDocID(), r.getHitPos());
//							newRes.setScore(r.getScore() + 1);
//							rawResults.add(newRes);
							numberOfNPs ++;
						}
					}
					
//					if (!StringUtils.isSubsetKeywords(np, r.getQuery().getQuestion())) {
//						if (resultsByNPs.containsKey(np)) {
//							Result res = resultsByNPs.get(np);
//							res.setScore(res.getScore() + 1);
//						} else resultsByNPs.put(np, r);
//					}
				}
				
				if (numberOfNPs != 0) {
//					r.incScore(numberOfNPs);	//	20060724_2x runs
					r.incScore(numberOfNPs * (((float) results.length) / ((float) sentence.length)));	//	20060725_0x runs
					rawResults.add(r);
				}
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

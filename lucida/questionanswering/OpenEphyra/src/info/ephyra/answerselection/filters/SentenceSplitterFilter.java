package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.ArrayList;

/**
 * <p>Splits text snippets into sentences in order to facilitate the detection
 * of redundant information.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class SentenceSplitterFilter extends Filter {
	/**
	 * Splits long snippets into individual sentences in order to facilitate
	 * subsequent filtering. The idea is that redundancy detection is easier for
	 * shorter snippets than for longer ones.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// raw results returned by the searchers
		ArrayList<Result> rawResults = new ArrayList<Result>();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String sentence = r.getAnswer();
				String[] sentences = sentence.split("\\.");
				if (sentences.length != 0) {
					
					//	re-join cut abbreviations
					ArrayList<String> sentenceList = new ArrayList<String>();
					String sen = sentences[0];
					for (int s = 1; s < sentences.length; s++) {
						String end = sen.substring(sen.lastIndexOf(" ") + 1).toLowerCase();
						if ((end.length() < 3) || end.matches("(^[aeiouy])++"))
							sen = sen + ". " + sentences[s];
						else {
							sentenceList.add(sen);
							sen = sentences[s];
						}
					}
					sentenceList.add(sen);
					sentences = sentenceList.toArray(new String[sentenceList.size()]);
					
					r.setAnswer(sentences[0]);
					rawResults.add(r);
					for (int s = 1; s < sentences.length; s++) {
						Result newRes = new Result(sentences[s], r.getQuery(), r.getDocID(), r.getHitPos());
						newRes.setScore(r.getScore());
						rawResults.add(newRes);
					}
				}
			}
		}
		
		results = rawResults.toArray(new Result[rawResults.size()]);
		rawResults.clear();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String sentence = r.getAnswer();
				String[] sentences = sentence.split("\\?|\\!");
				if (sentences.length != 0) {
					r.setAnswer(sentences[0]);
					rawResults.add(r);
					for (int s = 1; s < sentences.length; s++) {
						Result newRes = new Result(sentences[s], r.getQuery(), r.getDocID(), r.getHitPos());
						newRes.setScore(r.getScore());
						rawResults.add(newRes);
					}
				}
			}
		}
		
		results = rawResults.toArray(new Result[rawResults.size()]);
		rawResults.clear();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String sentence = r.getAnswer();
				String[] sentences = sentence.split("\\;");
				if (sentences.length != 0) {
					r.setAnswer(sentences[0]);
					rawResults.add(r);
					for (int s = 1; s < sentences.length; s++) {
						Result newRes = new Result(sentences[s], r.getQuery(), r.getDocID(), r.getHitPos());
						newRes.setScore(r.getScore());
						rawResults.add(newRes);
					}
				}
			}
		}
		
		results = rawResults.toArray(new Result[rawResults.size()]);
		rawResults.clear();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String sentence = r.getAnswer();
				String[] sentences = sentence.split("\\-\\-");
				if (sentences.length != 0) {
					r.setAnswer(sentences[0]);
					rawResults.add(r);
					for (int s = 1; s < sentences.length; s++) {
						Result newRes = new Result(sentences[s], r.getQuery(), r.getDocID(), r.getHitPos());
						newRes.setScore(r.getScore());
						rawResults.add(newRes);
					}
				}
			}
		}
		
		results = rawResults.toArray(new Result[rawResults.size()]);
		rawResults.clear();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String sentence = r.getAnswer();
				String[] sentences = sentence.split("\\.\\'\\'");
				if (sentences.length != 0) {
					r.setAnswer(sentences[0]);
					rawResults.add(r);
					for (int s = 1; s < sentences.length; s++) {
						Result newRes = new Result(sentences[s], r.getQuery(), r.getDocID(), r.getHitPos());
						newRes.setScore(r.getScore());
						rawResults.add(newRes);
					}
				}
			}
		}
		
		results = rawResults.toArray(new Result[rawResults.size()]);
		rawResults.clear();
		
		for (Result r : results) {
			if (r.getScore() != Float.NEGATIVE_INFINITY) {
				String sentence = r.getAnswer();
				String[] sentences = sentence.split(":");
				if (sentences.length != 0) {
					r.setAnswer(sentences[0]);
					rawResults.add(r);
					for (int s = 1; s < sentences.length; s++) {
						Result newRes = new Result(sentences[s], r.getQuery(), r.getDocID(), r.getHitPos());
						newRes.setScore(r.getScore());
						rawResults.add(newRes);
					}
				}
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

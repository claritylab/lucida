package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.search.Result;

import java.util.ArrayList;

/**
 * <p>An adaptive version of the <code>NumberOfKeywordsFilter</code> that also
 * takes occurrences of keywords in preceding text snippets into account.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-10
 */
public class AdaptiveNumberOfKeywordsFilter extends Filter {
	/**
	 * Counts the number of words in the first array that occur in the second
	 * array. Does not distinguish between lower and upper case.
	 * 
	 * @param keywords string array 1
	 * @param resWords string array 2
	 * @return number of words in array 1 that occur in array 2
	 */
	private int getNumberOfMatches(String[] keywords, String[] resWords) {
		int count = 0;
		
//		for (String keyword : keywords)
//			for (String resWord : resWords)
//				if (keyword.equalsIgnoreCase(resWord)) {
//					count++;
//					break;  // count each word in s1 only once
//				}
		
		//	double count subsequent keywords 
		for (int k = 0; k < keywords.length; k++) {
			String keyword = keywords[k];
			int maxSeq = 0;
			for (int r = 0; r < resWords.length; r++) {
				if (resWords[r].equalsIgnoreCase(keyword)) {
					if (maxSeq == 0) {
						count ++;
						maxSeq ++;
					}
					int i = 1;
					while (((k + i) < keywords.length) 
//					if (((k + i) < keywords.length) 
							&& ((r + i) < resWords.length) 
							&& resWords[r].matches("[A-Z].++")
							&& resWords[r + i].matches("[A-Z].++")
//							&& keywords[k].matches("[A-Z].++")
//							&& keywords[k + i].matches("[A-Z].++")
							&& resWords[r + i].equalsIgnoreCase(keywords[k + i])) {
						if (maxSeq <= i) {
							count += (i + 1);
//							count ++;
							maxSeq ++;
						}
						i++;
					}
				}
			}
		}
		
		return count;
	}
	
	/**
	 * Score result snippets according to the number of keywords (target terms)
	 * they contain. Within two snippets from the same document, transfer score
	 * from a snippet to the subsequent one if the former contains many of the
	 * keywords. The idea is that a subsequent snippet might use a pronoun for
	 * the target (thus not contain the target itself), but provide useful
	 * information anyway.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		// raw results returned by the searchers
		ArrayList<Result> rawResults = new ArrayList<Result>();
		
		int lastScore = 0;
		String lastDocID = "";
		int keywordCount = 1;
		
		for (Result result : results) {
			if (result.getScore() != Float.NEGATIVE_INFINITY) {
				
				String[] keywords = NETagger.tokenize(result.getQuery().getQueryString());
				for (int k = 0; k < keywords.length; k++) keywords[k] = SnowballStemmer.stem(keywords[k]);
				int k = keywords.length;
				keywordCount = k;
				
				String[] wordsInResult = NETagger.tokenize(result.getAnswer());
				for (int r = 0; r < wordsInResult.length; r++) wordsInResult[r] = SnowballStemmer.stem(wordsInResult[r]);
				
				int m = getNumberOfMatches(keywords, wordsInResult);
				
				if (m >= Math.floor(Math.sqrt(k - 1) + 1)) {
					lastDocID = result.getDocID();	//	remember doc ID so score is propagated only within same document
					if (lastDocID == null) lastDocID = "";
					lastScore = ((m * m + 1) / 2);	//	remember score
//					lastScore = ((m + 1) / 2);	//	remember score
					
					result.incScore(m * m);  // manipulate score
//					result.incScore(m);  // manipulate score
					rawResults.add(result);  // keep result
					
				} else if ((lastScore > 0) && lastDocID.equalsIgnoreCase(result.getDocID())) {
					result.incScore(lastScore);  // manipulate score
					rawResults.add(result);  // keep result
					lastScore = (lastScore / 2); //	decay last score
					
				} else {
					lastScore = 0;	//	reset remembered score
				}
			}
		}
		
		//	if too little results, match againg and consider only proper names
		if (rawResults.size() < 100) {
			for (Result result : results) {
				if (result.getScore() != Float.NEGATIVE_INFINITY) {
					
					String[] keywords = NETagger.tokenize(result.getQuery().getQueryString());
					ArrayList<String>keywordList = new ArrayList<String>();
					for (int k = 0; k < keywords.length; k++)
						if (keywords[k].matches("[A-Z]++.*+"))
							keywordList.add(SnowballStemmer.stem(keywords[k]));
					keywords = keywordList.toArray(new String[keywordList.size()]);
					int k = keywords.length;
					
					//	do this only if now less keywords
					if ((keywords.length != 0) && (k < keywordCount)) {
						String[] wordsInResult = NETagger.tokenize(result.getAnswer());
						for (int r = 0; r < wordsInResult.length; r++) wordsInResult[r] = SnowballStemmer.stem(wordsInResult[r]);
						
						int m = getNumberOfMatches(keywords, wordsInResult);
						
						if (m >= Math.floor(Math.sqrt(k - 1) + 1)) {
							result.incScore(m * m);  // manipulate score
//							result.incScore(m);  // manipulate score
							rawResults.add(result);  // keep result
							
						}
					}
				}
			}
		}
		
		return rawResults.toArray(new Result[rawResults.size()]);
	}
}

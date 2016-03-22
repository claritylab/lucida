package info.ephyra.nlp;

import info.ephyra.util.StringUtils;

import java.util.ArrayList;

import com.aliasi.sentences.MedlineSentenceModel;
import com.aliasi.sentences.SentenceModel;
import com.aliasi.tokenizer.IndoEuropeanTokenizerFactory;
import com.aliasi.tokenizer.Tokenizer;
import com.aliasi.tokenizer.TokenizerFactory;

/**
 * <p>This class provides a common interface to the
 * <a href="http://www.alias-i.com/lingpipe/">LingPipe</a> toolkit.</p>
 * 
 * <p>It supports the following natural language processing tools:
 * <ul>
 * <li>Tokenization</li>
 * <li>Sentence detection</li>
 * </ul>
 * </p>
 * 
 * @author Nico Schlaefer
 * @version 2006-11-25
 */
public class LingPipe {
	/** Tokenization model. */
	private static TokenizerFactory tokenizerFactory;
	/** Sentence detection model. */
	private static SentenceModel sentenceModel;
	
	/**
	 * Creates a model for the tokenizer, if not done already.
	 */
	public static void createTokenizer() {
		if (tokenizerFactory == null)
			tokenizerFactory = new IndoEuropeanTokenizerFactory();
	}
	
	/**
	 * Creates models for the tokenizer and the sentence detector, if not
	 * already done.
	 */
	public static void createSentenceDetector() {
		if (tokenizerFactory == null)
			tokenizerFactory = new IndoEuropeanTokenizerFactory();
		if (sentenceModel == null)
			sentenceModel = new MedlineSentenceModel();
	}
	
	/**
	 * Tokenizes a text.
	 * 
	 * @param text text to tokenize
	 * @return array of tokens or <code>null</code>, if the tokenizer is not
	 *         initialized
	 */
	public static String[] tokenize(String text) {
		if (tokenizerFactory == null) return null;
		
		ArrayList<String> tokenList = new ArrayList<String>();
		ArrayList<String> whiteList = new ArrayList<String>();
		Tokenizer tokenizer =
			tokenizerFactory.tokenizer(text.toCharArray(), 0, text.length());
		tokenizer.tokenize(tokenList, whiteList);
		
		return tokenList.toArray(new String[tokenList.size()]);
	}
	
	/**
	 * Tokenizes a text and concatenates the tokens with spaces.
	 * 
	 * @param text text to tokenize
	 * @return string of space-delimited tokens or <code>null</code>, if the
	 *         tokenizer is not initialized
	 */
	public static String tokenizeWithSpaces(String text) {
		String[] tokens = tokenize(text);
		return (tokens != null) ? StringUtils.concatWithSpaces(tokens) : null;
	}
	
	/**
	 * Splits a text into sentences.
	 * 
	 * @param text sequence of sentences
	 * @return array of sentences in the text or <code>null</code>, if the
	 *         sentence detector is not initialized
	 */
	public static String[] sentDetect(String text) {
		if (sentenceModel == null) return null;
		
	    // tokenize text
		ArrayList<String> tokenList = new ArrayList<String>();
		ArrayList<String> whiteList = new ArrayList<String>();
		Tokenizer tokenizer =
			tokenizerFactory.tokenizer(text.toCharArray(), 0, text.length());
		tokenizer.tokenize(tokenList, whiteList);
		
		String[] tokens = tokenList.toArray(new String[tokenList.size()]);
		String[] whites = whiteList.toArray(new String[whiteList.size()]);
		
		// detect sentences
		int[] sentenceBoundaries =
			sentenceModel.boundaryIndices(tokens, whites);
		
		int sentStartTok = 0;
		int sentEndTok = 0;
		
		String[] sentences = new String[sentenceBoundaries.length];
		for (int i = 0; i < sentenceBoundaries.length; i++) {
			sentEndTok = sentenceBoundaries[i];
			
			StringBuilder sb = new StringBuilder();
			for (int j = sentStartTok; j <= sentEndTok; j++) {
				sb.append(tokens[j]);
				if (whites[j + 1].length() > 0 && j < sentEndTok)
					sb.append(" ");
			}
			sentences[i] = sb.toString();
			
			sentStartTok = sentEndTok+1;
		}
		
		return sentences;
	}
}

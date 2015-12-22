package info.ephyra.answerselection.filters;

import info.ephyra.io.Logger;
import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.querygeneration.Query;
import info.ephyra.querygeneration.generators.BagOfWordsG;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.KeywordExtractor;
import info.ephyra.questionanalysis.QuestionNormalizer;
import info.ephyra.search.Result;
import info.ephyra.trec.TREC13To16Parser;
import info.ephyra.trec.TRECTarget;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;
import java.util.HashMap;

/**
 * <p>A web term importance filter that counts term frequencies in a Wikipedia
 * article on the target of the question.</p>
 * 
 * <p>This class extends the class <code>WebTermImportanceFilter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class WikipediaTermImportanceFilter extends WebTermImportanceFilter {
	
//	protected static final String person = "person";
//	protected static final String organization = "organization";
//	protected static final String location = "location";
//	protected static final String event = "event";
//	
//	public static final int NO_NORMALIZATION = 0;
//	public static final int LINEAR_LENGTH_NORMALIZATION = 1;
//	public static final int SQUARE_ROOT_LENGTH_NORMALIZATION = 2;
//	public static final int LOG_LENGTH_NORMALIZATION = 3;
//	public static final int LOG_10_LENGTH_NORMALIZATION = 4;
	
//	protected static final String WIKIPEDIA = "wikipedia";
	
	/**
	 * @param normalizationMode
	 * @param tfNormalizationMode
	 * @param isCombined
	 */
	public WikipediaTermImportanceFilter(int normalizationMode, int tfNormalizationMode, boolean isCombined) {
		super(normalizationMode, tfNormalizationMode, isCombined);
	}
	
	/** @see info.ephyra.answerselection.filters.WebTermImportanceFilter#getTargets(java.lang.String)
	 */
	@Override
	public String[] getTargets(String target) {
		String[] targets = {target};
		return targets;
	}
	
	/** @see info.ephyra.answerselection.filters.WebTermImportanceFilter#getTermCounters(java.lang.String[])
	 */
	public HashMap<String, TermCounter> getTermCounters(String[] targets) {
		if (targets.length == 0) return new HashMap<String, TermCounter>();
		return this.getTermCounters(targets[0]);
	}
	
	/**
	 * fetch the term frequencies in the top X result snippets of a web search
	 * for some target
	 * 
	 * @param target the target
	 * @return a HashMap mapping the terms in the web search results to their
	 *         frequency in the snippets
	 */
	public HashMap<String, TermCounter> getTermCounters(String target) {
		HashMap<String, TermCounter> rawTermCounters = null;
		try {
			String url = "http://en.wikipedia.org/wiki/" + target.replaceAll("\\s", "_");
			URLConnection connection = new URL(url).openConnection();
			connection.setDoInput(true);
			connection.setDoOutput(true);
			connection.setUseCaches(false);
			connection.setRequestProperty("User-Agent", "Ephyra");
			connection.connect();
			
			BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
			rawTermCounters = new HashMap<String, TermCounter>();
			boolean inTag = false;
			int c = 0;
			StringBuffer term = new StringBuffer();
			while ((c = reader.read()) != -1) {
				if (c == '<') {
					inTag = true;
					if (term.length() != 0) {
						String stemmedTerm = SnowballStemmer.stem(term.toString().toLowerCase());
						System.out.println(stemmedTerm);
						if (!rawTermCounters.containsKey(stemmedTerm))
							rawTermCounters.put(stemmedTerm, new TermCounter());
						rawTermCounters.get(stemmedTerm).increment(1);
						term = new StringBuffer();
					}
				} else if (c == '>') {
					inTag = false;
				} else if (!inTag) {
					if (c < 33) {
						if (term.length() != 0) {
							String stemmedTerm = SnowballStemmer.stem(term.toString().toLowerCase());
							System.out.println(stemmedTerm);
							if (!rawTermCounters.containsKey(stemmedTerm))
								rawTermCounters.put(stemmedTerm, new TermCounter());
							rawTermCounters.get(stemmedTerm).increment(1);
							term = new StringBuffer();
						}
					} else term.append((char) c);
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
		return rawTermCounters;
	}
	
//	/**
//	 * Increment the score of each result snippet for each word in it according
//	 * to the number of top-100 web search engine snippets containing this
//	 * particular word. This favors snippets that provide information given
//	 * frequently and thus likely to be more important with regard to the
//	 * target.
//	 * 
//	 * @param results array of <code>Result</code> objects
//	 * @return extended array of <code>Result</code> objects
//	 */
//	@SuppressWarnings("unchecked")
//	public Result[] apply(Result[] results) {
//		
//		//	catch empty result 
//		if (results.length == 0) return results;
//		
//		//	produce target variations
//		String target = results[0].getQuery().getOriginalQueryString();
//		
//		System.out.println("WikipediaTermImportanceFilter:\n processing target '" + target + "'");
//		
//		HashMap<String, TermCounter> rawTermCounters = this.cacheLookup(target);
//		
//		//	cache miss
//		if (rawTermCounters == null) {
//			rawTermCounters = this.getTermCounters(target);
//			this.cache(target, rawTermCounters);
//		}
//		
//		//	something's wrong, rely on other filters
//		if (rawTermCounters == null) return results;
//		
//		//	get target tokens
//		HashSet<String> rawTargetTerms = new HashSet<String>();
//		String[] targetTokens = OpenNLP.tokenize(target);
//		for (String tt : targetTokens)
//			if (Character.isLetterOrDigit(tt.charAt(0)))
//				rawTargetTerms.add(tt);
//		
//		//	stem terms, collect target terms
//		HashMap<String, TermCounter> termCounters = new HashMap<String, TermCounter>();//this.getTermCounters(targets);
//		HashSet<String> targetTerms = new HashSet<String>();
//		ArrayList<String> rawTerms = new ArrayList<String>(rawTermCounters.keySet());
//		for (String rawTerm : rawTerms) {
//			
//			String stemmedTerm = SnowballStemmer.stem(rawTerm.toLowerCase());
//			if (!termCounters.containsKey(stemmedTerm))
//				termCounters.put(stemmedTerm, new TermCounter());
//			termCounters.get(stemmedTerm).increment(rawTermCounters.get(rawTerm).getValue());
//			
//			if (rawTargetTerms.contains(rawTerm))
//				targetTerms.add(stemmedTerm);
//		}
//		
//		
//		//	score results
//		ArrayList<Result> resultList = new ArrayList<Result>();
//		boolean goOn;
//		do {
//			goOn = false;
//			ArrayList<Result> rawResults = new ArrayList<Result>();
//			
//			//	score all results
//			for (Result r : results) {
//				if (r.getScore() != Float.NEGATIVE_INFINITY) {
//					
//					//	tokenize sentence
//					String[] sentence = NETagger.tokenize(r.getAnswer());
//					float importance = 0;
//					
//					//	scan sentence for terms from web result
//					for (int i = 0; i < sentence.length; i++) {
//						String term = sentence[i];
//						if ((term.length() > 1)/* && !StringUtils.isSubsetKeywords(term, r.getQuery().getAnalyzedQuestion().getQuestion()) && !FunctionWords.lookup(term)*/) {
//							term = SnowballStemmer.stem(term.toLowerCase());
//							TermCounter count = termCounters.get(term);
//							if (count != null) {
//								int wc = WordFrequencies.lookup(term);
//								importance += (count.getValue() / Math.max(wc, 1));
//							}
//						}
//					}
//					
//					//	TODO don't throw out 0-scored results for combining approaches
//					if (importance > 0) {
//						if (this.normalizationMode == NO_NORMALIZATION)
//							r.setScore(importance);
//						else if (this.normalizationMode == LINEAR_LENGTH_NORMALIZATION)
//							r.setScore(importance / sentence.length); // try normalized score
//						else if (this.normalizationMode == SQUARE_ROOT_LENGTH_NORMALIZATION)
//							r.setScore(importance / ((float) Math.sqrt(sentence.length))); // try normalized score
//						else if (this.normalizationMode == LOG_LENGTH_NORMALIZATION)
//							r.setScore(importance / (1 + ((float) Math.log(sentence.length)))); // try normalized score
//						else if (this.normalizationMode == LOG_10_LENGTH_NORMALIZATION)
//							r.setScore(importance / (1 + ((float) Math.log10(sentence.length)))); // try normalized score
//						
//						rawResults.add(r);
//					}
//				}
//			}
//			
//			if (rawResults.size() != 0) {
//				
//				//	find top result
//				Collections.sort(rawResults);
//				Collections.reverse(rawResults);
//				Result top = rawResults.remove(0);
//				resultList.add(top);
//				
//				//	decrement scores of top result terms
//				String[] sentence = NETagger.tokenize(top.getAnswer());
//				for (int i = 0; i < sentence.length; i++) {
//					String term = SnowballStemmer.stem(sentence[i].toLowerCase());
//					TermCounter count = termCounters.get(term);
//					
//					if (count != null) {
//						if (targetTerms.contains(term)) count.divideValue(1);
//						else count.divideValue(2);
//						
//						if (count.getValue() == 0) termCounters.remove(term);
//					}
//				}
//				
//				//	prepare remaining results for next round
//				results = rawResults.toArray(new Result[rawResults.size()]);
//				goOn = true;
//			}
//			
//		} while (goOn);
//		
//		Collections.sort(resultList);
//		Collections.reverse(resultList);
//		
//		//	set position-dependent extra score for combining approaches
//		float eScore = 100;
//		for (Result r : resultList) {
//			r.addExtraScore((this.getClass().getName() + this.normalizationMode), eScore);
//			eScore *= 0.9f;
//		}
//		
//		return resultList.toArray(new Result[resultList.size()]);
//	}
	
//	private static String lastTarget = null;
//	private static HashMap<String, TermCounter> lastTargetTermCounters = null;
//	
//	private void cache(String target, HashMap<String, TermCounter> termCounters) {
//		System.out.println("WikipediaTermImportanceFilter: caching web lookup result for target '" + target + "'");
//		lastTarget = target;
//		lastTargetTermCounters = termCounters;
//	}
//	
//	private HashMap<String, TermCounter> cacheLookup(String target) {
//		System.out.println("WikipediaTermImportanceFilter: doing cache lookup result for target '" + target + "'");
//		if (target.equals(lastTarget)) {
//			System.out.println("  --> cache hit");
//			return lastTargetTermCounters;
//		} else {
//			System.out.println("  --> cache miss, last target is '" + lastTarget + "'");
//			return null;
//		}
//	}
	
	protected static boolean TEST_TERM_DOWMLOD = false;
	
	public static void main(String[] args) {
		TEST_TERM_DOWMLOD = true;
		
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		// create tokenizer
		MsgPrinter.printStatusMsg("Creating tokenizer...");
		if (!OpenNLP.createTokenizer("res/nlp/tokenizer/opennlp/EnglishTok.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create tokenizer.");
//		LingPipe.createTokenizer();
		
//		// create sentence detector
//		MsgPrinter.printStatusMsg("Creating sentence detector...");
//		if (!OpenNLP.createSentenceDetector("res/nlp/sentencedetector/opennlp/EnglishSD.bin.gz"))
//			MsgPrinter.printErrorMsg("Could not create sentence detector.");
//		LingPipe.createSentenceDetector();
		
		// create stemmer
		MsgPrinter.printStatusMsg("Creating stemmer...");
		SnowballStemmer.create();
		
//		// create part of speech tagger
//		MsgPrinter.printStatusMsg("Creating POS tagger...");
//		if (!OpenNLP.createPosTagger("res/nlp/postagger/opennlp/tag.bin.gz",
//									 "res/nlp/postagger/opennlp/tagdict"))
//			MsgPrinter.printErrorMsg("Could not create OpenNLP POS tagger.");
//		if (!StanfordPosTagger.init("res/nlp/postagger/stanford/" +
//				"train-wsj-0-18.holder"))
//			MsgPrinter.printErrorMsg("Could not create Stanford POS tagger.");
		
//		// create chunker
//		MsgPrinter.printStatusMsg("Creating chunker...");
//		if (!OpenNLP.createChunker("res/nlp/phrasechunker/opennlp/" +
//								   "EnglishChunk.bin.gz"))
//			MsgPrinter.printErrorMsg("Could not create chunker.");
		
		// create named entity taggers
		MsgPrinter.printStatusMsg("Creating NE taggers...");
		NETagger.loadListTaggers("res/nlp/netagger/lists/");
		NETagger.loadRegExTaggers("res/nlp/netagger/patterns.lst");
		MsgPrinter.printStatusMsg("  ...loading models");
//		if (!NETagger.loadNameFinders("res/nlp/netagger/opennlp/"))
//			MsgPrinter.printErrorMsg("Could not create OpenNLP NE tagger.");
//		if (!StanfordNeTagger.isInitialized() && !StanfordNeTagger.init())
//			MsgPrinter.printErrorMsg("Could not create Stanford NE tagger.");
		MsgPrinter.printStatusMsg("  ...done");
		
		WikipediaTermImportanceFilter wtif = new WikipediaTermImportanceFilter(NO_NORMALIZATION, NO_NORMALIZATION, false);
		TRECTarget[] targets = TREC13To16Parser.loadTargets(args[0]);
		for (TRECTarget target : targets) {
			String question = target.getTargetDesc();
			
			// query generation
			MsgPrinter.printGeneratingQueries();
			String qn = QuestionNormalizer.normalize(question);
			MsgPrinter.printNormalization(qn);  // print normalized question string
			Logger.logNormalization(qn);  // log normalized question string
			String[] kws = KeywordExtractor.getKeywords(qn);
			AnalyzedQuestion aq = new AnalyzedQuestion(question);
			aq.setKeywords(kws);
			aq.setFactoid(false);
			
			Query[] queries = new BagOfWordsG().generateQueries(aq);
			for (int q = 0; q < queries.length; q++)
				queries[q].setOriginalQueryString(question);
			
			Result[] results = new Result[1];
			results[0] = new Result("This would be the answer", queries[0]);
			wtif.apply(results);
		}
	}
}

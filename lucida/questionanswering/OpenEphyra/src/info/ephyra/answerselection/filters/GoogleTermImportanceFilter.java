package info.ephyra.answerselection.filters;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.StanfordNeTagger;
import info.ephyra.search.searchers.GoogleKM;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;

import com.google.soap.search.GoogleSearch;
import com.google.soap.search.GoogleSearchFault;
import com.google.soap.search.GoogleSearchResult;
import com.google.soap.search.GoogleSearchResultElement;

/**
 * <p>A web term importance filter that counts term frequencies in text snippets
 * retrieved with the Google search engine.</p>
 * 
 * <p>This class extends the class <code>WebTermImportanceFilter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class GoogleTermImportanceFilter extends WebTermImportanceFilter {
	/** Google license key. */
	private static final String GOOGLE_KEY = "Enter your Google license key.";
	/** Maximum total number of search results. */
	private static final int MAX_RESULTS_TOTAL = 250;
	/** Maximum number of search results per query. */
	private static final int MAX_RESULTS_PERQUERY = 10;
	/** Number of retries if search fails. */
	private static final int RETRIES = 50;
	
	/**
	 * @param normalizationMode
	 * @param tfNormalizationMode
	 * @param isCombined
	 */
	public GoogleTermImportanceFilter(int normalizationMode, int tfNormalizationMode, boolean isCombined) {
		super(normalizationMode, tfNormalizationMode, isCombined);
	}
	
	/** @see info.ephyra.answerselection.filters.WebTermImportanceFilter#getTermCounters(java.lang.String[])
	 */
	@Override
	public HashMap<String, TermCounter> getTermCounters(String[] targets) {
		HashMap<String, TermCounter> termCounters = new HashMap<String, TermCounter>();
		
		//	process targets
		for (String target : targets) {
			
//			//	process wikipedia lookup target
//			if (target.endsWith(WIKIPEDIA)) {
//				
//				//	get snippets from google
//				GoogleSearch search = new GoogleSearch();
//				if (TEST_TARGET_GENERATION) System.out.println("Got search ...");
//				
//				// set license key
//				search.setKey(GOOGLE_KEY);
//				if (TEST_TARGET_GENERATION) System.out.println(" - key is " + GOOGLE_KEY);
//				
//				// set search string
//				search.setQueryString(target);
//				if (TEST_TARGET_GENERATION) System.out.println(" - target is " + target);
//				
//				// set language to English only
//				search.setLanguageRestricts("English");
//				if (TEST_TARGET_GENERATION) System.out.println(" - language set");
//				
//				// set hit position of first search result
//				search.setStartResult(0);
//				if (TEST_TARGET_GENERATION) System.out.println(" - start result set to " + 0);
//				
//				// set maximum number of search results
//				search.setMaxResults(MAX_RESULTS_PERQUERY);
//				if (TEST_TARGET_GENERATION) System.out.println(" - max results set");
//				
//				// perform search
//				GoogleSearchResult googleResult = null;
//				int retries = 0;
//				while (googleResult == null)
//					try {
//						googleResult = search.doSearch();
//					} catch (GoogleSearchFault e) {
//						MsgPrinter.printSearchError(e);  // print search error message
//						
//						if (retries == RETRIES) {
//							MsgPrinter.printErrorMsg("\nSearch failed.");
//							System.exit(1);
//						}
//						retries++;
//						
//						try {
//							GoogleKM.sleep(1000);
//						} catch (InterruptedException ie) {}
//					}
//				
//				// get snippets
//				GoogleSearchResultElement[] elements = googleResult.getResultElements();
//				if (TEST_TARGET_GENERATION) System.out.println(" - got results: " + elements.length);
//				
//				for (int i = 0; i < elements.length; i++) {
//					String url = elements[i].getURL();
//					
//					//	get artivle from wikipedia and extract terms
//					if (url.toLowerCase().indexOf(WIKIPEDIA.toLowerCase()) != -1) try {
//						BufferedReader br = new BufferedReader(new InputStreamReader(new URL(url).openStream()));
//						String line;
//						while ((line = br.readLine()) != null) {
//							if (line.startsWith("<p>")) {
//								String plain = line.toLowerCase().replaceAll("\\<[^\\>]++\\>", " ");
//								plain = plain.replaceAll("\\&\\#39\\;", "'");
//								if (TEST_TARGET_GENERATION) System.out.println(" - plain: " + plain);
//								
//								//	tokenize and tag sentence
//								String[] sentence = NETagger.tokenize(plain);
//								
//								//	scan sentence for NPs
//								for (int s = 0; s < sentence.length; s++) {
//									String term = SnowballStemmer.stem(sentence[s].toLowerCase());
//									if (term.length() > 1) {
//										if (!termCounters.containsKey(term))
//											termCounters.put(term, new TermCounter());
//										termCounters.get(term).increment();
//									}
//								}
//							}
//						}
//					} catch (IOException ioe) {}
//				}
//			}
//			
//			//	process other target
//			else 
			
			//	subsequently get top MAX_RESULTS_TOTAL snippets, MAX_RESULTS_PERQUERY each time
			for (int startResult = 0; startResult < MAX_RESULTS_TOTAL; startResult += MAX_RESULTS_PERQUERY) {
				
				//	get snippets from google
				GoogleSearch search = new GoogleSearch();
				if (TEST_TARGET_GENERATION) System.out.println("Got search ...");
				
				// set license key
				search.setKey(GOOGLE_KEY);
				if (TEST_TARGET_GENERATION) System.out.println(" - key is " + GOOGLE_KEY);
				
				// set search string
				search.setQueryString(target);
				if (TEST_TARGET_GENERATION) System.out.println(" - target is " + target);
				
				// set language to English only
				search.setLanguageRestricts("English");
				if (TEST_TARGET_GENERATION) System.out.println(" - language set");
				
				// set hit position of first search result
				search.setStartResult(startResult);
				if (TEST_TARGET_GENERATION) System.out.println(" - start result set to " + startResult);
				
				// set maximum number of search results
				search.setMaxResults(MAX_RESULTS_PERQUERY);
				if (TEST_TARGET_GENERATION) System.out.println(" - max results set");
				
				// perform search
				GoogleSearchResult googleResult = null;
				int retries = 0;
				while (googleResult == null)
					try {
						googleResult = search.doSearch();
					} catch (GoogleSearchFault e) {
						MsgPrinter.printSearchError(e);  // print search error message
						
						if (retries == RETRIES) {
							MsgPrinter.printErrorMsg("\nSearch failed.");
							//System.exit(1);
							return termCounters;
						}
						retries++;
						
						try {
							GoogleKM.sleep(1000);
						} catch (InterruptedException ie) {}
					}
				
				// get snippets
				GoogleSearchResultElement[] elements = googleResult.getResultElements();
				if (TEST_TARGET_GENERATION) System.out.println(" - got results: " + elements.length);
				
				//	parse google snippets
				int lengthSum = 0;
				for (int i = 0; i < elements.length; i++) {
//					if (TEST_TARGET_GENERATION) System.out.println(" - summary: " + elements[i].getSummary());
//					if (TEST_TARGET_GENERATION) System.out.println(" - snippet: " + elements[i].getSnippet());
					String plain = elements[i].getSnippet().replaceAll("\\<[^\\>]++\\>", " ");
					plain = plain.replaceAll("\\&\\#39\\;", "'");
					if (TEST_TARGET_GENERATION) System.out.println(" - plain: " + plain);
					
					//	tokenize and tag sentence
					String[] sentence = NETagger.tokenize(plain);
//					String[] sentence = NETagger.tokenize(elements[i].getSnippet());
//					String[] sentence = NETagger.tokenize(elements[i].getSummary());
					lengthSum += sentence.length;
					
					//	scan sentence for NPs
					for (int s = 0; s < sentence.length; s++) {
						String term = SnowballStemmer.stem(sentence[s].toLowerCase());
						if (term.length() > 1) {
							if (!termCounters.containsKey(term))
								termCounters.put(term, new TermCounter());
							termCounters.get(term).increment();
						}
					}
				}
			}
		}
		return termCounters;
	}
	
	public static void main(String[] args) {
		
		TEST_TARGET_GENERATION = true;
		
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		// create tokenizer
		MsgPrinter.printStatusMsg("Creating tokenizer...");
		if (!OpenNLP.createTokenizer("res/nlp/tokenizer/opennlp/EnglishTok.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create tokenizer.");
//		LingPipe.createTokenizer();
		
		// create sentence detector
//		MsgPrinter.printStatusMsg("Creating sentence detector...");
//		if (!OpenNLP.createSentenceDetector("res/nlp/sentencedetector/opennlp/EnglishSD.bin.gz"))
//			MsgPrinter.printErrorMsg("Could not create sentence detector.");
//		LingPipe.createSentenceDetector();
		
		// create stemmer
		MsgPrinter.printStatusMsg("Creating stemmer...");
		SnowballStemmer.create();
		
		// create part of speech tagger
		MsgPrinter.printStatusMsg("Creating POS tagger...");
		if (!OpenNLP.createPosTagger("res/nlp/postagger/opennlp/tag.bin.gz",
									 "res/nlp/postagger/opennlp/tagdict"))
			MsgPrinter.printErrorMsg("Could not create OpenNLP POS tagger.");
//		if (!StanfordPosTagger.init("res/nlp/postagger/stanford/" +
//				"train-wsj-0-18.holder"))
//			MsgPrinter.printErrorMsg("Could not create Stanford POS tagger.");
		
		// create chunker
		MsgPrinter.printStatusMsg("Creating chunker...");
		if (!OpenNLP.createChunker("res/nlp/phrasechunker/opennlp/" +
								   "EnglishChunk.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create chunker.");
		
		// create named entity taggers
		MsgPrinter.printStatusMsg("Creating NE taggers...");
		NETagger.loadListTaggers("res/nlp/netagger/lists/");
		NETagger.loadRegExTaggers("res/nlp/netagger/patterns.lst");
		MsgPrinter.printStatusMsg("  ...loading models");
//		if (!NETagger.loadNameFinders("res/nlp/netagger/opennlp/"))
//			MsgPrinter.printErrorMsg("Could not create OpenNLP NE tagger.");
		if (!StanfordNeTagger.isInitialized() && !StanfordNeTagger.init())
			MsgPrinter.printErrorMsg("Could not create Stanford NE tagger.");
		MsgPrinter.printStatusMsg("  ...done");
		
//		WebTermImportanceFilter wtif = new TargetGeneratorTest();
//		TRECTarget[] targets = TREC13To16Parser.loadTargets(args[0]);
//		for (TRECTarget target : targets) {
//			String question = target.getTargetDesc();
//			
//			// query generation
//			MsgPrinter.printGeneratingQueries();
//			String qn = QuestionNormalizer.normalize(question);
//			MsgPrinter.printNormalization(qn);  // print normalized question string
//			Logger.logNormalization(qn);  // log normalized question string
//			String[] kws = KeywordExtractor.getKeywords(qn);
//			AnalyzedQuestion aq = new AnalyzedQuestion(question);
//			aq.setKeywords(kws);
//			aq.setFactoid(false);
//			
//			Query[] queries = new BagOfWordsG().generateQueries(aq);
//			for (int q = 0; q < queries.length; q++)
//				queries[q].setOriginalQueryString(question);
//			
//			Result[] results = new Result[1];
//			results[0] = new Result("This would be the answer", queries[0]);
//			wtif.apply(results);
//		}
		
		GoogleTermImportanceFilter gtif = new GoogleTermImportanceFilter(NO_NORMALIZATION, NO_NORMALIZATION, false);
		String[] targets = gtif.getTargets("Warren Moon");
		final HashMap<String, TermCounter> termCounters = gtif.getTermCounters(targets);
		ArrayList<String> termList = new ArrayList<String>(termCounters.keySet());
		Collections.sort(termList, new Comparator<String>() {
			public int compare(String o1, String o2) {
				int tc1 = termCounters.get(o1).getValue();
				int tc2 = termCounters.get(o2).getValue();
				return ((tc1 == tc2) ? o1.compareTo(o2) : (tc2 - tc1)); 
			}
		});
//		Iterator<String> terms = termCounters.keySet().iterator();
		Iterator<String> terms = termList.iterator();
		int atLeast5 = 0;
		while (terms.hasNext()) {
			String term = terms.next();
			int tc = termCounters.get(term).getValue();
			System.out.println(term + ": " + tc);
			if (tc > 4) atLeast5++;
		}
		System.out.println("At least 5 times: " + atLeast5);
	}
}
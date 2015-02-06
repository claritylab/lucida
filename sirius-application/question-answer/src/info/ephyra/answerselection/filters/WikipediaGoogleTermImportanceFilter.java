package info.ephyra.answerselection.filters;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.StanfordNeTagger;
import info.ephyra.search.searchers.GoogleKM;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;
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
 * <p>A web term importance filter that counts term frequencies in a Wikipedia
 * article on the target of the question. If no Wikipedia article is found, text
 * snippets retrieved with the Google search engine are used instead.</p>
 * 
 * <p>This class extends the class <code>WebTermImportanceFilter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class WikipediaGoogleTermImportanceFilter extends WebTermImportanceFilter {
	
	/** Google license key. */
	private static final String GOOGLE_KEY = "Enter your Google license key.";
	/** Maximum total number of search results. */
	private static final int MAX_RESULTS_TOTAL = 250;
	/** Maximum number of search results per query. */
	private static final int MAX_RESULTS_PERQUERY = 10;
	/** Number of retries if search fails. */
	private static final int RETRIES = 50;
	
	private static HashMap<String, TermCounter> missPageTermCounters = new HashMap<String, TermCounter>();
	
	private void initMissTerms() {
		if (missPageTermCounters.isEmpty()) try {
			BufferedReader br = new BufferedReader(new FileReader("./res/definitional/webreinforcement/wikipediaMissTerms"));
			String line;
			while ((line = br.readLine()) != null) {
				if (!missPageTermCounters.containsKey(line))
					missPageTermCounters.put(line, new TermCounter());
				missPageTermCounters.get(line).increment(1);
			}
			br.close();
		} catch (IOException ioe) {}
	}
	
	/**
	 * @param normalizationMode
	 * @param tfNormalizationMode
	 * @param isCombined
	 */
	public WikipediaGoogleTermImportanceFilter(int normalizationMode, int tfNormalizationMode, boolean isCombined) {
		super(normalizationMode, tfNormalizationMode, isCombined);
		this.initMissTerms();
	}
	
	/** @see info.ephyra.answerselection.filters.WebTermImportanceFilter#getTermCounters(java.lang.String[])
	 */
	@Override
	public HashMap<String, TermCounter> getTermCounters(String[] targets) {
		HashMap<String, TermCounter> termCounters = new HashMap<String, TermCounter>();
		
		//	process targets
		for (int t = 0; t < targets.length; t++) {
			String target = targets[t];
			HashMap<String, TermCounter> targetTermCounters = new HashMap<String, TermCounter>();
			
			//	skip quoted versions
			if (!target.startsWith("\"")) try {
				String url = "http://en.wikipedia.org/wiki/" + target.replaceAll("\\s", "_");
				URLConnection connection = new URL(url).openConnection();
				connection.setDoInput(true);
				connection.setDoOutput(true);
				connection.setUseCaches(false);
				connection.setRequestProperty("User-Agent", "Ephyra");
				connection.connect();
				
				BufferedReader reader = new BufferedReader(new InputStreamReader(connection.getInputStream()));
				boolean inTag = false;
				int c = 0;
				StringBuffer term = new StringBuffer();
				while ((c = reader.read()) != -1) {
					if (c == '<') {
						inTag = true;
						if (term.length() != 0) {
							String stemmedTerm = SnowballStemmer.stem(term.toString().toLowerCase());
//							System.out.println(stemmedTerm);
							if (!targetTermCounters.containsKey(stemmedTerm))
								targetTermCounters.put(stemmedTerm, new TermCounter());
							targetTermCounters.get(stemmedTerm).increment(1);
							term = new StringBuffer();
						}
					} else if (c == '>') {
						inTag = false;
					} else if (!inTag) {
						if (c < 33) {
							if (term.length() != 0) {
								String stemmedTerm = SnowballStemmer.stem(term.toString().toLowerCase());
//								System.out.println(stemmedTerm);
								if (!targetTermCounters.containsKey(stemmedTerm))
									targetTermCounters.put(stemmedTerm, new TermCounter());
								targetTermCounters.get(stemmedTerm).increment(1);
								term = new StringBuffer();
							}
						} else term.append((char) c);
					}
				}
				
			} catch (IOException e) {
				e.printStackTrace();
			}
			
			//	difference to terms on miss page over 10%
			int sum = getCountSum(targetTermCounters);
			int diff = sumDiff(targetTermCounters, missPageTermCounters);
			System.out.println("WikipediaGoogleWebTermImportanceFilter: sum is " + sum + ", diff is " + diff);
			
			//	unquoted target (otherwise sum = diff = 0), match in wikipedia
			if ((diff * 10) > sum)  {
				addTermCounters(targetTermCounters, termCounters);
				System.out.println("WikipediaGoogleWebTermImportanceFilter: found target '" + target + "' in wikipedia");
				if (t == 0) { // 20070713: do not look up further targets if wikipedia article on original target exists
					System.out.println("  ==> No further lookups needed");
					return termCounters;
				}
				
			//	otherwise, get term counters from google
			} else {
				System.out.println("WikipediaGoogleWebTermImportanceFilter: target '" + target + "' not found in wikipedia, doing Google lookup");
				addTermCounters(getGoogleTermCounters(target), termCounters);
			}
		}
		
		return termCounters;
	}
	
	private HashMap<String, TermCounter> getGoogleTermCounters(String target) {
		HashMap<String, TermCounter> targetTermCounters = new HashMap<String, TermCounter>();
		
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
						return targetTermCounters;
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
				String plain = elements[i].getSnippet().replaceAll("\\<[^\\>]++\\>", " ");
				plain = plain.replaceAll("\\&\\#39\\;", "'");
				if (TEST_TARGET_GENERATION) System.out.println(" - plain: " + plain);
				
				//	tokenize and tag sentence
				String[] sentence = NETagger.tokenize(plain);
				lengthSum += sentence.length;
				
				//	scan sentence for NPs
				for (int s = 0; s < sentence.length; s++) {
					String term = SnowballStemmer.stem(sentence[s].toLowerCase());
					if (term.length() > 1) {
						if (!targetTermCounters.containsKey(term))
							targetTermCounters.put(term, new TermCounter());
						targetTermCounters.get(term).increment();
					}
				}
			}
		}
		
		return targetTermCounters;
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
			MsgPrinter.printErrorMsg("Could not create POS tagger.");
		
		// create chunker
		MsgPrinter.printStatusMsg("Creating chunker...");
		if (!OpenNLP.createChunker("res/phrasechunker/opennlp/" +
								   "EnglishChunk.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create chunker.");
		
		// create named entity tagger
//		MsgPrinter.printStatusMsg("Creating NE taggers...");
//		if (!NETagger.createNameFinders("res/nlp/netagger/opennlp/"))
//			MsgPrinter.printErrorMsg("Could not create NE tagger.");
		
		//	create stanford named entity tagger 
		MsgPrinter.printStatusMsg("Creating Stanford NE tagger...");
		if (!StanfordNeTagger.isInitialized() && !StanfordNeTagger.init())
			MsgPrinter.printErrorMsg("Could not create Stanford NE tagger.");
		
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
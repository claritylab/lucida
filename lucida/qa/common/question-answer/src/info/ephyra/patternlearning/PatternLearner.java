package info.ephyra.patternlearning;

import info.ephyra.answerselection.filters.AnswerPatternFilter;
import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.StanfordNeTagger;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.nlp.indices.IrregularVerbs;
import info.ephyra.nlp.indices.Prepositions;
import info.ephyra.nlp.semantics.ontologies.WordNet;
import info.ephyra.querygeneration.Query;
import info.ephyra.querygeneration.generators.QuestionInterpretationG;
import info.ephyra.questionanalysis.QuestionInterpretation;
import info.ephyra.questionanalysis.QuestionInterpreter;
import info.ephyra.questionanalysis.QuestionNormalizer;
import info.ephyra.search.Result;
import info.ephyra.search.Search;
import info.ephyra.search.searchers.BingKM;
import info.ephyra.trec.TREC8To12Parser;
import info.ephyra.trec.TRECAnswer;
import info.ephyra.trec.TRECPattern;
import info.ephyra.trec.TRECQuestion;
import info.ephyra.util.FileUtils;
import info.ephyra.util.RegexConverter;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Hashtable;

/**
 * A pattern learning tool for Ephyra.
 * 
 * @author Nico Schlaefer
 * @version 2006-04-04
 */
public class PatternLearner {
	/** Support threshold for answer patterns. */
	private static final float SUPPORT_THRESH = 0.0001f;
	/** Confidence threshold for answer patterns. */
	private static final float CONFIDENCE_THRESH = 0.01f;
	/** Question strings. */
	private static String[] qss;
	/** Maps questions or query strings to answers. */
	private static Hashtable<String, String> ass;
	/** Maps questions or query strings to patterns for correct answers. */
	private static Hashtable<String, String> regexs;
	
	/**
	 * Loads the questions, answers and patterns from TREC files.
	 * 
	 * @param qFile name of the file containing the questions
	 * @param aFile name of the file containing the answers or an empty string
	 * @param pFile name of the file containing the patterns or an empty string
	 */
	private static void loadTRECData(String qFile, String aFile, String pFile) {
		ass = new Hashtable<String, String>();
		regexs = new Hashtable<String, String>();
		
		// load questions from file
		TRECQuestion[] questions = TREC8To12Parser.loadQuestions(qFile);
		qss = new String[questions.length];
		for (int i = 0; i < questions.length; i++)
			qss[i] = questions[i].getQuestionString();
		
		// if an answer file is provided,
		// load answers and derive patterns
		if (!aFile.equals("")) {
			TRECAnswer[] answers = TREC8To12Parser.loadTREC9Answers(aFile);
			String answer;
			
			for (int i = 0; i < questions.length; i++) {
				answer = answers[i].getAnswerString();
				ass.put(qss[i], answer);
				if (pFile.equals(""))
					regexs.put(qss[i], RegexConverter.strToRegex(answer));
			}
		}
		
		// if a patterns file is provided,
		// load patterns and derive answer strings
		if (!pFile.equals("")) {
			TRECPattern[] patterns = TREC8To12Parser.loadPatternsAligned(pFile);
			String pattern;
			
			for (int i = 0; i < questions.length; i++)
				if ((i < patterns.length) && (patterns[i] != null)) {
					pattern = patterns[i].getRegexs()[0];
					regexs.put(qss[i], pattern);
					if (aFile.equals(""))
						ass.put(qss[i],
								RegexConverter.regexToQueryStr(pattern));
				}
		}
	}
	
	/**
	 * Interprets the questions and writes target-context-answer-regex
	 * tuples to resource files.
	 * 
	 * @param dir target directory
	 * @return <code>true</code>, iff the interpretations could be written to
	 * 		   resource files
	 */
	private static boolean interpretQuestions(String dir) {
		boolean success = true;
		
		for (int i = 0; i < qss.length; i++) {
			// print original question string
			MsgPrinter.printQuestionString(qss[i]);
			
			// normalize question
			String qn = QuestionNormalizer.normalize(qss[i]);
			// stem verbs and nouns
			String stemmed = QuestionNormalizer.stemVerbsAndNouns(qn);
			
			// print normalized and stemmed question string
			MsgPrinter.printNormalization(stemmed);
			
			// interpret question
			QuestionInterpretation[] qis =
				QuestionInterpreter.interpret(qn, stemmed);
			MsgPrinter.printInterpretations(qis);
			
			for (QuestionInterpretation qi : qis) 
				if (!saveInterpretation(dir, qi, ass.get(qss[i]),
										regexs.get(qss[i])))
					success = false;
		}
		
		return success;
	}
	
	/**
	 * Saves a question interpretation, an answer string and a regular
	 * expression that describes a correct answer to a file.
	 * 
	 * @param dir target directory
	 * @param qi question interpretation
	 * @param as answer string
	 * @param regex regular expression
	 * @return <code>true</code>, iff the tuple could be saved
	 */
	private static boolean saveInterpretation(String dir,
			QuestionInterpretation qi, String as, String regex) {
		try {
			File file = new File(dir + "/" + qi.getProperty());
			
			// form tuple
			String tuple = qi.getTarget();
			for (String context : qi.getContext()) tuple += "#" + context;
			tuple += "#" + as;
			tuple += "#" + regex;
			
			// first check if the tuple already exists in the file
			if (file.exists()) {
				BufferedReader in = new BufferedReader(new FileReader(file));
				while (in.ready())
					if (tuple.equalsIgnoreCase(in.readLine())) return true;
				in.close();
			}
			
			// append new tuple
			PrintWriter out = new PrintWriter(new FileOutputStream(file, true));
			out.println(tuple);
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Loads target-context-answer-regex tuples from resource files and forms
	 * queries.
	 * 
	 * @param dir directory containing the target-context-answer-regex tuples
	 * @return queries formed from the tuples
	 */
	private static Query[] formQueries(String dir) {
		QuestionInterpretationG queryGenerator = new QuestionInterpretationG();
		ArrayList<Query> results = new ArrayList<Query>();
		
		File[] files = FileUtils.getFiles(dir);
		BufferedReader in;
		
		String[] tuple, context, kws;
		String prop, line, target, as, regex, queryString;
		QuestionInterpretation qi;
		Query query;
		
		try {
			for (File file : files) {
				prop = file.getName();
				in = new BufferedReader(new FileReader(file));
				
				while (in.ready()) {
					line = in.readLine().trim();
					if (line.length() == 0 || line.startsWith("//"))
						continue;  // skip blank lines and comments
					
					// extract interpretation, answer string and pattern
					tuple = line.split("#", -1);
					target = tuple[0];
					context = new String[tuple.length - 3];
					for (int i = 1; i < tuple.length - 2; i++)
						context[i - 1] = tuple[i];
					as = tuple[tuple.length - 2];
					regex = tuple[tuple.length - 1];
					
					// complement answer string or regular expression
					if (as.equals(""))
						as = RegexConverter.regexToQueryStr(regex);
					else if (regex.equals(""))
						regex =	RegexConverter.strToRegex(as);
					
					// create query object
					qi = new QuestionInterpretation(target, context, prop);
					kws = new String[] {"\"" + as + "\""};
					queryString = queryGenerator.queryString(target, context,
															 kws);
					query = new Query(queryString, null, 0);
					query.setInterpretation(qi);
					
					// store query, answer and regular expression
					results.add(query);
					ass.put(queryString, as);
					regexs.put(queryString, regex);
				}
			}
		} catch (IOException e) {
			return new Query[0];
		}
		
		return results.toArray(new Query[results.size()]);
	}
	
	/**
	 * Fetches text passages from knowledge sources.
	 * 
	 * @param queries the queries sent to the searchers
	 * @return results from the searchers
	 */
	private static Result[] fetchPassages(Query[] queries) {
		return Search.doSearch(queries);
	}
	
	/**
	 * Extracts answer patterns from the text passages in the search results.
	 * 
	 * @param results search results
	 */
	private static void extractPatterns(Result[] results) {
		String as;
		for (Result result : results) {
			as = ass.get(result.getQuery().getQueryString());
			PatternExtractor.extract(result, as);
		}
	}
	
	/**
	 * Saves answer patterns to resource files.
	 * 
	 * @param dir target directory
	 * @return <code>true</code>, iff the answer patterns could be saved
	 */
	private static boolean savePatterns(String dir) {
		return AnswerPatternFilter.savePatterns(dir);
	}
	
	/**
	 * Loads answer patterns from resource files.
	 * 
	 * @param dir directory containing the answer patterns
	 * @return <code>true</code>, iff the answer patterns could be loaded
	 */
	private static boolean loadPatterns(String dir) {
		return AnswerPatternFilter.loadPatterns(dir);
	}
	
	/**
	 * Assesses the answer patterns on the text passages in the
	 * <code>Result</code> objects.
	 * 
	 * @param results search results
	 */
	private static void assessPatterns(Result[] results) {
		String regex;
		for (Result result : results) {
			regex = regexs.get(result.getQuery().getQueryString());
			AnswerPatternFilter.assessPatterns(result, regex);
		}
	}
	
	/**
	 * Drops answer patterns that have a low support or confidence.
	 */
	private static void filterPatterns() {
		// drop answer patterns that have a low support
		AnswerPatternFilter.dropLowSupport(SUPPORT_THRESH);
		
		// drop answer patterns that have a low confidence
		AnswerPatternFilter.dropLowConfidence(CONFIDENCE_THRESH);
	}
	
	/**
	 * Initializes the pattern learning tool.
	 */
	public static void init() {
		MsgPrinter.printInitializing();
		
		// create tokenizer
		MsgPrinter.printStatusMsg("Creating tokenizer...");
		if (!OpenNLP.createTokenizer("res/nlp/tokenizer/opennlp/" +
									 "EnglishTok.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create tokenizer.");
//		LingPipe.createTokenizer();
		
		// create sentence detector
		MsgPrinter.printStatusMsg("Creating sentence detector...");
		if (!OpenNLP.createSentenceDetector("res/nlp/sentencedetector/" +
											"opennlp/EnglishSD.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create sentence detector.");
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
		
		// create syntactic parser
//		MsgPrinter.printStatusMsg("Creating syntactic parser...");
//		if (!OpenNLP.createParser("res/nlp/syntacticparser/opennlp/"))
//			MsgPrinter.printErrorMsg("Could not create OpenNLP parser.");
//		try {
//			StanfordParser.initialize();
//		} catch (Exception e) {
//			MsgPrinter.printErrorMsg("Could not create Stanford parser.");
//		}
		
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
		
		// create linker
//		MsgPrinter.printStatusMsg("Creating linker...");
//		if (!OpenNLP.createLinker("res/nlp/corefresolver/opennlp/"))
//			MsgPrinter.printErrorMsg("Could not create linker.");
		
		// create WordNet dictionary
		MsgPrinter.printStatusMsg("Creating WordNet dictionary...");
		if (!WordNet.initialize("res/ontologies/wordnet/file_properties.xml"))
			MsgPrinter.printErrorMsg("Could not create WordNet dictionary.");
		
		// load function words (numbers are excluded)
		MsgPrinter.printStatusMsg("Loading function verbs...");
		if (!FunctionWords.loadIndex("res/indices/functionwords_nonumbers"))
			MsgPrinter.printErrorMsg("Could not load function words.");
		
		// load prepositions
		MsgPrinter.printStatusMsg("Loading prepositions...");
		if (!Prepositions.loadIndex("res/indices/prepositions"))
			MsgPrinter.printErrorMsg("Could not load prepositions.");
		
		// load irregular verbs
		MsgPrinter.printStatusMsg("Loading irregular verbs...");
		if (!IrregularVerbs.loadVerbs("res/indices/irregularverbs"))
			MsgPrinter.printErrorMsg("Could not load irregular verbs.");
		
		// load question patterns
		MsgPrinter.printStatusMsg("Loading question patterns...");
		if (!QuestionInterpreter.loadPatterns("res/patternlearning/" +
											  "questionpatterns/"))
			MsgPrinter.printErrorMsg("Could not load question patterns.");
		
		// add knowledge miners used to fetch text passages for pattern learning
		MsgPrinter.printStatusMsg("Adding BingKM...");
		Search.addKnowledgeMiner(new BingKM());
//		MsgPrinter.printStatusMsg("Adding GoogleKM...");
//		Search.addKnowledgeMiner(new GoogleKM());
//		MsgPrinter.printStatusMsg("Adding YahooKM...");
//		Search.addKnowledgeMiner(new YahooKM());
//		MsgPrinter.printStatusMsg("Adding IndriKMs...");
//		for (String[] indriIndices : IndriKM.getIndriIndices())
//			Search.addKnowledgeMiner(new IndriKM(indriIndices, false));
//		for (String[] indriServers : IndriKM.getIndriServers())
//			Search.addKnowledgeMiner(new IndriKM(indriServers, true));
	}
	
	/**
	 * Loads the TREC data, interprets the questions and writes
	 * target-context-answer-regex tuples to files.
	 * 
	 * @param qFile name of the file containing the questions
	 * @param aFile name of the file containing the answers or an empty string
	 * @param pFile name of the file containing the patterns or an empty string
	 * @return <code>true, iff the TREC data could be interpreted
	 */
	public static boolean interpret(String qFile, String aFile, String pFile) {
		// load TREC data
		MsgPrinter.printLoadingTRECData();
		loadTRECData(qFile, aFile, pFile);
		
		// interpret TREC questions and save interpretations to files
		MsgPrinter.printInterpretingQuestions();
		return interpretQuestions("res/patternlearning/interpretations");
	}
	
	/**
	 * Loads target-context-answer-regex tuples from resource files, forms
	 * queries, fetches text passages, extracts answer patterns and writes them
	 * to resource files.
	 * 
	 * @return <code>true</code>, iff the answer patterns could be extracted
	 */
	public static boolean extract() {
		// load tuples and form queries
		MsgPrinter.printFormingQueries();
		ass = new Hashtable<String, String>();
		regexs = new Hashtable<String, String>();
		Query[] queries;
		ArrayList<Query> queryList = new ArrayList<Query>();
		queries = formQueries("res/patternlearning/interpretations");
		for (Query query : queries) queryList.add(query);
		queries = formQueries("res/patternlearning/interpretations_extract");
		for (Query query : queries) queryList.add(query);
		queries = queryList.toArray(new Query[queryList.size()]);
		
		// fetch text passages
		MsgPrinter.printFetchingPassages();
		Result[] results = fetchPassages(queries);
		
		// extract answer patterns
		MsgPrinter.printExtractingPatterns();
		extractPatterns(results);
		
		// save answer patterns
		MsgPrinter.printSavingPatterns();
		return savePatterns("res/patternlearning/answerpatterns_extract");
	}
	
	/**
	 * Loads target-context-answer-regex tuples and answer patterns from
	 * resource files, forms queries from the tuples, fetches text passages,
	 * assesses the answer patterns on the text passages and writes them to
	 * resource files.
	 * 
	 * @return <code>true</code>, iff the answer patterns could be assessed
	 */
	public static boolean assess() {
		// load answer patterns
		MsgPrinter.printLoadingPatterns();
		if (!loadPatterns("res/patternlearning/answerpatterns_extract"))
			return false;
		
		// load tuples and form queries
		MsgPrinter.printFormingQueries();
		ass = new Hashtable<String, String>();
		regexs = new Hashtable<String, String>();
		Query[] queries;
		ArrayList<Query> queryList = new ArrayList<Query>();
		queries = formQueries("res/patternlearning/interpretations");
		for (Query query : queries) queryList.add(query);
		queries = formQueries("res/patternlearning/interpretations_assess");
		for (Query query : queries) queryList.add(query);
		queries = queryList.toArray(new Query[queryList.size()]);
		
		// fetch text passages
		MsgPrinter.printFetchingPassages();
		Result[] results = fetchPassages(queries);
		
		// assess answer patterns
		MsgPrinter.printAssessingPatterns();
		assessPatterns(results);
		
		// save answer patterns
		MsgPrinter.printSavingPatterns();
		return savePatterns("res/patternlearning/answerpatterns_assess");
	}
	
	/**
	 * Loads answer patterns from resource files, drops patterns with a low
	 * support or confidence and writes the remaining patterns back to resource
	 * files.
	 * 
	 * @return <code>true</code>, iff the answer patterns could be filtered
	 */
	public static boolean filter() {
		// load answer patterns
		MsgPrinter.printLoadingPatterns();
		if (!loadPatterns("res/patternlearning/answerpatterns_assess"))
			return false;
		
		// drop patterns with low support/confidence
		MsgPrinter.printFilteringPatterns();
		filterPatterns();
		
		// save answer patterns
		MsgPrinter.printSavingPatterns();
		return savePatterns("res/patternlearning/answerpatterns");
	}
	
	/**
	 * <p>Entry point of the program.</p>
	 * 
	 * <p>Learns and assesses answer patterns using questions and patterns from
	 * the TREC QA track as training data.</p>
	 * 
	 * @param args argument 1: name of the question file<br>
	 * 			   argument 2: name of the answer file<br>
	 * 			   argument 3: name of the file containing the patterns
	 */
	public static void main(String[] args) {
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
//		if (args.length < 3) {
//			MsgPrinter.printUsage("java PatternLearner question_file " +
//								  "answer_file pattern_file");
//			System.exit(1);
//		}
		
		// initialize the system
		init();
		
		// interpret TREC data
//		interpret(args[0], args[1], args[2]);
		
		// extract answer patterns
		extract();
		
		// assess answer patterns
		assess();
		
		// filter answer patterns
		filter();
	}
}

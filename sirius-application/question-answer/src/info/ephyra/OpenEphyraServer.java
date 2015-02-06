package info.ephyra;

import info.ephyra.answerselection.AnswerSelection;
import info.ephyra.answerselection.filters.AnswerPatternFilter;
import info.ephyra.answerselection.filters.AnswerTypeFilter;
import info.ephyra.answerselection.filters.DuplicateFilter;
import info.ephyra.answerselection.filters.FactoidSubsetFilter;
import info.ephyra.answerselection.filters.FactoidsFromPredicatesFilter;
import info.ephyra.answerselection.filters.PredicateExtractionFilter;
import info.ephyra.answerselection.filters.QuestionKeywordsFilter;
import info.ephyra.answerselection.filters.ScoreCombinationFilter;
import info.ephyra.answerselection.filters.ScoreNormalizationFilter;
import info.ephyra.answerselection.filters.ScoreSorterFilter;
import info.ephyra.answerselection.filters.StopwordFilter;
import info.ephyra.answerselection.filters.TruncationFilter;
import info.ephyra.answerselection.filters.WebDocumentFetcherFilter;
import info.ephyra.io.Logger;
import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.LingPipe;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.StanfordNeTagger;
import info.ephyra.nlp.StanfordParser;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.nlp.indices.IrregularVerbs;
import info.ephyra.nlp.indices.Prepositions;
import info.ephyra.nlp.indices.WordFrequencies;
import info.ephyra.nlp.semantics.ontologies.Ontology;
import info.ephyra.nlp.semantics.ontologies.WordNet;
import info.ephyra.querygeneration.Query;
import info.ephyra.querygeneration.QueryGeneration;
import info.ephyra.querygeneration.generators.BagOfTermsG;
import info.ephyra.querygeneration.generators.BagOfWordsG;
import info.ephyra.querygeneration.generators.PredicateG;
import info.ephyra.querygeneration.generators.QuestionInterpretationG;
import info.ephyra.querygeneration.generators.QuestionReformulationG;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.QuestionAnalysis;
import info.ephyra.questionanalysis.QuestionInterpreter;
import info.ephyra.questionanalysis.QuestionNormalizer;
import info.ephyra.search.Result;
import info.ephyra.search.Search;
import info.ephyra.search.searchers.IndriKM;
import info.ephyra.search.searchers.IndriDocumentKM;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.eclipse.jetty.server.nio.SelectChannelConnector;
import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.AbstractHandler;
import org.eclipse.jetty.util.thread.QueuedThreadPool;

import java.net.URLDecoder;
import java.io.PrintWriter;

import java.io.IOException;

import java.util.ArrayList;

/**
 * <code>OpenEphyra</code> is an open framework for question answering (QA).
 * 
 * @author Nico Schlaefer
 * @version 2008-03-23
 */
public class OpenEphyraServer extends AbstractHandler {
	/** Factoid question type. */
	protected static final String FACTOID = "FACTOID";
	/** List question type. */
	protected static final String LIST = "LIST";
	
	/** Maximum number of factoid answers. */
	protected static final int FACTOID_MAX_ANSWERS = 1;
	/** Absolute threshold for factoid answer scores. */
	protected static final float FACTOID_ABS_THRESH = 0;
	/** Relative threshold for list answer scores (fraction of top score). */
	protected static final float LIST_REL_THRESH = 0.1f;
	
	/** Serialized classifier for score normalization. */
	public static final String NORMALIZER =
		"res/scorenormalization/classifiers/" +
		"AdaBoost70_" +
		"Score+Extractors_" +
		"TREC10+TREC11+TREC12+TREC13+TREC14+TREC15+TREC8+TREC9" +
		".serialized";
	
	/** The directory of Ephyra, required when Ephyra is used as an API. */
	protected String dir;
	

        @Override
        public void handle(String target,
                       Request baseRequest,
                       HttpServletRequest request,
                       HttpServletResponse response) 
            throws IOException, ServletException
        {        
             
            String query_str = request.getQueryString();

            System.out.println("Query str: " + query_str);

            if (query_str == null) {
		    response.setContentType("text/html;charset=utf-8");
		    response.setStatus(HttpServletResponse.SC_OK);
		    baseRequest.setHandled(true);
		    return;
            }           

            String[] tokens = query_str.split("=");
            String question = URLDecoder.decode(tokens[1], "UTF-8");

            // response
            response.setContentType("text/html;charset=utf-8");
            response.setStatus(HttpServletResponse.SC_OK);
            baseRequest.setHandled(true);

            PrintWriter out = response.getWriter();
            out.flush();

		// determine question type and extract question string
		String type;
		if (question.matches("(?i)" + FACTOID + ":.*+")) {
			// factoid question
			type = FACTOID;
			question = question.split(":", 2)[1].trim();
		} else if (question.matches("(?i)" + LIST + ":.*+")) {
			// list question
			type = LIST;
			question = question.split(":", 2)[1].trim();
		} else {
			// question type unspecified
			type = FACTOID;  // default type
		}

		// ask question
		Result[] results = new Result[0];
		if (type.equals(FACTOID)) {
			Logger.logFactoidStart(question);
			results = askFactoid(question, FACTOID_MAX_ANSWERS,
					FACTOID_ABS_THRESH);
			Logger.logResults(results);
			Logger.logFactoidEnd();
		} else if (type.equals(LIST)) {
			Logger.logListStart(question);
			results = askList(question, LIST_REL_THRESH);
			Logger.logResults(results);
			Logger.logListEnd();
		}

        String answer = results[0].getAnswer();

        if (answer != null)
            out.println(answer);
        else
            out.println("Sorry, I cannot answer your question."); 

        out.close();
        }

	/**
	 * Entry point of Ephyra. Initializes the engine and starts the web service interface.
	 * 
	 * @param args command line arguments are ignored
	 */
	public static void main(String[] args) throws Exception {
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		// set log file and enable logging
		Logger.setLogfile("log/OpenEphyra");
		Logger.enableLogging(true);
		

        String addr = "localhost";
        int port = 8080;
        if (args.length > 1) {
            addr = args[0];
            port = Integer.parseInt(args[1]);
        }
        int NTHREADS = Integer.parseInt(System.getenv("THREADS"));

        Server server = new Server();
        SelectChannelConnector con1 = new SelectChannelConnector();
        con1.setHost(addr);
        con1.setPort(port);
        con1.setThreadPool(new QueuedThreadPool(NTHREADS));
        con1.setMaxIdleTime(30000);
        con1.setRequestHeaderSize(8192);

        server.setConnectors(new Connector[]{con1});
		server.setHandler(new OpenEphyraServer()); 
	 
		server.start();
		server.join();
	}
	
	/**
	 * <p>Creates a new instance of Ephyra and initializes the system.</p>
	 * 
	 * <p>For use as a standalone system.</p>
	 */
	protected OpenEphyraServer() {
		this("");
	}
	
	/**
	 * <p>Creates a new instance of Ephyra and initializes the system.</p>
	 * 
	 * <p>For use as an API.</p>
	 * 
	 * @param dir directory of Ephyra
	 */
	public OpenEphyraServer(String dir) {
		this.dir = dir;
		
		MsgPrinter.printInitializing();
		
		// create tokenizer
		MsgPrinter.printStatusMsg("Creating tokenizer...");
		if (!OpenNLP.createTokenizer(dir +
				"res/nlp/tokenizer/opennlp/EnglishTok.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create tokenizer.");
		
		// create sentence detector
		MsgPrinter.printStatusMsg("Creating sentence detector...");
		if (!OpenNLP.createSentenceDetector(dir +
				"res/nlp/sentencedetector/opennlp/EnglishSD.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create sentence detector.");
		LingPipe.createSentenceDetector();
		
		// create stemmer
		MsgPrinter.printStatusMsg("Creating stemmer...");
		SnowballStemmer.create();
		
		// create part of speech tagger
		MsgPrinter.printStatusMsg("Creating POS tagger...");
		if (!OpenNLP.createPosTagger(
				dir + "res/nlp/postagger/opennlp/tag.bin.gz",
				dir + "res/nlp/postagger/opennlp/tagdict"))
			MsgPrinter.printErrorMsg("Could not create OpenNLP POS tagger.");
		
		// create chunker
		MsgPrinter.printStatusMsg("Creating chunker...");
		if (!OpenNLP.createChunker(dir +
				"res/nlp/phrasechunker/opennlp/EnglishChunk.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create chunker.");
		
		// create syntactic parser
		MsgPrinter.printStatusMsg("Creating syntactic parser...");
		try {
			StanfordParser.initialize();
		} catch (Exception e) {
			MsgPrinter.printErrorMsg("Could not create Stanford parser.");
		}
		
		// create named entity taggers
		MsgPrinter.printStatusMsg("Creating NE taggers...");
		NETagger.loadListTaggers(dir + "res/nlp/netagger/lists/");
		NETagger.loadRegExTaggers(dir + "res/nlp/netagger/patterns.lst");
		MsgPrinter.printStatusMsg("  ...loading models");
		if (!StanfordNeTagger.isInitialized() && !StanfordNeTagger.init())
			MsgPrinter.printErrorMsg("Could not create Stanford NE tagger.");
		MsgPrinter.printStatusMsg("  ...done");
		
		// create WordNet dictionary
		MsgPrinter.printStatusMsg("Creating WordNet dictionary...");
		if (!WordNet.initialize(dir +
				"res/ontologies/wordnet/file_properties.xml"))
			MsgPrinter.printErrorMsg("Could not create WordNet dictionary.");
		
		// load function words (numbers are excluded)
		MsgPrinter.printStatusMsg("Loading function verbs...");
		if (!FunctionWords.loadIndex(dir +
				"res/indices/functionwords_nonumbers"))
			MsgPrinter.printErrorMsg("Could not load function words.");
		
		// load prepositions
		MsgPrinter.printStatusMsg("Loading prepositions...");
		if (!Prepositions.loadIndex(dir +
				"res/indices/prepositions"))
			MsgPrinter.printErrorMsg("Could not load prepositions.");
		
		// load irregular verbs
		MsgPrinter.printStatusMsg("Loading irregular verbs...");
		if (!IrregularVerbs.loadVerbs(dir + "res/indices/irregularverbs"))
			MsgPrinter.printErrorMsg("Could not load irregular verbs.");
		
		// load word frequencies
		MsgPrinter.printStatusMsg("Loading word frequencies...");
		if (!WordFrequencies.loadIndex(dir + "res/indices/wordfrequencies"))
			MsgPrinter.printErrorMsg("Could not load word frequencies.");
		
		// load query reformulators
		MsgPrinter.printStatusMsg("Loading query reformulators...");
		if (!QuestionReformulationG.loadReformulators(dir +
				"res/reformulations/"))
			MsgPrinter.printErrorMsg("Could not load query reformulators.");
		
		// load question patterns
		MsgPrinter.printStatusMsg("Loading question patterns...");
		if (!QuestionInterpreter.loadPatterns(dir +
				"res/patternlearning/questionpatterns/"))
			MsgPrinter.printErrorMsg("Could not load question patterns.");
		
		// load answer patterns
		MsgPrinter.printStatusMsg("Loading answer patterns...");
		if (!AnswerPatternFilter.loadPatterns(dir +
				"res/patternlearning/answerpatterns/"))
			MsgPrinter.printErrorMsg("Could not load answer patterns.");
	}
	
	
	/**
	 * Initializes the pipeline for factoid questions.
	 */
	protected void initFactoid() {
		// question analysis
		Ontology wordNet = new WordNet();
		// - dictionaries for term extraction
		QuestionAnalysis.clearDictionaries();
		QuestionAnalysis.addDictionary(wordNet);
		// - ontologies for term expansion
		QuestionAnalysis.clearOntologies();
		QuestionAnalysis.addOntology(wordNet);
		
		// query generation
		QueryGeneration.clearQueryGenerators();
		QueryGeneration.addQueryGenerator(new BagOfWordsG());
		QueryGeneration.addQueryGenerator(new BagOfTermsG());
		QueryGeneration.addQueryGenerator(new PredicateG());
		QueryGeneration.addQueryGenerator(new QuestionInterpretationG());
		QueryGeneration.addQueryGenerator(new QuestionReformulationG());
		
        // search
        // - knowledge miners for unstructured knowledge sources
        Search.clearKnowledgeMiners();

        for (String[] indriIndices : IndriKM.getIndriIndices())
            Search.addKnowledgeMiner(new IndriKM(indriIndices, false));

        // - knowledge annotators for (semi-)structured knowledge sources
        Search.clearKnowledgeAnnotators();
        /* Search.addKnowledgeAnnotator(new WikipediaKA("list.txt")); */

        // answer extraction and selection
        // (the filters are applied in this order)
        AnswerSelection.clearFilters();
        // - answer extraction filters
        AnswerSelection.addFilter(new AnswerTypeFilter());
        AnswerSelection.addFilter(new AnswerPatternFilter());
        AnswerSelection.addFilter(new PredicateExtractionFilter());
        AnswerSelection.addFilter(new FactoidsFromPredicatesFilter());
        AnswerSelection.addFilter(new TruncationFilter());
        // - answer selection filters
        AnswerSelection.addFilter(new StopwordFilter());
        AnswerSelection.addFilter(new QuestionKeywordsFilter());
        AnswerSelection.addFilter(new ScoreNormalizationFilter(NORMALIZER));
        AnswerSelection.addFilter(new ScoreCombinationFilter());
        AnswerSelection.addFilter(new FactoidSubsetFilter());
        AnswerSelection.addFilter(new DuplicateFilter());
        AnswerSelection.addFilter(new ScoreSorterFilter());
	}
	
	/**
	 * Runs the pipeline and returns an array of up to <code>maxAnswers</code>
	 * results that have a score of at least <code>absThresh</code>.
	 * 
	 * @param aq analyzed question
	 * @param maxAnswers maximum number of answers
	 * @param absThresh absolute threshold for scores
	 * @return array of results
	 */
	protected Result[] runPipeline(AnalyzedQuestion aq, int maxAnswers,
								  float absThresh) {
		// query generation
		MsgPrinter.printGeneratingQueries();
		Query[] queries = QueryGeneration.getQueries(aq);
		
		// search
		MsgPrinter.printSearching();
		Result[] results = Search.doSearch(queries);
		
		// answer selection
		MsgPrinter.printSelectingAnswers();
		results = AnswerSelection.getResults(results, maxAnswers, absThresh);
		
		return results;
	}
	
	/**
	 * Returns the directory of Ephyra.
	 * 
	 * @return directory
	 */
	public String getDir() {
		return dir;
	}
	
	
	/**
	 * Asks Ephyra a factoid question and returns up to <code>maxAnswers</code>
	 * results that have a score of at least <code>absThresh</code>.
	 * 
	 * @param question factoid question
	 * @param maxAnswers maximum number of answers
	 * @param absThresh absolute threshold for scores
	 * @return array of results
	 */
	public Result[] askFactoid(String question, int maxAnswers,
							   float absThresh) {
		// initialize pipeline
		initFactoid();
		
		// analyze question
		MsgPrinter.printAnalyzingQuestion();
		AnalyzedQuestion aq = QuestionAnalysis.analyze(question);
		
		// get answers
		Result[] results = runPipeline(aq, maxAnswers, absThresh);
		
		return results;
	}
	
	/**
	 * Asks Ephyra a factoid question and returns a single result or
	 * <code>null</code> if no answer could be found.
	 * 
	 * @param question factoid question
	 * @return single result or <code>null</code>
	 */
	public Result askFactoid(String question) {
		Result[] results = askFactoid(question, 1, 0);
		
		return (results.length > 0) ? results[0] : null;
	}
	
	/**
	 * Asks Ephyra a list question and returns results that have a score of at
	 * least <code>relThresh * top score</code>.
	 * 
	 * @param question list question
	 * @param relThresh relative threshold for scores
	 * @return array of results
	 */
	public Result[] askList(String question, float relThresh) {
		question = QuestionNormalizer.transformList(question);
		
		Result[] results = askFactoid(question, Integer.MAX_VALUE, 0);
		
		// get results with a score of at least relThresh * top score
		ArrayList<Result> confident = new ArrayList<Result>();
		if (results.length > 0) {
			float topScore = results[0].getScore();
			
			for (Result result : results)
				if (result.getScore() >= relThresh * topScore)
					confident.add(result);
		}
		
		return confident.toArray(new Result[confident.size()]);
	}
}

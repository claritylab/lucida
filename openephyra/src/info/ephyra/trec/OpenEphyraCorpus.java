package info.ephyra.trec;

import info.ephyra.OpenEphyra;
import info.ephyra.answerselection.AnswerSelection;
import info.ephyra.answerselection.filters.AnswerPatternFilter;
import info.ephyra.answerselection.filters.AnswerProjectionFilter;
import info.ephyra.answerselection.filters.AnswerTypeFilter;
import info.ephyra.answerselection.filters.DuplicateFilter;
import info.ephyra.answerselection.filters.FactoidSubsetFilter;
import info.ephyra.answerselection.filters.FactoidsFromPredicatesFilter;
import info.ephyra.answerselection.filters.PredicateExtractionFilter;
import info.ephyra.answerselection.filters.QuestionKeywordsFilter;
import info.ephyra.answerselection.filters.ResultLengthFilter;
import info.ephyra.answerselection.filters.ScoreCombinationFilter;
import info.ephyra.answerselection.filters.ScoreNormalizationFilter;
import info.ephyra.answerselection.filters.ScoreSorterFilter;
import info.ephyra.answerselection.filters.StopwordFilter;
import info.ephyra.answerselection.filters.TruncationFilter;
import info.ephyra.answerselection.filters.WebDocumentFetcherFilter;
import info.ephyra.io.Logger;
import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.semantics.ontologies.Ontology;
import info.ephyra.nlp.semantics.ontologies.WordNet;
import info.ephyra.querygeneration.QueryGeneration;
import info.ephyra.querygeneration.generators.BagOfTermsG;
import info.ephyra.querygeneration.generators.BagOfWordsG;
import info.ephyra.querygeneration.generators.PredicateG;
import info.ephyra.querygeneration.generators.QuestionInterpretationG;
import info.ephyra.querygeneration.generators.QuestionReformulationG;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.QuestionAnalysis;
import info.ephyra.questionanalysis.QuestionNormalizer;
import info.ephyra.search.Result;
import info.ephyra.search.Search;
import info.ephyra.search.searchers.BingKM;
import info.ephyra.search.searchers.IndriKM;

import java.util.ArrayList;

/**
 * <p>A modified version of <code>OpenEphyra</code> that is optimized for the
 * TREC evaluation. If no answers are found, the question is assumed to ask for
 * a proper name and the pipeline is rerun to improve the recall. This setup
 * extracts answers from the Web and projects them onto a local corpus.</p>
 * 
 * <p>This class extends <code>OpenEphyra</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-01-26
 */
public class OpenEphyraCorpus extends OpenEphyra {
	/**
	 * Entry point of Ephyra. Initializes the engine and starts the command line
	 * interface.
	 * 
	 * @param args command line arguments are ignored
	 */
	public static void main(String[] args) {
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		// set log file and enable logging
		Logger.setLogfile("log/OpenEphyraCorpus");
		Logger.enableLogging(true);
		
		// initialize Ephyra and start command line interface
		(new OpenEphyraCorpus()).commandLine("");
	}
	
	/**
	 * Initializes the pipeline for factoid questions, using a local corpus as a
	 * knowledge source.
	 */
	protected void initFactoidCorpus() {
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
		for (String[] indriServers : IndriKM.getIndriServers())
			Search.addKnowledgeMiner(new IndriKM(indriServers, true));
		// - knowledge annotators for (semi-)structured knowledge sources
		Search.clearKnowledgeAnnotators();
		
		// answer extraction and selection
		// (the filters are applied in this order)
		AnswerSelection.clearFilters();
		// - answer extraction filters
		AnswerSelection.addFilter(new AnswerTypeFilter());
		AnswerSelection.addFilter(new AnswerPatternFilter());
		AnswerSelection.addFilter(new WebDocumentFetcherFilter());
		AnswerSelection.addFilter(new PredicateExtractionFilter());
		AnswerSelection.addFilter(new FactoidsFromPredicatesFilter());
		AnswerSelection.addFilter(new TruncationFilter());
		// - answer selection filters
	}
	
	/**
	 * Initializes the pipeline for factoid questions, using the Web as a
	 * knowledge source.
	 * 
	 * @param resultsCorp results retrieved from the corpus
	 */
	protected void initFactoidWeb(Result[] resultsCorp) {
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
		Search.addKnowledgeMiner(new BingKM());
//		Search.addKnowledgeMiner(new GoogleKM());
//		Search.addKnowledgeMiner(new YahooKM());
		// - knowledge annotators for (semi-)structured knowledge sources
		Search.clearKnowledgeAnnotators();
		
		// answer extraction and selection
		// (the filters are applied in this order)
		AnswerSelection.clearFilters();
		// - answer extraction filters
		AnswerSelection.addFilter(new AnswerTypeFilter());
		AnswerSelection.addFilter(new AnswerPatternFilter());
		AnswerSelection.addFilter(new WebDocumentFetcherFilter());
		AnswerSelection.addFilter(new PredicateExtractionFilter());
		AnswerSelection.addFilter(new FactoidsFromPredicatesFilter());
		AnswerSelection.addFilter(new TruncationFilter());
		// - answer selection filters
		AnswerSelection.addFilter(new StopwordFilter());
		AnswerSelection.addFilter(new QuestionKeywordsFilter());
		AnswerSelection.addFilter(new AnswerProjectionFilter(resultsCorp));
		AnswerSelection.addFilter(new ScoreNormalizationFilter(NORMALIZER));
		AnswerSelection.addFilter(new ScoreCombinationFilter());
		AnswerSelection.addFilter(new FactoidSubsetFilter());
		AnswerSelection.addFilter(new DuplicateFilter());
		AnswerSelection.addFilter(new ScoreSorterFilter());
		AnswerSelection.addFilter(new ResultLengthFilter());
	}
	
	/**
	 * Asks Ephyra a factoid question and returns up to <code>maxAnswers</code>
	 * results that have a score of at least <code>absThresh</code>. This method
	 * is optimized for the TREC evaluation: if the answer type cannot be
	 * determined and no answers are found, it simply returns a list of proper
	 * names.
	 * 
	 * @param question factoid question
	 * @param maxAnswers maximum number of answers
	 * @param absThresh absolute threshold for scores
	 * @return array of results
	 */
	public Result[] askFactoid(String question, int maxAnswers,
							   float absThresh) {
		// initialize pipeline
		initFactoidCorpus();
		
		// analyze question
		MsgPrinter.printAnalyzingQuestion();
		AnalyzedQuestion aq = QuestionAnalysis.analyze(question);
		
		// get corpus answers
		Result[] resultsCorp = runPipeline(aq, Integer.MAX_VALUE,
				Float.NEGATIVE_INFINITY);
		
		// get web answers and project them
		initFactoidWeb(resultsCorp);
		Result[] results = runPipeline(aq, maxAnswers, absThresh);
		
		// return results if any
		if (results.length > 0) return results;
		
		if (aq.getAnswerTypes().length == 0) {
			// assume that question asks for a proper name
			aq.setAnswerTypes(new String[] {"NEproperName"});
			
			// get corpus answers (only factoid answers)
			initFactoidCorpus();
			resultsCorp = runPipeline(aq, Integer.MAX_VALUE, 0);
			
			// get web answers and project them
			initFactoidWeb(resultsCorp);
			results = runPipeline(aq, maxAnswers, absThresh);
		}
		
		return results;
	}
	
	/**
	 * Asks Ephyra a list question and returns results that have a score of at
	 * least <code>relThresh * top score</code>. This method is optimized for
	 * the TREC evaluation: if no answers are found, it simply returns a list
	 * of proper names.
	 * 
	 * @param question list question
	 * @param relThresh relative threshold for scores
	 * @return array of results
	 */
	public Result[] askList(String question, float relThresh) {
		question = QuestionNormalizer.transformList(question);
		
		Result[] results = askFactoid(question, Integer.MAX_VALUE, 0);
		
		if (results.length == 0) {
			// assume that question asks for proper names
			AnalyzedQuestion aq = QuestionAnalysis.analyze(question);
			aq.setAnswerTypes(new String[] {"NEproperName"});
			
			// get corpus answers (only factoid answers)
			initFactoidCorpus();
			Result[] resultsCorp = runPipeline(aq, Integer.MAX_VALUE, 0);
			
			// get web answers and project them
			initFactoidWeb(resultsCorp);
			results = runPipeline(aq, Integer.MAX_VALUE, 0);
		}
		
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

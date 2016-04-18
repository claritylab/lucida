package info.ephyra.trec;

import info.ephyra.answerselection.AnswerSelection;
import info.ephyra.answerselection.definitional.Dossier;
import info.ephyra.answerselection.filters.CutKeywordsFilter;
import info.ephyra.answerselection.filters.CutStatementProviderFilter;
import info.ephyra.answerselection.filters.DirectSpeechFilter;
import info.ephyra.answerselection.filters.DuplicateSnippetFilter;
import info.ephyra.answerselection.filters.NuggetEvaluationFilter;
import info.ephyra.answerselection.filters.OverlapAnalysisFilter;
import info.ephyra.answerselection.filters.ProperNameFilter;
import info.ephyra.answerselection.filters.ResultLengthFilter;
import info.ephyra.answerselection.filters.ScoreResetterFilter;
import info.ephyra.answerselection.filters.ScoreSorterFilter;
import info.ephyra.answerselection.filters.SentenceExtractionFilter;
import info.ephyra.answerselection.filters.SentenceSplitterFilter;
import info.ephyra.answerselection.filters.TermFilter;
import info.ephyra.answerselection.filters.WebTermImportanceFilter;
import info.ephyra.answerselection.filters.WikipediaGoogleTermImportanceFilter;
import info.ephyra.io.Logger;
import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.querygeneration.Query;
import info.ephyra.querygeneration.QueryGeneration;
import info.ephyra.querygeneration.generators.BagOfWordsG;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.KeywordExtractor;
import info.ephyra.questionanalysis.QuestionAnalysis;
import info.ephyra.questionanalysis.QuestionInterpretation;
import info.ephyra.questionanalysis.QuestionInterpreter;
import info.ephyra.questionanalysis.QuestionNormalizer;
import info.ephyra.search.Result;
import info.ephyra.search.Search;
import info.ephyra.search.searchers.IndriKM;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>Runs and evaluates Ephyra on the data from the TREC 13-16 QA tracks.</p>
 * 
 * <p>This class extends <code>OpenEphyraCorpus</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-03-23
 */
public class EphyraTREC13To16 extends OpenEphyraCorpus {
	/** Maximum number of factoid answers. */
	protected static final int FACTOID_MAX_ANSWERS = 1;
	/** Absolute threshold for factoid answer scores. */
	protected static final float FACTOID_ABS_THRESH = 0;
	/** Relative threshold for list answer scores (fraction of top score). */
	protected static final float LIST_REL_THRESH = 0.1f;
	
	/** Target objects containing the TREC questions. */
	private static TRECTarget[] targets;
	/** Tag that uniquely identifies the run (also used as output file name). */
	private static String runTag;
	/** Log file for the results returned by Ephyra. */
	private static String logFile;
	
	/** Log file used as a source for answers to some of the questions. */
	private static String inputLogFile;
	/** Load answers to factoid questions from log file? */
	private static boolean factoidLog = false;
	/** Load answers to list questions from log file? */
	private static boolean listLog = false;
	/** Load answers to "Other" questions from log file? */
	private static boolean otherLog = false;
	
	/** Patterns for factoid questions (optional). */
	private static TRECPattern[] factoidPatterns;
	/** Patterns for list questions (optional). */
	private static TRECPattern[] listPatterns;
	/** Scores for the factoid questions within a target. */
	private static ArrayList<Float> factoidQuestionScores =
		new ArrayList<Float>();
	/** Scores for the list questions within a target. */
	private static ArrayList<Float> listQuestionScores =
		new ArrayList<Float>();
	/** Factoid scores for the targets. */
	private static ArrayList<Float> factoidTargetScores =
		new ArrayList<Float>();
	/** List scores for the targets. */
	private static ArrayList<Float> listTargetScores =
		new ArrayList<Float>();
	
	/**
	 * Calculates the score for a single factoid question.
	 * 
	 * @param qid ID of the question
	 * @param results the results from Ephyra
	 * @param absThresh absolute confidence threshold for results
	 * @return for each answer a flag that is true iff the answer is correct
	 */
	private static boolean[] evalFactoidQuestion(String qid, Result[] results,
												 float absThresh) {
		// get pattern if available
		TRECPattern pattern = null;
		for (TRECPattern factoidPattern : factoidPatterns)
			if (factoidPattern.getId().equals(qid)) {
				pattern = factoidPattern;
				break;
			}
		
		// drop result if its score does not satisfy the threshold
		if (results.length > 0 && results[0].getScore() < absThresh)
			results = new Result[0];
		
		// evaluate result with pattern
		if (results.length > 0 && pattern != null) {
			String firstAnswer = results[0].getAnswer();
			for (String regex : pattern.getRegexs())
				if (firstAnswer.matches(".*?" + regex + ".*+")) {
					// answer correct if it matches one of the patterns
					factoidQuestionScores.add(1f);
					return new boolean[] {true};
				}
			
			// answer wrong if it does not match one of the patterns
			factoidQuestionScores.add(0f);
			return new boolean[] {false};
		} else if (results.length == 0 && pattern == null) {
			// answer correct if neither result nor pattern available
			factoidQuestionScores.add(1f);
			return new boolean[] {true};
		} else {
			// answer wrong if either result or pattern available
			factoidQuestionScores.add(0f);
			return new boolean[] {false};
		}
	}
	
	/**
	 * Calculates the score for a single list question.
	 * 
	 * @param qid ID of the question
	 * @param results the results from Ephyra
	 * @param relThresh relative confidence threshold for results
	 * @return for each answer a flag that is true iff the answer is correct
	 */
	private static boolean[] evalListQuestion(String qid, Result[] results,
											  float relThresh) {
		// get pattern
		TRECPattern pattern = null;
		for (TRECPattern listPattern : listPatterns)
			if (listPattern.getId().equals(qid)) {
				pattern = listPattern;
				break;
			}
		if (pattern == null) return new boolean[0];  // pattern not available
		
		// get results with a score of at least relThresh * top score
		ArrayList<Result> resultList = new ArrayList<Result>();
		if (results.length > 0) {
			float topScore = results[0].getScore();
			for (Result result : results)
				if (result.getScore() >= relThresh * topScore)
					resultList.add(result);
		}
		
		float f = 0;  // F measure
		boolean[] correct = new boolean[resultList.size()];  // correct results
		if (resultList.size() > 0) {
			String[] regexs = pattern.getRegexs();
			int total = regexs.length;  // total number of known answers
			int returned = resultList.size();  // number of returned results
			int covered = 0;  // number of answers covered by the results
			for (String regex : regexs) {
				boolean found = false;
				for (int i = 0; i < resultList.size(); i++) {
					String answer = resultList.get(i).getAnswer();
					if (answer.matches(".*?" + regex + ".*+")) {
						if (!found) {
							covered++;
							found = true;
						}
						correct[i] = true;
					}
				}
			}
			
			if (covered > 0) {
				float recall = ((float) covered) / total;
				float precision = ((float) covered) / returned;
				f = (2 * recall * precision) / (recall + precision);
			}
		}
		
		listQuestionScores.add(f);
		return correct;
	}
	
	/**
	 * Calculates the factoid score for the current target.
	 * 
	 * @return the score or <code>-1</code> if there are no factoid questions
	 */
	private static float evalFactoidTarget() {
		// sum of factoid question scores
		float sum = 0;
		for (Float score : factoidQuestionScores) sum += score;
		// number of factoid questions
		int num = factoidQuestionScores.size();
		if (num == 0) return -1;
		// factoid score for the target
		float score = sum / num;
		
		// clear scores
		factoidQuestionScores = new ArrayList<Float>();
		
		factoidTargetScores.add(score);
		return score;
	}
	
	/**
	 * Calculates the list score for the current target.
	 * 
	 * @return the score or <code>-1</code> if there are no list questions
	 */
	private static float evalListTarget() {
		// sum of list question scores
		float sum = 0;
		for (Float score : listQuestionScores) sum += score;
		// number of list questions
		int num = listQuestionScores.size();
		if (num == 0) return -1;
		// list score for the target
		float score = sum / num;
		
		// clear scores
		listQuestionScores = new ArrayList<Float>();
		
		listTargetScores.add(score);
		return score;
	}
	
	/**
	 * Calculates the total score for the factoid component and logs the score.
	 * 
	 * @param absThresh absolute confidence threshold for results
	 * @return the score or <code>-1</code> if there are no factoid questions
	 */
	private static float evalFactoidTotal(float absThresh) {
		// sum of factoid target scores
		float sum = 0;
		for (Float score : factoidTargetScores) sum += score;
		// number of targets with factoid questions
		int num = factoidTargetScores.size();
		if (num == 0) return -1;
		// score for the factoid component
		float score = sum / num;
		
		// clear scores
		factoidTargetScores = new ArrayList<Float>();
		
		// log score for factoid component
		Logger.logFactoidScore(score, absThresh);
		
		return score;
	}
	
	/**
	 * Calculates the total score for the list component and logs the score.
	 * 
	 * @param relThresh relative confidence threshold for results
	 * @return the score or <code>-1</code> if there are no list questions
	 */
	private static float evalListTotal(float relThresh) {
		// sum of list target scores
		float sum = 0;
		for (Float score : listTargetScores) sum += score;
		// number of targets with list questions
		int num = listTargetScores.size();
		if (num == 0) return -1;
		// score for the list component
		float score = sum / num;
		
		// clear scores
		listTargetScores = new ArrayList<Float>();
		
		// log score for list component
		Logger.logListScore(score, relThresh);
		
		return score;
	}
	
	/**
	 * Initializes Ephyra, asks the questions or loads the answers from a log
	 * file, evaluates the answers if patterns are available and logs and saves
	 * the answers.
	 */
	private static void runAndEval() {
		// initialize Ephyra
		EphyraTREC13To16 ephyra = new EphyraTREC13To16();
		
		// evaluate for multiple thresholds
		boolean firstThreshold = true;
//		for (float fAbsThresh = FACTOID_ABS_THRESH;
//			 fAbsThresh <= 1; fAbsThresh += 0.01) {
		float fAbsThresh = FACTOID_ABS_THRESH;
//		for (float lRelThresh = LIST_REL_THRESH;
//			 lRelThresh <= 1; lRelThresh += 0.01) {
		float lRelThresh = LIST_REL_THRESH;
		
		for (TRECTarget target : targets) {
			MsgPrinter.printTarget(target.getTargetDesc());
			
			// normalize target description, determine target types
			if (firstThreshold) TargetPreprocessor.preprocess(target);
			
			String targetDesc = target.getTargetDesc();
			String condensedTarget = target.getCondensedTarget();
			TRECQuestion[] questions = target.getQuestions();
			
			// condensed target is used as contextual information
			QuestionAnalysis.setContext(condensedTarget);
			
			for (int i = 0; i < questions.length; i++) {
				MsgPrinter.printQuestion(questions[i].getQuestionString());
				
				String id = questions[i].getId();
				String type = questions[i].getType();
				String qs;
				if (type.equals("FACTOID") || type.equals("LIST")) {
					// resolve coreferences in factoid and list questions
					if (firstThreshold) {
						MsgPrinter.printResolvingCoreferences();
						CorefResolver.resolvePronounsToTarget(target, i);
					}
					qs = questions[i].getQuestionString();
				} else {
					qs = targetDesc;
				}
				
				// set pattern used to evaluate answers for overlap analysis
				OverlapAnalysisFilter.setPattern(null);
				if (type.equals("FACTOID")) {
					for (TRECPattern pattern : factoidPatterns) {
						if (pattern.getId().equals(id)) {
							OverlapAnalysisFilter.setPattern(pattern);
							break;
						}
					}
				}
				
				// ask Ephyra or load answer from log file
				Result[] results = null;
				if ((type.equals("FACTOID") && factoidLog) ||
						(type.equals("LIST") && listLog) ||
						(type.equals("OTHER") && otherLog)) {
					results =
						TREC13To16Parser.loadResults(qs, type, inputLogFile);
				}
				if (results == null) { // answer not loaded from log file
					if (type.equals("FACTOID")) {
						Logger.logFactoidStart(qs);
						results = ephyra.askFactoid(qs, FACTOID_MAX_ANSWERS,
													FACTOID_ABS_THRESH);
//						results = new Result[0];
						Logger.logResults(results);
						Logger.logFactoidEnd();
					} else if (type.equals("LIST")) {
						Logger.logListStart(qs);
						results = ephyra.askList(qs, LIST_REL_THRESH);
//						results = new Result[0];
						Logger.logResults(results);
						Logger.logListEnd();
					} else {
						Logger.logDefinitionalStart(qs);
						results = ephyra.askOther(target);
//						results = new Result[0];
						Logger.logResults(results);
						Logger.logDefinitionalEnd();
					}
				}
				
				// calculate question score if patterns are available
				boolean[] correct = null;
				if (type.equals("FACTOID") && factoidPatterns != null)
					correct = evalFactoidQuestion(id, results, fAbsThresh);
				else if (type.equals("LIST") && listPatterns != null)
					correct = evalListQuestion(id, results, lRelThresh);
				
				// update target data structure
				TRECAnswer[] answers = new TRECAnswer[results.length];
				for (int j = 0; j < results.length; j++) {
					String answer = results[j].getAnswer();
					String supportDoc = results[j].getDocID();
					answers[j] = new TRECAnswer(id,	answer, supportDoc);
				}
				questions[i].setAnswers(answers);
				if (results.length > 0) {
					QuestionInterpretation qi =
						results[0].getQuery().getInterpretation();
					if (qi != null)	questions[i].setInterpretation(qi);
				}
				
				if (answers.length == 0) {  // no answer found
					answers = new TRECAnswer[1];
					if (type.equals("FACTOID"))
						answers[0] = new TRECAnswer(id, null, "NIL");
					else
						answers[0] = new TRECAnswer(id, "No answers found.",
													"XIE19960101.0001");
				}
				
				// save answers to output file
				TREC13To16Parser.saveAnswers("log/" + runTag, answers, correct,
											 runTag);
			}
			
			// calculate target scores if patterns are available
			if (factoidPatterns != null) evalFactoidTarget();
			if (listPatterns != null) evalListTarget();
		}
		
		// calculate component scores and log scores if patterns are available
		if (factoidPatterns != null) evalFactoidTotal(fAbsThresh);
		if (listPatterns != null) evalListTotal(lRelThresh);
		
		firstThreshold = false;
//		}
//		}
	}
	
	/**
	 * Runs Ephyra on the TREC questions.
	 * 
	 * @param args argument 1: questionfile<br>
	 * 			   [argument 2: tag=runtag (uniquely identifies the run, also
	 * 							used as output file name, if not set an
	 * 							unambiguous tag is generated automatically)]<br>
	 * 			   [argument 3: log=logfile (if not set an unambiguous file name
	 * 							is generated automatically)]<br>
	 * 			   [argument 4: lin=input_logfile (specifies a separate logfile
	 * 							that is used as a source for answers to some of
	 * 							the	questions, if not set the standard log file
	 * 							is used)]<br>
	 * 			   [argument 5: lflags=[f][l][o] (answers to these types of
	 * 							questions are loaded from the log file instead
	 * 							of querying Ephyra,	e.g. "flo" for factoid, list
	 * 							and other questions)]<br>
	 * 			   [argument 6: fp=factoid_patternfile]<br>
	 * 			   [argument 7: lp=list_patternfile]
	 */
	public static void main(String[] args) {
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		if (args.length < 1) {
			MsgPrinter.printUsage("java EphyraTREC13To16 questionfile " +
								  "[tag=runtag] [log=logfile] " +
								  "[lin=input_logfile] " +
								  "[lflags=[f][l][o]] " +
								  "[fp=factoid_patternfile] " +
								  "[lp=list_patternfile]");
			System.exit(1);
		}
		
		// load targets
		targets = TREC13To16Parser.loadTargets(args[0]);
		
		for (int i = 1; i < args.length; i++)
			if (args[i].matches("tag=.*")) {
				// set run tag
				runTag = args[i].substring(4);
			} else if (args[i].matches("log=.*")) {
				// set log file
				logFile = args[i].substring(4);
			} else if (args[i].matches("lin=.*")) {
				// set separate input log file
				inputLogFile = args[i].substring(4);
			} else if (args[i].matches("lflags=.*")) {
				// answers for some question types are loaded from log file
				String flags = args[i].substring(7).toLowerCase();
				if (flags.contains("f")) factoidLog = true;
				if (flags.contains("l")) listLog = true;
				if (flags.contains("o")) otherLog = true;
			} else if (args[i].matches("fp=.*")) {
				// load factoid patterns
				factoidPatterns =
					TREC13To16Parser.loadPatterns(args[i].substring(3));
			} else if (args[i].matches("lp=.*")) {
				// load list patterns
				listPatterns =
					TREC13To16Parser.loadPatterns(args[i].substring(3));
			}
		
		// if run tag or log file not set, generate unambiguous names
		if (runTag == null || logFile == null) {
			String n = "";
			Matcher m = Pattern.compile("\\d++").matcher(args[0]);
			if (m.find()) n = m.group(0);
			String date = "";
			Calendar c = new GregorianCalendar();
			date += c.get(Calendar.DAY_OF_MONTH);
			if (date.length() == 1) date = "0" + date;
			date = (c.get(Calendar.MONTH) + 1) + date;
			if (date.length() == 3) date = "0" + date;
			date = c.get(Calendar.YEAR) + date;
			
			if (runTag == null) runTag = "TREC" + n + "_" + date + "_out";
			if (logFile == null) logFile = "log/TREC" + n + "_" + date;
		}
		
		// if input log file not set, use standard log file
		if (inputLogFile == null) inputLogFile = logFile;
		
		// enable logging
		Logger.setLogfile(logFile);
		Logger.enableLogging(true);
		
		// run Ephyra on questions, evaluate answers if patterns available
		runAndEval();
	}
	
	// Layout 1
	/**
	 * Initializes the pipeline for 'other' questions.
	 */
	protected void initOther() {
		// query generation
		QueryGeneration.clearQueryGenerators();
		
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
		
		//	initialize scores
		AnswerSelection.addFilter(new ScoreResetterFilter());
		
		//	extract sentences from snippets
		AnswerSelection.addFilter(new SentenceExtractionFilter());
		
		//	cut meaningless introductions from sentences
		AnswerSelection.addFilter(new CutKeywordsFilter());
		AnswerSelection.addFilter(new CutStatementProviderFilter());
		AnswerSelection.addFilter(new SentenceSplitterFilter());
		AnswerSelection.addFilter(new CutKeywordsFilter());
		
		//	remove duplicates
		AnswerSelection.addFilter(new DuplicateSnippetFilter());
		
		//	throw out enumerations of proper names
		AnswerSelection.addFilter(new ProperNameFilter());
		
		//	throw out direct speech snippets, rarely contain useful information
		AnswerSelection.addFilter(new DirectSpeechFilter());
		
		//	sort out snippets containing no new terms
		AnswerSelection.addFilter(new TermFilter());
		
		AnswerSelection.addFilter(
				new WikipediaGoogleTermImportanceFilter(
					WebTermImportanceFilter.LOG_LENGTH_NORMALIZATION,
					WebTermImportanceFilter.LOG_LENGTH_NORMALIZATION,
					false
				)
			);
		AnswerSelection.addFilter(new ScoreSorterFilter());
		
		//	cut off result
		AnswerSelection.addFilter(new ResultLengthFilter(3000));
	}
	
	// Layout 2
//	/**
//	 * Initializes the pipeline for 'other' questions.
//	 */
//	protected void initOther() {
//		// query generation
//		QueryGeneration.clearQueryGenerators();
//		
//		// search
//		// - knowledge miners for unstructured knowledge sources
//		Search.clearKnowledgeMiners();
//		for (String[] indriIndices : IndriKM.getIndriIndices())
//			Search.addKnowledgeMiner(new IndriKM(indriIndices, false));
//		for (String[] indriServers : IndriKM.getIndriServers())
//			Search.addKnowledgeMiner(new IndriKM(indriServers, true));
//		// - knowledge annotators for (semi-)structured knowledge sources
//		Search.clearKnowledgeAnnotators();
//		
//		// answer extraction and selection
//		// (the filters are applied in this order)
//		AnswerSelection.clearFilters();
//		
//		//	initialize scores
//		AnswerSelection.addFilter(new ScoreResetterFilter());
//		
//		//	extract sentences from snippets
//		AnswerSelection.addFilter(new SentenceExtractionFilter());
//		
//		//	cut meaningless introductions from sentences
//		AnswerSelection.addFilter(new CutKeywordsFilter());
//		AnswerSelection.addFilter(new CutStatementProviderFilter());
//		AnswerSelection.addFilter(new SentenceSplitterFilter());
//		AnswerSelection.addFilter(new CutKeywordsFilter());
//		
//		//	remove duplicates
//		AnswerSelection.addFilter(new DuplicateSnippetFilter());
//		
//		//	throw out enumerations of proper names
//		AnswerSelection.addFilter(new ProperNameFilter());
//		
//		//	throw out direct speech snippets, rarely contain useful information
//		AnswerSelection.addFilter(new DirectSpeechFilter());
//		
//		AnswerSelection.addFilter(
//				new WikipediaGoogleWebTermImportanceFilter(
//					WebTermImportanceFilter.LOG_LENGTH_NORMALIZATION,
//					WebTermImportanceFilter.LOG_LENGTH_NORMALIZATION,
//					false
//				)
//			);
//		AnswerSelection.addFilter(new ScoreSorterFilter());
//		
//		//	cut off result
//		AnswerSelection.addFilter(new ResultLengthFilter(3000));
//	}
	
	// Layout 3
//	/**
//	 * Initializes the pipeline for 'other' questions.
//	 */
//	protected void initOther() {
//		// query generation
//		QueryGeneration.clearQueryGenerators();
//		
//		// search
//		// - knowledge miners for unstructured knowledge sources
//		Search.clearKnowledgeMiners();
//		for (String[] indriIndices : IndriKM.getIndriIndices())
//			Search.addKnowledgeMiner(new IndriDocumentKM(indriIndices, false));
//		for (String[] indriServers : IndriKM.getIndriServers())
//			Search.addKnowledgeMiner(new IndriDocumentKM(indriServers, true));
//		// - knowledge annotators for (semi-)structured knowledge sources
//		Search.clearKnowledgeAnnotators();
//		
//		// answer extraction and selection
//		// (the filters are applied in this order)
//		AnswerSelection.clearFilters();
//		
//		//	initialize scores
//		AnswerSelection.addFilter(new ScoreResetterFilter());
//		
//		//	extract sentences from snippets
//		AnswerSelection.addFilter(new SentenceExtractionFilter());
//		
//		//	cut meaningless introductions from sentences
//		AnswerSelection.addFilter(new CutKeywordsFilter());
//		AnswerSelection.addFilter(new CutStatementProviderFilter());
//		AnswerSelection.addFilter(new SentenceSplitterFilter());
//		AnswerSelection.addFilter(new CutKeywordsFilter());
//		
//		//	remove duplicates
//		AnswerSelection.addFilter(new DuplicateSnippetFilter());
//		
//		//	throw out enumerations of proper names
//		AnswerSelection.addFilter(new ProperNameFilter());
//		
//		//	throw out direct speech snippets, rarely contain useful information
//		AnswerSelection.addFilter(new DirectSpeechFilter());
//		
//		//	sort out snippets containing no new terms
//		AnswerSelection.addFilter(new TermFilter());
//		
//		AnswerSelection.addFilter(
//				new WikipediaGoogleWebTermImportanceFilter(
//					WebTermImportanceFilter.LOG_LENGTH_NORMALIZATION,
//					WebTermImportanceFilter.LOG_LENGTH_NORMALIZATION,
//					false
//				)
//			);
//		AnswerSelection.addFilter(new ScoreSorterFilter());
//		
//		//	cut off result
//		AnswerSelection.addFilter(new ResultLengthFilter(3000));
//	}
	
	/**
	 * Asks Ephyra an 'other' question.
	 * 
	 * @param question other question
	 * @return array of results
	 */
	public final Result[] askOther(String question) {
		// initialize pipeline
		initOther();
		
		// query generation
		MsgPrinter.printGeneratingQueries();
		String qn = QuestionNormalizer.normalize(question);
		MsgPrinter.printNormalization(qn);  // print normalized question string
		Logger.logNormalization(qn);  // log normalized question string
		String[] kws = KeywordExtractor.getKeywords(qn);
		AnalyzedQuestion aq = new AnalyzedQuestion(question);
		aq.setKeywords(kws);
		aq.setFactoid(false);
		BagOfWordsG gen = new BagOfWordsG();
		
		Query[] queries = gen.generateQueries(aq);
		for (int q = 0; q < queries.length; q++)
			queries[q].setOriginalQueryString(question);
		
		MsgPrinter.printQueryStrings(queries);  // print query strings
		Logger.logQueryStrings(queries);  // log query strings
		
		// search
		MsgPrinter.printSearching();
		Result[] results = Search.doSearch(queries);
		
		// answer selection
		MsgPrinter.printSelectingAnswers();
		results = AnswerSelection.getResults(results, Integer.MAX_VALUE, 0);
		
		return results;
	}
	
	/**
	 * Asks Ephyra an 'other' question, making use of the target description and
	 * previous questions and answers.
	 * 
	 * @param target the target the 'other' question is about
	 * @return array of results
	 */
	public Result[] askOther(TRECTarget target) {
		//	get target type from interpretations of factoid/list questions
		TRECQuestion[] factoidQuestions = target.getQuestions();
		
		ArrayList<String> props = new ArrayList<String>();
		ArrayList<String> vals = new ArrayList<String>();
		
		ArrayList<String> sentences = new ArrayList<String>();
		String[] targetTokens = NETagger.tokenize(target.getTargetDesc());
		for (String tt : targetTokens) sentences.add(tt);
		
		//	collect properties and answers from FACTOID and LIST questions
		for (TRECQuestion fq : factoidQuestions) {
			QuestionInterpretation qi = fq.getInterpretation();
			if (qi != null) {
				String prop = qi.getProperty();
				TRECAnswer[] answers = fq.getAnswers();
				if (answers.length != 0) {
					
					//	collect property/value pair
					String val = answers[0].getAnswerString();
					props.add(prop);
					vals.add(val);
//					MsgPrinter.printStatusMsg("Dossier on '" + target.getTargetDesc() + "' contains: '" + prop + "' is '" + val + "'");
					
					//	remember answer sentence for previous results
					String[] questionTokens = NETagger.tokenize(fq.getQuestionString());
					for (String qt : questionTokens) sentences.add(qt);
				}
			}
		}
		
		//	filter out results that bring no new terms but ones contained in the target, a previous question, or an answert to a previous question
		TermFilter.setPreviousResultsTerms(sentences.toArray(new String[sentences.size()]));
		
		//	initialize Dossier
//		Dossier dossier = Dossier.getDossier(target.getTargetDesc(), target.getTargetType(), props.toArray(new String[props.size()]), vals.toArray(new String[vals.size()]));
		Dossier dossier = Dossier.getDossier(target.getTargetDesc(), null, props.toArray(new String[props.size()]), vals.toArray(new String[vals.size()]));
//		MsgPrinter.printStatusMsg("Target type of '" + target.getTargetDesc() + "' is " + dossier.getTargetType());
		ArrayList<Result> rawResults = new ArrayList<Result>();
		
		//	collect missing properties
		String[] missingProps = dossier.getMissingPropertyNames();
		for (String mp : missingProps) {
			
			//	generate FACTOID question from template
			String question = QuestionInterpreter.getQuestion(target.getTargetDesc(), mp);
			
			//	if valid template exists, ask FACTOID question
			if (question != null) {
//				MsgPrinter.printStatusMsg("Building Dossier on '" + target.getTargetDesc() + "', would ask this question now: '" + question + "'");
//				Logger.enableLogging(false);
//				Result res = this.askFactoid(question);
//				Logger.enableLogging(true);
//				
//				//	if question could be answered, add new property and value to dossier
//				if (res != null) {
//					dossier.setProperty(mp, res.getAnswer());
//					MsgPrinter.printStatusMsg("Dossier on '" + target.getTargetDesc() + "' extended: '" + mp + "' set to '" + res.getAnswer() + "'");
//					rawResults.add(res);
//					String sentence = res.getSentence();
//					
//					//	get supporting sentence of answer and, if existing, remember it as nugget
//					if (sentence != null) {
//						Result newRes = new Result(sentence, res.getQuery(), res.getDocID(), res.getHitPos());
//						newRes.setScore(res.getScore() + 2);
//						rawResults.add(newRes);
//					}
//				}
			}
		}
		
		NuggetEvaluationFilter.setTargetID(target.getId());
		
		//	collect BagOfWords results for target
		Result[] nuggets = askOther(target.getTargetDesc());
		for (Result r : nuggets) rawResults.add(r);
		nuggets = rawResults.toArray(new Result[rawResults.size()]);
		
		NuggetEvaluationFilter.targetFinished();
		
		//	reset term filter
		TermFilter.setPreviousResultsTerms(null);
		NuggetEvaluationFilter.setTargetID(null);
		
		return nuggets;
	}
}

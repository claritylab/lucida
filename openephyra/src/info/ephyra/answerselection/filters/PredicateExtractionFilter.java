package info.ephyra.answerselection.filters;

import info.ephyra.nlp.LingPipe;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.VerbFormConverter;
import info.ephyra.nlp.semantics.ASSERT;
import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.nlp.semantics.ontologies.WordNet;
import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.QuestionAnalysis;
import info.ephyra.questionanalysis.Term;
import info.ephyra.questionanalysis.TermExtractor;
import info.ephyra.search.Result;
import info.ephyra.util.Dictionary;
import info.ephyra.util.StringUtils;

import java.text.ParseException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Map;
import java.util.Set;

/**
 * <p>Extracts predicates which are similar to those in the question from
 * documents.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-17
 */
public class PredicateExtractionFilter extends Filter {
	/**
	 * Maximum number of sentences that are annotated with predicate-argument
	 * structures.
	 */
	private static final int MAX_SENTENCES = 300;
	
	/**
	 * Maximum length of a sentence in characters to be considered for predicate
	 * extraction.
	 */
	private static final int MAX_SENT_LENGTH_CHARS = 300;
	
	/**
	 * Maximum length of a sentence in tokens to be considered for predicate
	 * extraction.
	 */
	private static final int MAX_SENT_LENGTH_TOKENS = 50;
	
	/**
	 * Extracts NEs of the expected answer types from a sentence.
	 * 
	 * @param sentence the sentence
	 * @param answerTypes the expected answer types
	 * @return NEs and their types
	 */
	private Map<String, String[]> extractNes(String sentence,
			String[] answerTypes) {
		// maps NEs to their types
		Map<String, String[]> extracted = new Hashtable<String, String[]>();
		
		// tokenize sentence
		String[][] tokens = new String[1][];
		tokens[0] = NETagger.tokenize(sentence);
		
		for (String answerType : answerTypes) {
			// get IDs of the taggers for the most specific NE type that can be
			// tagged
			String[] neTypes = answerType.split("->");
			int neIds[] = new int[0];
			String neType = "";
			for (String thisType : neTypes) {
				int[] thisIds = NETagger.getNeIds(thisType);
				if (thisIds.length > 0) {
					neIds = thisIds;
					neType = thisType;
				}
			}
			
			// extract NEs of that type
			for (int neId : neIds) {
				String[][] nes = NETagger.extractNes(tokens, neId);
				
				// untokenize NEs
				for (int i = 0; i < nes[0].length; i++)
					nes[0][i] = OpenNLP.untokenize(nes[0][i], sentence);
				
				for (String ne : nes[0]) {
					String[] types = extracted.get(ne);
					
					// check if NE type has already been added
					if (types != null) {
						boolean exists = false;
						for (String existing : types)
							if (neType.equals(existing)) {
								exists = true;
								break;
							}
						if (exists) continue;
					}
					
					// add new NE type
					String[] newTypes;
					if (types == null) {
						newTypes = new String[1];
					} else {
						newTypes = new String[types.length + 1];
						for (int i = 0; i < types.length; i++)
							newTypes[i] = types[i];
					}
					newTypes[newTypes.length - 1] = neType;
					extracted.put(ne, newTypes);
				}
			}
		}
		
		return extracted;
	}
	
	/**
	 * Gets all forms of the verbs and expansions of predicates with missing
	 * arguments. The verb forms are associated with their weights.
	 * 
	 * @param ps predicates
	 * @return verb forms and their weights
	 */
	private Hashtable<String[], Double> getAllVerbForms(Predicate[] ps) {
		Hashtable<String[], Double> allVerbForms =
			new Hashtable<String[], Double>();
		
		for (Predicate p : ps) {
			// get verbs from predicates with missing arguments only
			if (!p.hasMissingArgs()) continue;
			
			// get predicate verb and expansions
			Term verbTerm = p.getVerbTerm();
			String verb = verbTerm.getText();
			Map<String, Double> expansionsMap = verbTerm.getExpansions();
			Set<String> expansions = expansionsMap.keySet();
			
			// get all verb forms
			String infinitive = WordNet.getLemma(verb, WordNet.VERB);
			if (infinitive == null) infinitive = verb;
			String[] verbForms = VerbFormConverter.getAllForms(infinitive);
			allVerbForms.put(verbForms, 1d);
			for (String expansion : expansions) {
				infinitive = WordNet.getLemma(expansion, WordNet.VERB);
				if (infinitive == null) infinitive = expansion;
				verbForms = VerbFormConverter.getAllForms(infinitive);
				allVerbForms.put(verbForms, expansionsMap.get(expansion));
			}
		}
		
		return allVerbForms;
	}
	
	/**
	 * Decides if predicates should be extracted from this sentence. If the
	 * sentence passes the tests, NEs of the expected answer types and terms
	 * are extracted and added to the result.
	 * 
	 * @param sentence sentence-level result
	 * @return <code>true</code> iff the sentence is relevant
	 */
	private boolean checkSentence(Result sentence) {
		AnalyzedQuestion aq = sentence.getQuery().getAnalyzedQuestion();
		String s = sentence.getAnswer();
		
		// check the length of the sentence against thresholds
		if (s.length() > MAX_SENT_LENGTH_CHARS) return false;
		String[] tokens = NETagger.tokenize(s);
		if (tokens.length > MAX_SENT_LENGTH_TOKENS) return false;
		
//		// check if the sentence contains a matching verb term
//		boolean match = false;
//		Predicate[] questionPs = aq.getPredicates();
//		String[] tokens = OpenNLP.tokenize(s);
//		String[] pos = OpenNLP.tagPos(tokens);
//		for (int i = 0; i < tokens.length; i++) {
//			// look for verbs only
//			if (!pos[i].startsWith("VB") || !pos[i].matches("[a-zA-Z]*"))
//				continue;
//			Term sentenceTerm = new Term(tokens[i], pos[i]);
//			
//			for (Predicate questionP : questionPs) {
//				// compare to predicates with missing arguments only
//				if (!questionP.hasMissingArgs()) continue;
//				Term predicateTerm = questionP.getVerbTerm();
//				
//				if (predicateTerm.simScore(sentenceTerm.getLemma()) > 0) {
//					match = true;
//					break;
//				}
//			}
//			
//			if (match) break;
//		}
//		if (!match) return false;
//		-> checked in apply() (performance optimized)
		
		// check if the sentence contains NEs of the expected types
		String[] answerTypes = aq.getAnswerTypes();
		if (answerTypes.length != 0) {  // answer type known
			boolean newNE = false;
			Map<String, String[]> extracted = extractNes(s, answerTypes);
			String questionNorm = StringUtils.normalize(aq.getQuestion());
			for (String ne : extracted.keySet()) {
				String neNorm = StringUtils.normalize(ne);
				if (!StringUtils.isSubsetKeywords(neNorm, questionNorm)) {
					newNE = true;
					break;
				}
			}
			if (!newNE) return false;  // no NEs that are not in the question
			sentence.setNes(extracted);
		}
		
		// check if the sentence contains a matching argument term
		// - single-token terms are extracted first to avoid dictionary lookups
		boolean match = false;
		Term[] singleTerms = TermExtractor.getSingleTokenTerms(s);
		Predicate[] questionPs = aq.getPredicates();
		for (Term singleTerm : singleTerms) {
			for (Predicate questionP : questionPs) {
				// compare to predicates with missing arguments only
				if (!questionP.hasMissingArgs()) continue;
				
				Term[] predicateTerms = questionP.getArgTerms();
				for (Term predicateTerm : predicateTerms)
					if (predicateTerm.simScore(singleTerm.getLemma()) > 0) {
						match = true;
						break;
					}
				
				if (match) break;
			}
			
			if (match) break;
		}
		if (!match) return false;
		// - multi-token terms are extracted from sentences that pass the test
		Dictionary[] dicts = QuestionAnalysis.getDictionaries();
		Term[] multiTerms = TermExtractor.getTerms(s, dicts);
		sentence.setTerms(multiTerms);
		
		return true;
	}
	
	/**
	 * Extracts relevant predicates from documents.
	 * 
	 * @param results array of <code>Result</code> objects containing documents
	 * @return array of <code>Result</code> objects containing predicates
	 */
	public Result[] apply(Result[] results) {
		if (results.length == 0) return results;
		
		ArrayList<Result> allResults = new ArrayList<Result>();
		
		// extract relevant sentences
		// - get sentences that contain relevant verbs,
		//   use weights of verbs as confidence scores
		HashSet<Result> ssSet = new HashSet<Result>();
		for (Result result : results) {
			// only apply this filter to results for the semantic parsing
			// approach
			Query query = result.getQuery();
			Predicate[] ps = query.getAnalyzedQuestion().getPredicates();
			if (!query.extractWith(FactoidsFromPredicatesFilter.ID) ||
					ps.length == 0 ||
					result.getScore() != 0) {
				allResults.add(result);
				continue;
			}
			
			// get all verb forms and build patterns
			Hashtable<String[], Double> verbFormsMap = getAllVerbForms(ps);
			ArrayList<String> verbPatterns = new ArrayList<String>();
			ArrayList<Double> verbWeights = new ArrayList<Double>();
			for (String[] verbForms : verbFormsMap.keySet()) {
				String verbPattern = "(?i).*?\\b(" +
						StringUtils.concat(verbForms, "|") + ")\\b.*+";
				verbPatterns.add(verbPattern);
				verbWeights.add(verbFormsMap.get(verbForms));
			}
			
			String[] paragraphs = result.getAnswer().split("\\n");
			for (String p : paragraphs) {
				// paragraph does not contain relevant verb?
				boolean contains = false;
				for (String verbPattern : verbPatterns) {
					if (p.matches(verbPattern)) {
						contains = true;
						break;
					}
				}
				if (!contains) continue;
				
				String[] sentences = LingPipe.sentDetect(p);
				for (String s : sentences) {
					// sentence does not contain relevant verb?
					Double weight = 0d;
					for (int i = 0; i < verbPatterns.size(); i++) {
						if (s.matches(verbPatterns.get(i))) {
							weight = verbWeights.get(i);
							break;
						}
					}
					if (weight == 0d) continue;
					
					// replace whitespaces by single blanks and trim
					s = s.replaceAll("\\s++", " ").trim();
					
					// create sentence-level result object
					Result sentence = result.getCopy();
					sentence.setAnswer(s);
					sentence.setScore(weight.floatValue());
					
					ssSet.add(sentence);
				}
			}
		}
		// - check if these sentences are relevant,
		//   get MAX_SENTENCES sentences with most relevant verbs
		Result[] ss = ssSet.toArray(new Result[ssSet.size()]);
		ss = (new ScoreSorterFilter()).apply(ss);
		ArrayList<Result> ssList = new ArrayList<Result>();
		for (Result s : ss) {
			s.setScore(0);
			if (checkSentence(s)) ssList.add(s);
			
			// get at most MAX_SENTENCES sentences
			if (ssList.size() >= MAX_SENTENCES) break;
		}
		ss = ssList.toArray(new Result[ssList.size()]);
		if (ss.length == 0)
			return allResults.toArray(new Result[allResults.size()]);
		
		// annotate predicates in sentences
		String[] sentences = new String[ss.length];
		for (int i = 0; i < ss.length; i++) sentences[i] = ss[i].getAnswer();
		String[][] ass = ASSERT.annotatePredicates(sentences);
		
		// extract predicates from annotations
		for (int i = 0; i < ass.length; i++) {
			Term[] terms = ss[i].getTerms();
			Predicate[] questionPs =
				ss[i].getQuery().getAnalyzedQuestion().getPredicates();
			
			for (int j = 0; j < ass[i].length; j++) {
				// build predicate
				Predicate predicate = null;
				try {
					predicate = new Predicate(sentences[i], ass[i][j], terms);
				} catch (ParseException e) {
//					MsgPrinter.printErrorMsg(e.getMessage());
//					System.exit(1);
					continue;
				}
				
				// calculate similarity score
				double simScore = 0;
				Predicate simPredicate = null;
				for (Predicate questionP : questionPs)
					// compare to predicates with missing arguments only
					if (questionP.hasMissingArgs()) {
						double currSimScore = predicate.simScore(questionP);
						
						if (currSimScore > simScore) {
							simScore = currSimScore;
							simPredicate = questionP;
						}
					}
				
				// keep predicate if it is similar to a question predicate
				if (simScore > 0) {
					predicate.setSimScore(simScore);
					predicate.setSimPredicate(simPredicate);
					Result result = ss[i].getCopy();
					result.setAnswer(ass[i][j]);
					result.setSentence(sentences[i]);
					result.setPredicate(predicate);
					
					allResults.add(result);
				}
			}
		}
		
		return allResults.toArray(new Result[allResults.size()]);
	}
}

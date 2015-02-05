package info.ephyra.querygeneration.generators;

import info.ephyra.answerselection.filters.AnswerPatternFilter;
import info.ephyra.answerselection.filters.AnswerTypeFilter;
import info.ephyra.answerselection.filters.FactoidsFromPredicatesFilter;
import info.ephyra.querygeneration.Query;
import info.ephyra.querygeneration.QuestionReformulator;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.util.FileUtils;
import info.ephyra.util.StringUtils;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

/**
 * <p>The <code>QuestionReformulationG</code> can be applied to a question to
 * obtain alternative queries that are more specific than a "bag of words" and
 * therefore are more likely to return good results.</p>
 * 
 * <p>This class extends the class <code>QueryGenerator</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-11
 */
public class QuestionReformulationG extends QueryGenerator {
	/**
	 * Question reformulators that are applied to the question to obtain
	 * additional, more specific queries.
	 */
	private static QuestionReformulator[] reformulators;
	/** Answer extraction techniques for this query type. */
	private static final String[] EXTRACTION_TECHNIQUES = {
		AnswerTypeFilter.ID,
		AnswerPatternFilter.ID,
		FactoidsFromPredicatesFilter.ID
	};
	
	/**
	 * Generates queries that are reformulations of the question string.
	 * 
	 * @param aq analyzed question
	 * @return <code>Query</code> objects
	 */
	public Query[] generateQueries(AnalyzedQuestion aq) {
		// only generate queries if the answer type is known or the question is
		// not a factoid question
		String[] ats = aq.getAnswerTypes();
		if (ats.length == 0 && aq.isFactoid()) return new Query[0];
		
		ArrayList<Query> results = new ArrayList<Query>();
		
		// create question reformulations
		String verbMod = aq.getVerbMod();
		String[] kws = aq.getKeywords();
		if (reformulators != null) {  // reformulators loaded
			Query[] queries;
			for (QuestionReformulator reformulator : reformulators) {
				queries = reformulator.apply(verbMod);
				
				if (queries != null)
					for (Query query : queries) {
						// include context keywords in the query string
						String queryString = query.getQueryString();
						for (String kw : kws)
							if (!StringUtils.equalsCommonNorm(queryString, kw))
								queryString += " " + kw;
						query.setQueryString(queryString);
						
						query.setAnalyzedQuestion(aq);
						query.setExtractionTechniques(EXTRACTION_TECHNIQUES);
						
						results.add(query);
					}
			}
		}
		
		return results.toArray(new Query[results.size()]);
	}
	
	/**
	 * Loads the reformulation rules from text files in the given folder.
	 * 
	 * @param dir folder that contains the reformulation rules
	 * @return true, iff the reformulation rules were loaded successfully
	 */
	public static boolean loadReformulators(String dir) {
		File[] files = FileUtils.getFiles(dir);
		reformulators = new QuestionReformulator[files.length];
		
		try {
			for (int i = 0; i < files.length; i++)
				reformulators[i] =
					new QuestionReformulator(files[i].getPath());
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
}

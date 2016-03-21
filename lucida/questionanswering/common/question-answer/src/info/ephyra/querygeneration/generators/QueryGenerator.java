package info.ephyra.querygeneration.generators;

import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.AnalyzedQuestion;

/**
 * <p>A <code>QueryGenerator</code> creates one or more <code>Query</code>
 * objects from an analyzed question.</p>
 * 
 * <p>This class is abstract.</p>
 * 
 * @author Nico Schlaefer
 * @version 2006-10-30
 */
public abstract class QueryGenerator {
	/**
	 * Generates one or more <code>Query</code> objects from an analyzed
	 * question.
	 * 
	 * @param aq analyzed question
	 * @return <code>Query</code> objects
	 */
	public abstract Query[] generateQueries(AnalyzedQuestion aq);
}

package info.ephyra.querygeneration;

import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.QuestionInterpretation;

import java.io.Serializable;

/**
 * <p>A <code>Query</code> is a data structure representing a search engine
 * query.</p>
 * 
 * <p>The required fields are a query string, the analyzed question, a score
 * that is the higher the more specific the query and the extraction techniques
 * applied to results retrieved with this query.</p>
 * 
 * <p>This class implements the interface <code>Serializable</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-05-01
 */
public class Query implements Serializable {
	/** Version number used during deserialization. */
	private static final long serialVersionUID = 20070501;
	
	/** The query string. */
	private String queryString;
	
	/** the original query String before normalization*/
	private String originalQueryString;
	
	/** The analyzed question. */
	private AnalyzedQuestion analyzedQuestion;
	/**
	 * The score of the query. More specific queries receive a higher score than
	 * simple keyword queries. The score is used by the answer selection module
	 * to score the results retrieved with this query.
	 */
	private float score;
	/**
	 * The answer extraction techniques applied to results retrieved with this
	 * query.
	 */
	private String[] extractionTechniques;
	/**
	 * The interpretation of the question used to generate this query
	 * (optional).
	 */
	private QuestionInterpretation qi;
	
	/**
	 * Creates a new <code>Query</code> object and sets the query string.
	 * 
	 * @param queryString query string
	 */
	public Query(String queryString) {
		this.queryString = queryString;
	}
	
	/**
	 * Creates a new <code>Query</code> object and sets the query string, the
	 * analyzed question and the score of the query.
	 * 
	 * @param queryString query string
	 * @param analyzedQuestion analyzed question
	 * @param score score of the query
	 */
	public Query(String queryString, AnalyzedQuestion analyzedQuestion,
			float score) {
		this(queryString);
		
		this.analyzedQuestion = analyzedQuestion;
		this.score = score;
	}
	
	/**
	 * Returns the query string.
	 * 
	 * @return query string
	 */
	public String getQueryString() {
		return queryString;
	}
	
	/**
	 * @return the original query String, before normalization
	 */
	public String getOriginalQueryString() {
		return originalQueryString;
	}
	
	/**
	 * @param originalQueryString the original query String
	 */
	public void setOriginalQueryString(String originalQueryString) {
		this.originalQueryString = originalQueryString;
	}
	
	/**
	 * Returns the analyzed question
	 * 
	 * @return analyzed question
	 */
	public AnalyzedQuestion getAnalyzedQuestion() {
		return analyzedQuestion;
	}
	
	/**
	 * Returns the score of the query.
	 * 
	 * @return score of the query
	 */
	public float getScore() {
		return score;
	}
	
	/**
	 * Checks if the given answer extraction technique is applied to results
	 * retrieved with this query.
	 * 
	 * @param technique answer extraction technique
	 * @return <code>true</code> iff the technique is used
	 */
	public boolean extractWith(String technique) {
		if (extractionTechniques == null) return false;
		
		for (String extractionTechnique : extractionTechniques)
			if (technique.equals(extractionTechnique)) return true;
		
		return false;
	}
	
	/**
	 * Returns the interpretation of the question used to generate this query.
	 * 
	 * @return question interpretation
	 */
	public QuestionInterpretation getInterpretation() {
		return qi;
	}
	
	/**
	 * Sets the query string.
	 * 
	 * @param queryString query string
	 */
	public void setQueryString(String queryString) {
		this.queryString = queryString;
	}
	
	/**
	 * Sets the analyzed question.
	 * 
	 * @param analyzedQuestion analyzed question
	 */
	public void setAnalyzedQuestion(AnalyzedQuestion analyzedQuestion) {
		this.analyzedQuestion = analyzedQuestion;
	}
	
	/**
	 * Sets the score of the query.
	 * 
	 * @param score score of the query
	 */
	public void setScore(float score) {
		this.score = score;
	}
	
	/**
	 * Sets the answer extraction techniques that are applied to results
	 * retrieved with this query.
	 * 
	 * @param techniques answer extraction techniques
	 */
	public void setExtractionTechniques(String[] techniques) {
		extractionTechniques = techniques;
	}
	
	/**
	 * Adds an answer extraction technique that is applied to results retrieved
	 * with this query.
	 * 
	 * @param technique answer extraction technique
	 * @return <code>true</code> iff a new technique was added
	 */
	public boolean addExtractionTechnique(String technique) {
		// check if the extraction technique has already been added
		if (extractionTechniques != null)
			for (String existing : extractionTechniques)
				if (technique.equals(existing)) return false;
		
		// add new extraction technique
		String[] newTechniques;
		if (extractionTechniques == null) {
			newTechniques = new String[1];
		} else {
			newTechniques = new String[extractionTechniques.length + 1];
			for (int i = 0; i < extractionTechniques.length; i++)
				newTechniques[i] = extractionTechniques[i];
		}
		newTechniques[newTechniques.length - 1] = technique;
		extractionTechniques = newTechniques;
		return true;
	}
	
	/**
	 * Sets the interpretation of the question used to generate this query.
	 * 
	 * @param qi question interpretation
	 */
	public void setInterpretation(QuestionInterpretation qi) {
		this.qi = qi;
	}
	
	/**
	 * Return a copy of this <code>Query</code> object.
	 * 
	 * @return copy of the <code>Query</code> object
	 */
	public Query getCopy() {
		Query query = new Query(queryString, analyzedQuestion, score);
		query.extractionTechniques = extractionTechniques;
		query.qi = qi;
		
		return query;
	}
}

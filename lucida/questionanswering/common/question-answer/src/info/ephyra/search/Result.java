package info.ephyra.search;

import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.querygeneration.Query;
import info.ephyra.questionanalysis.Term;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

/**
 * <p>A <code>Result</code> is a data structure representing a result returned
 * by the QA engine.</p>
 * 
 * <p>It comprises the following elements:
 * <ul>
 * <li>answer string</li>
 * <li>score which is a confidence measure for the answer</li>
 * <li>query used to obtain the answer (optional)</li>
 * <li>ID (e.g. a URL) of a document containing the answer (optional)</li>
 * <li>hit position at which the answer was returned by a search engine
 *     (optional)</li>
 * <li>flag indicating whether the answer was judged correct (optional)</li>
 * <li>other elements depending on the granularity of the answer string
 *     (e.g. sentence, factoid)</li>
 * </ul></p>
 * 
 * <p>This class implements the interfaces <code>Comparable</code> and
 * <code>Serializable</code>. Note: it has a natural ordering that is
 * inconsistent with <code>equals()</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-01-29
 */
public class Result implements Comparable<Result>, Serializable {
	/** Version number used during deserialization. */
	private static final long serialVersionUID = 20070501;
	
	/** The answer string. */
	private String answer;
	/** A confidence measure for the answer, initially 0. */
	private float score = 0;
	/** A normalized confidence measure for the answer (optional). */
	private float normScore = 0;
	/** The <code>Query</code> that was used to obtain the answer (optional). */
	private Query query;
	/** The ID (e.g. a URL) of a document containing the answer (optional). */
	private String docID;
	/** The ID of the document in the search engine cache (optional). */
	private String cacheID;
	/** The hit position of the answer, starting from 0 (optional). */
	private int hitPos = -1;
	/** A flag indicating whether the answer was judged correct (optional). */
	private boolean correct;
	/** Hashmap holding intermediate scores so they don't influence sorting*/
	private HashMap<String, Float> extraScores = new HashMap<String, Float>();
	
	/**
	 * If this is a sentence-level answer, named entities extracted from the
	 * sentence and their types (optional).
	 */
	private Map<String, String[]> nes;
	/**
	 * If this is a sentence-level answer, terms extracted from the sentence
	 * (optional).
	 */
	private Term[] terms;
	/**
	 * If this is a sentence-level answer, a predicate extracted from the
	 * sentence (optional).
	 */
	private Predicate predicate;
	
	/**
	 * If this is a factoid answer, a sentence in the supporting document the
	 * answer was extracted from (optional).
	 */
	private String sentence;
	/** If this is a factoid answer, the named entity types (optional). */
	private String[] neTypes;
	/**
	 * If this is a factoid answer, the techniques used to extract it
	 * (optional).
	 */
	private String[] extractionTechniques;
	
	/**
	 * If this is an answer to an 'other' question, list to keep the IDs of
	 * covered nugget (optional).
	 */
	private ArrayList<String> coveredNuggets = new ArrayList<String>();
	
	/**
	 * Creates a <code>Result</code> object and sets the answer string.
	 * 
	 * @param answer answer string
	 */
	public Result(String answer) {
		this.answer = answer;
	}
	
	/**
	 * Creates a <code>Result</code> object and sets the answer string and the
	 * <code>Query</code> that was used to obtain the answer.
	 * 
	 * @param answer answer string
	 * @param query <code>Query</code> object
	 */
	public Result(String answer, Query query) {
		this(answer);
		
		this.query = query;
	}
	
	/**
	 * Creates a <code>Result</code> object and sets the answer string, the
	 * <code>Query</code> that was used to obtain the answer and the ID of a
	 * document that contains it.
	 * 
	 * @param answer answer string
	 * @param query <code>Query</code> object
	 * @param docID document ID
	 */
	public Result(String answer, Query query, String docID) {
		this(answer,query);
		
		this.docID = docID;
	}
	
	/**
	 * Creates a <code>Result</code> object and sets the answer string, the
	 * <code>Query</code> that was used to obtain the answer, the ID of a
	 * document that contains it and the hit position.
	 * 
	 * @param answer answer string
	 * @param query <code>Query</code> object
	 * @param docID document ID
	 * @param hitPos hit position, starting from 0
	 */
	public Result(String answer, Query query, String docID, int hitPos) {
		this(answer, query, docID);
		
		this.hitPos = hitPos;
	}
	
	/**
	 * Compares two results by comparing their scores.
	 * 
	 * @param result the result to be compared
	 * @return a negative integer, zero or a positive integer as this result is
	 *         less than, equal to or greater than the specified result
	 */
	public int compareTo(Result result) {
		float diff = score - result.getScore();
		
		if (diff < 0)
			return -1;
		else if (diff > 0)
			return 1;
		else
			return 0;
//			return answer.compareTo(result.getAnswer());  // tie-breaking
	}
	
	/**
	 * Indicates whether an other result is equal to this one.
	 * Two results are considered equal if the answer strings are equal.
	 * 
	 * @param o the object to be compared
	 * @return <code>true</code> iff the objects are equal
	 */
	public boolean equals(Object o) {
		// if objects incomparable, return false
		if (!(o instanceof Result)) return false;
		Result result = (Result) o;
		
		return answer.equals(result.answer);
	}
	
	/**
	 * Returns the hash code of the answer string as a hash code for the result.
	 * 
	 * @return hash code
	 */
	public int hashCode() {
		return answer.hashCode();
	}
	
	/**
	 * Returns the answer string.
	 * 
	 * @return answer string
	 */
	public String getAnswer() {
		return answer;
	}
	
	/**
	 * Returns the confidence score of the result.
	 * 
	 * @return confidence score
	 */
	public float getScore() {
		return score;
	}
	
	/**
	 * Returns the normalized score of the result.
	 * 
	 * @return normalized score
	 */
	public float getNormScore() {
		return normScore;
	}
	
	/**
	 * Returns the <code>Query</code> that was used to obtain this result, or
	 * <code>null</code> if it is not set.
	 * 
	 * @return <code>Query</code> used to obtain this result or
	 * 		   <code>null</code>
	 */
	public Query getQuery() {
		return query;
	}
	
	/**
	 * Returns the ID of a document that contains the answer or
	 * <code>null</code> if it is not set.
	 * 
	 * @return document ID or <code>null</code>
	 */
	public String getDocID() {
		return docID;
	}
	
	/**
	 * Returns the ID of the document in the search engine cache or
	 * <code>null</code> if it is not set.
	 * 
	 * @return ID of the cached document or <code>null</code>
	 */
	public String getCacheID() {
		return cacheID;
	}
	
	/**
	 * Returns the hit position of the result, starting from 0, or -1 if it is
	 * not set.
	 * 
	 * @return hit position or -1
	 */
	public int getHitPos() {
		return hitPos;
	}
	
	/**
	 * Checks if the answer was judged correct.
	 * 
	 * @return <code>true</code> iff the answer was judged as correct
	 */
	public boolean isCorrect() {
		return correct;
	}
	
	/**
	 * Returns named entities extracted from a sentence-level answer and their
	 * types.
	 * 
	 * @return NEs or <code>null</code> if no NEs have been extracted
	 */
	public Map<String, String[]> getNes() {
		return nes;
	}
	
	/**
	 * Returns terms extracted from a sentence-level answer.
	 * 
	 * @return terms or <code>null</code> if no terms have been extracted
	 */
	public Term[] getTerms() {
		return terms;
	}
	
	/**
	 * Returns a predicate extracted from a sentence-level answer.
	 * 
	 * @return predicate or <code>null</code> if no predicate has been extracted
	 */
	public Predicate getPredicate() {
		return predicate;
	}
	
	/**
	 * Returns the supporting sentence of a factoid answer.
	 * 
	 * @return supporting sentence or <code>null</code> if it is not set
	 */
	public String getSentence() {
		return sentence;
	}
	
	/**
	 * Checks if a factoid answer is a named entity.
	 * 
	 * @return <code>true</code> iff the answer is a NE
	 */
	public boolean isNamedEntity() {
		return neTypes != null;
	}
	
	/**
	 * Gets the named entity types if the answer is a named entity.
	 * 
	 * @return NE types or <code>null</code> if the answer is not a NE
	 */
	public String[] getNeTypes() {
		return neTypes;
	}
	
	/**
	 * Returns the techniques a factoid answer was extracted with.
	 * 
	 * @return answer extraction techniques
	 */
	public String[] getExtractionTechniques() {
		return extractionTechniques;
	}
	
	/**
	 * Checks if a factoid answer was extracted with the given technique.
	 * 
	 * @param technique answer extraction technique
	 * @return <code>true</code> iff the answer was extracted with the technique
	 */
	public boolean extractedWith(String technique) {
		if (extractionTechniques == null) return false;
		
		for (String extractionTechnique : extractionTechniques)
			if (technique.equals(extractionTechnique)) return true;
		
		return false;
	}
	
	/**	
	 * Returns the IDs of all nuggets covered by an answer to an 'other'
	 * question.
	 * 
	 * @return IDs of all covered nuggets
	 */
	public String[] getCoveredNuggetIDs() {
		return coveredNuggets.toArray(new String[coveredNuggets.size()]);
	}
	
	/**
	 * Sets the answer string.
	 * 
	 * @param answer the answer string
	 */
	public void setAnswer(String answer) {
		this.answer = answer;
	}
	
	/**
	 * Sets the confidence score of this result.
	 * 
	 * @param score confidence score
	 */
	public void setScore(float score) {
		this.score = score;
	}
	
	/**
	 * Increments the confidence score by the given value.
	 * 
	 * @param value the value to be added to the score
	 */
	public void incScore(float value) {
		score += value;
	}
	
	/**
	 * Sets the normalized score of this result.
	 * 
	 * @param normScore normalized score
	 */
	public void setNormScore(float normScore) {
		this.normScore = normScore;
	}
	
	/**
	 * Sets the ID of a document that contains the answer.
	 * 
	 * @param docID document ID
	 */
	public void setDocID(String docID) {
		this.docID = docID;
	}
	
	/**
	 * Sets the ID of the document in the search engine cache.
	 * 
	 * @param cacheID ID of the cached document
	 */
	public void setCacheID(String cacheID) {
		this.cacheID = cacheID;
	}
	
	/**
	 * Judges the answer as correct.
	 */
	public void setCorrect() {
		correct = true;
	}
	
	/**
	 * Sets named entities extracted from a sentence-level answer and their
	 * types.
	 * 
	 * @param nes NEs and their types
	 */
	public void setNes(Map<String, String[]> nes) {
		this.nes = nes;
	}
	
	/**
	 * Sets terms extracted from a sentence-level answer.
	 * 
	 * @param terms terms extracted from a sentence
	 */
	public void setTerms(Term[] terms) {
		this.terms = terms;
	}
	
	/**
	 * Sets a predicate extracted from a sentence-level answer.
	 * 
	 * @param predicate predicate extracted from a sentence
	 */
	public void setPredicate(Predicate predicate) {
		this.predicate = predicate;
	}
	
	/**
	 * Sets the supporting sentence of a factoid answer.
	 * 
	 * @param sentence supporting sentence
	 */
	public void setSentence(String sentence) {
		this.sentence = sentence.trim();
	}
	
	/**
	 * Adds a named entity type for a factoid answer.
	 * 
	 * @param neType NE type
	 * @return <code>true</code> if a new type was added
	 */
	public boolean addNeType(String neType) {
		// check if NE type has already been added
		if (neTypes != null)
			for (String existing : neTypes)
				if (neType.equals(existing)) return false;
		
		// add new NE type
		String[] newNeTypes;
		if (neTypes == null) {
			newNeTypes = new String[1];
		} else {
			newNeTypes = new String[neTypes.length + 1];
			for (int i = 0; i < neTypes.length; i++)
				newNeTypes[i] = neTypes[i];
		}
		newNeTypes[newNeTypes.length - 1] = neType;
		neTypes = newNeTypes;
		return true;
	}
	
	/**
	 * Sets the techniques a factoid answer was extracted with.
	 * 
	 * @param techniques answer extraction techniques
	 */
	public void setExtractionTechniques(String[] techniques) {
		extractionTechniques = techniques;
	}
	
	/**
	 * Adds an answer extraction technique for a factoid answer.
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
	 * Adds the ID of a nugget covered by an answer to an 'other' question.
	 * 
	 * @param nuggetID ID of the nugget to add 
	 */
	public void addCoveredNuggetID(String nuggetID) {
		this.coveredNuggets.add(nuggetID);
	}
	
	/**
	 * Returns a copy of this <code>Result</code> object.
	 * 
	 * @return copy of this object
	 */
	public Result getCopy() {
		Result result = new Result(answer, query, docID, hitPos);
		result.score = score;
		result.normScore = normScore;
		result.cacheID = cacheID;
		result.correct = correct;
		result.nes = nes;
		result.terms = terms;
		result.predicate = predicate;
		result.sentence = sentence;
		result.neTypes = neTypes;
		result.extractionTechniques = extractionTechniques;
		result.coveredNuggets = coveredNuggets;
		result.extraScores.putAll(this.extraScores);
		
		return result;
	}
	
	/**	add an extra score to this Result for storage, extra score will not influence sorting
	 * @param	sourceName	the name of the source of the score
	 * @param	score		the value of the score
	 */
	public void addExtraScore(String sourceName, float score) {
		this.extraScores.put(sourceName, new Float(score));
	}
	
	/**	retrieve the extra score set by some source
	 * @param	sourceName	the name of the source who set the required score
	 * @return the extra score set by the source with the specified name, or 0, if the is no suchh extra score
	 */
	public float getExtraScore(String sourceName) {
		if (this.extraScores.containsKey(sourceName)) return this.extraScores.get(sourceName).floatValue();
		else return 0f;
	}
	
	/**	retrieve all extra scores set for this Result
	 * @return an array holding all the extra scores
	 */
	public float[] getExtraScores() {
		float[] scores = new float[this.extraScores.size()];
		int i = 0;
		for (Iterator<Float> iter = this.extraScores.values().iterator(); iter.hasNext(); i++)
			scores[i] = iter.next().floatValue();
		return scores;
	}
}

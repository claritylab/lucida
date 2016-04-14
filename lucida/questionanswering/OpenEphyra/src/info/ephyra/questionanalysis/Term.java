package info.ephyra.questionanalysis;

import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.nlp.semantics.ontologies.WordNet;
import info.ephyra.util.StringUtils;

import java.io.Serializable;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Map;
import java.util.Set;

import net.didion.jwnl.data.POS;

/**
 * <p>A <code>Term</code> comprises one or more tokens of text that form a unit
 * of meaning. It can be an individual word, a compound noun or a named entity.
 * </p>
 * 
 * <p>This class implements the interface <code>Serializable</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-01-23
 */
public class Term implements Serializable {
	/** Version number used during deserialization. */
	private static final long serialVersionUID = 20070501;
	
	/** Part of speech tag for terms that comprise multiple tokens */
	public static final String COMPOUND = "COMPOUND";
	
	/** The textual representation of the term. */
	private String text;
	/** The lemma of the term. */
	private String lemma;
	/**
	 * The part of speech of the term or <code>COMPOUND</code> to indicate that
	 * it comprises multiple tokens.
	 */
	private String pos;
	/** The named entity types of the term (optional). */
	private String[] neTypes = new String[0];
	/** Relative frequency of the term. */
	private double relFrequency;
	/** Maps expansions of the term to their weights. */
	private Map<String, Double> expansions;
	/** Maps lemmas of the expansions to their weights. */
	private Map<String, Double> expansionLemmas;
	
	// Getters/Setters
	public String getText() {return text;}
	public String getLemma() {return lemma;}
	public String getPos() {return pos;}
	public String[] getNeTypes() {return neTypes;}
	public void setNeTypes(String[] neTypes) {this.neTypes = neTypes;}
	public double getRelFrequency() {return relFrequency;}
	public void setRelFrequency(double relFrequency) {
		this.relFrequency = relFrequency;}
	public Map<String, Double> getExpansions() {return expansions;}
	public void setExpansions(Map<String, Double> expansions) {
		this.expansions = expansions;}
	
	/**
	 * Constructs a term from the provided information.
	 * 
	 * @param text textual representation
	 * @param pos part of speech
	 */
	public Term(String text, String pos) {
		this.text = text;
		this.pos = pos;
		
		// derive the lemma
		generateLemma();
	}
	
	/**
	 * Constructs a term from the provided information.
	 * 
	 * @param text textual representation
	 * @param pos part of speech
	 * @param neTypes named entity types
	 */
	public Term(String text, String pos, String[] neTypes) {
		this(text, pos);
		this.neTypes = neTypes;
	}
	
	/**
	 * Generates the lemma of the term.
	 */
	private void generateLemma() {
		String lemma;
		if (pos.startsWith("VB")) {
			// lemmatize verbs that are in WordNet
			lemma = WordNet.getLemma(text, POS.VERB);
		} else if (pos.startsWith("JJ")) {
			// lemmatize adjectives that are in WordNet
			lemma = WordNet.getLemma(text, POS.ADJECTIVE);
		} else if (pos.startsWith("RB")) {
			// lemmatize adverbs that are in WordNet
			lemma = WordNet.getLemma(text, POS.ADVERB);
		} else {
			// lemmatize nouns that are in WordNet
			if (pos.startsWith("COMPOUND"))
				lemma = WordNet.getCompoundLemma(text, POS.NOUN);  // compound
			else
				lemma = WordNet.getLemma(text, POS.NOUN);  // single token
		}
		if (lemma == null) lemma = text;
		
		setLemma(lemma);
	}
	
	/**
	 * Normalizes and sets the lemma of the term.
	 * 
	 * @param lemma the lemma of the term
	 */
	public void setLemma(String lemma) {
		this.lemma = StringUtils.normalize(lemma);
	}
	
	/**
	 * Normalizes and sets the lemmas of the expansions.
	 * 
	 * @param expansionLemmas the lemmas of the expansions
	 */
	public void setExpansionLemmas(Map<String, Double> expansionLemmas) {
		Map<String, Double> normalized = new Hashtable<String, Double>();
		
		for (String lemma : expansionLemmas.keySet()) {
			double weight = expansionLemmas.get(lemma);
			String norm = StringUtils.normalize(lemma);
			normalized.put(norm, weight);
		}
		
		this.expansionLemmas = normalized;
	}
	
	/**
	 * Gets the weight of the term or expansion with the given lemma.
	 * 
	 * @param lemma the lemma
	 * @return the weight or <code>0</code> if there is no match
	 */
	public double getWeight(String lemma) {
		if (lemma.equals(this.lemma)) return 1;
		
		if (expansionLemmas == null) return 0;
		Double weight = expansionLemmas.get(lemma);
		return (weight != null) ? weight : 0;
	}
	
	/**
	 * Calculates similarity scores for the given lemma and the lemmas of the
	 * term and its expansions based on their weights and the number of common
	 * tokens. Gets the maximum of all these scores.
	 * 
	 * @param lemma lemma to compare with
	 * @return similarity score
	 */
	public double simScore(String lemma) {
		// tokenize lemma,
		// eliminate duplicates, function words and tokens of length < 2
		String[] tokens = lemma.split(" ");
		Set<String> lookupSet = new HashSet<String>();
		for (String token : tokens)
			if (token.length() > 1 && !FunctionWords.lookup(token))
				lookupSet.add(token);
		if (lookupSet.size() == 0) return 0;
		
		// calculate similarity score for the term
		// (Jaccard coefficient)
		tokens = this.lemma.split(" ");
		Set<String> tokenSet = new HashSet<String>();
		for (String token : tokens)
			if (token.length() > 1 && !FunctionWords.lookup(token))
				tokenSet.add(token);
		double intersect = 0;
		int union = lookupSet.size();
		for (String token : tokenSet)
			if (lookupSet.contains(token)) intersect++; else union++;
		double simScore = intersect / union;
		
		// calculate similarity scores for the expansions
		// (Jaccard coefficient)
		for (String expansionLemma : expansionLemmas.keySet()) {
			tokens = expansionLemma.split(" ");
			tokenSet.clear();
			for (String token : tokens)
				if (token.length() > 1 && !FunctionWords.lookup(token))
					tokenSet.add(token);
			double weight = expansionLemmas.get(expansionLemma);
			intersect = 0;
			union = lookupSet.size();
			for (String token : tokenSet)
				if (lookupSet.contains(token)) intersect++; else union++;
			simScore = Math.max(simScore, weight * (intersect / union));
		}
		
		return simScore;
	}
	
	/**
	 * Creates a string representation of the term.
	 * 
	 * @return string representation
	 */
	public String toString() {
		String s = "{\"" + text + "\"; POS: " + pos;
		if (neTypes.length > 0) {
			s += "; NE types: " + neTypes[0];
			for (int i = 1; i < neTypes.length; i++)
				s += ", " + neTypes[i];
		}
		if (expansions != null &&  expansions.size() > 0) {
			String[] texts =
				expansions.keySet().toArray(new String[expansions.size()]);
			s += "; Expansions: {" + texts[0] + "=" + expansions.get(texts[0]);
			for (int i = 1; i < expansions.size(); i++)
				s += ", " + texts[i] + "=" + expansions.get(texts[i]);
			s += "}";
		}
		s += "}";
		
		return s;
	}
}

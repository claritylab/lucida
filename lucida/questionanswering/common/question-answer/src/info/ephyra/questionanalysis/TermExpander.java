package info.ephyra.questionanalysis;

import info.ephyra.nlp.VerbFormConverter;
import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.nlp.semantics.ontologies.Ontology;
import info.ephyra.nlp.semantics.ontologies.WordNet;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import net.didion.jwnl.data.POS;

/**
 * Expands single- and multi-token terms by looking them up in one or more open-
 * or specific-domain ontologies.
 * 
 * @author Nico Schlaefer
 * @version 2007-02-30
 */
public class TermExpander {
	/** Maximum number of expansions. */
	public static final int MAX_EXPANSIONS = 100;
	/** Minimum weight of an expansion. */
	public static final double MIN_EXPANSION_WEIGHT = 0;
	
	/** Maximum number of expansions used in a query. */
	public static final int MAX_EXPANSIONS_QUERY = 10;
	/** Minimum weight of an expansion used in a query. */
	public static final double MIN_EXPANSION_WEIGHT_QUERY = 0.9;
	
	/**
	 * Checks if the term is the target of one of the predicates.
	 * 
	 * @param term a term
	 * @param ps predicates in the same sentence
	 * @return <code>true</code> iff the term is a predicate target
	 */
	private static boolean isTarget(Term term, Predicate[] ps) {
		String text = term.getText();
		String lemma = WordNet.getLemma(text, POS.VERB);
		
		for (Predicate p : ps) {
			String verb = p.getVerb();
			if (verb.matches(".*?\\b" + text + "\\b.*+"))
				return true;
			// also try the infinitive in case the verb form was modified in the
			// term but not in the predicate (e.g. because the predicate was
			// built from an offset annotation)
			if (lemma != null && verb.matches(".*?\\b" + lemma + "\\b.*+"))
				return true;
		}
		
		return false;
	}
	
	/**
	 * Drops expansions that do not contain any keywords and applies the given
	 * thresholds for the maximum number of expansions and the minimum weight of
	 * an expansion.
	 * 
	 * @param expansions expansions along with their weights
	 * @param maxExpansions maximum number of expansions
	 * @param minExpansionWeight minimum weight of an expansion
	 * @param strict iff <code>true</code>, the threshold for the maximum number
	 *               of expansions is applied strictly even if that means that
	 *               some of a number of expansion with equal weights have to be
	 *               dropped randomly
	 * @return minimum weight of an expansion in the reduced map
	 */
	public static double cutOffExpansions(Map<String, Double> expansions,
			int maxExpansions, double minExpansionWeight, boolean strict) {
		// drop duplicates and expansions that do not contain keywords
		ArrayList<String> dropped = new ArrayList<String>();
		HashSet<String> expansionSet = new HashSet<String>();
		for (String expansion : expansions.keySet()) {
			if (!expansionSet.add(StringUtils.normalize(expansion))) {
				dropped.add(expansion);
				continue;
			}
			String[] kws = KeywordExtractor.getKeywords(expansion);
			if (kws.length == 0) dropped.add(expansion);
		}
		for (String expansion : dropped) expansions.remove(expansion);
		
		// get hurdle for weights of expansions to satisfy both thresholds
		if (expansions.size() == 0) return 0;
		Double[] weights =
			expansions.values().toArray(new Double[expansions.size()]);
		Arrays.sort(weights);
		double hurdle = weights[Math.max(weights.length - maxExpansions, 0)];
		hurdle = Math.max(hurdle, minExpansionWeight);
		
		// drop expansions that have a weight below that hurdle
		dropped.clear();
		for (String expansion : expansions.keySet())
			if (expansions.get(expansion) < hurdle)
				dropped.add(expansion);
		for (String expansion : dropped) expansions.remove(expansion);
		
		dropped.clear();
		if (strict) {
			// drop as many expansions with a weight equal to the hurdle
			// as required to satisfy the MAX_EXPANSIONS threshold
			int numToDrop = Math.max(expansions.size() - maxExpansions, 0);
			for (String expansion : expansions.keySet()) {
				if (numToDrop == 0) break;
				if (expansions.get(expansion) == hurdle) {
					dropped.add(expansion);
					numToDrop--;
				}
			}
			for (String expansion : dropped) expansions.remove(expansion);
		}
		
		return hurdle;
	}
	
	/**
	 * Drops expansions that do not contain any keywords and applies the default
	 * thresholds for the maximum number of expansions and the minimum weight of
	 * an expansion.
	 * 
	 * @param expansions expansions along with their weights
	 * @param strict iff <code>true</code>, the threshold for the maximum number
	 *               of expansions is applied strictly even if that means that
	 *               some of a number of expansion with equal weights have to be
	 *               dropped randomly
	 * @return minimum weight of an expansion in the reduced map
	 */
	public static double cutOffExpansions(Map<String, Double> expansions,
			boolean strict) {
		return cutOffExpansions(expansions, MAX_EXPANSIONS,
				MIN_EXPANSION_WEIGHT, strict);
	}
	
	/**
	 * Creates a new expansion map and applies the thresholds for the maximum
	 * number of expansions and the minimum weight of an expansion for queries.
	 * 
	 * @param expansions expansions along with their weights
	 * @param strict iff <code>true</code>, the threshold for the maximum number
	 *               of expansions is applied strictly even if that means that
	 *               some of a number of expansion with equal weights have to be
	 *               dropped randomly
	 * @return new reduced expansion map for queries
	 */
	public static Map<String, Double> reduceExpansionsQuery(
			Map<String, Double> expansions, boolean strict) {
		HashMap<String, Double> map = new HashMap<String, Double>();
		for (String expansion : expansions.keySet())
			map.put(expansion, expansions.get(expansion));
		
		cutOffExpansions(map, MAX_EXPANSIONS_QUERY, MIN_EXPANSION_WEIGHT_QUERY,
				strict);
		
		return map;
	}
	
	/**
	 * Expands a term by looking up related terms in ontologies.
	 * 
	 * @param term a term
	 * @param ps predicates in the same sentence
	 * @param ontologies ontologies used to expand the term
	 */
	public static void expandTerm(Term term, Predicate[] ps,
			Ontology[] ontologies) {
		String text = term.getText();
		String pos = term.getPos();
		Map<String, Double> lemmas = new Hashtable<String, Double>();
		Map<String, Double> expansions = new Hashtable<String, Double>();
		
		// expand events, entities and modifiers
		if (isTarget(term, ps) || pos.startsWith("VB")) {
			// lemmatize verbs that are in WordNet
			String lemma = WordNet.getLemma(text, POS.VERB);
			if (lemma == null) lemma = text;
			// set lemma if the POS was misleading
			if (!pos.startsWith("VB")) term.setLemma(lemma);
			
			// expand event
			for (Ontology ontology : ontologies) {
				Map<String, Double> expanded = ontology.expandEvent(lemma);
				lemmas.putAll(expanded);
			}
			
			// ensure that there are at most MAX_EXPANSIONS expansions with
			// weights of at least MIN_EXPANSION_WEIGHT
			cutOffExpansions(lemmas, true);
			
			// restore verb form
			if (pos.equals("VBZ")) {
				// third person singular
				for (String exp : lemmas.keySet()) {
					double weight = lemmas.get(exp);
					String form =
						VerbFormConverter.infinitiveToThirdPersonS(exp);
					expansions.put(form, weight);
				}
			} else if (pos.equals("VBG")) {
				// gerund
				for (String exp : lemmas.keySet()) {
					double weight = lemmas.get(exp);
					String[] forms =
						VerbFormConverter.infinitiveToGerund(exp);
					for (String form : forms) expansions.put(form, weight);
				}
			} else if (pos.equals("VBD")) {
				// simple past
				for (String exp : lemmas.keySet()) {
					double weight = lemmas.get(exp);
					String[] forms =
						VerbFormConverter.infinitiveToSimplePast(exp);
					for (String form : forms) expansions.put(form, weight);
				}
			} else if (pos.equals("VBN")) {
				// past participle
				for (String exp : lemmas.keySet()) {
					double weight = lemmas.get(exp);
					String[] forms =
						VerbFormConverter.infinitiveToPastParticiple(exp);
					for (String form : forms) expansions.put(form, weight);
				}
			}
		} else if (pos.startsWith("JJ") || pos.startsWith("RB")) {
			// get modifier type
			POS modType =
				(pos.startsWith("JJ")) ? POS.ADJECTIVE : POS.ADVERB;
			
			// lemmatize adjectives and adverbs that are in WordNet
			String lemma = WordNet.getLemma(text, modType);
			if (lemma == null) lemma = text;
			
			// expand modifier
			for (Ontology ontology : ontologies) {
				Map<String, Double> expanded =
					ontology.expandModifier(lemma, modType);
				lemmas.putAll(expanded);
			}
			
			// ensure that there are at most MAX_EXPANSIONS expansions with
			// weights of at least MIN_EXPANSION_WEIGHT
			cutOffExpansions(lemmas, true);
		} else {
			// lemmatize nouns that are in WordNet
			String lemma;
			if (pos.startsWith("COMPOUND"))
				lemma = WordNet.getCompoundLemma(text, POS.NOUN);  // compound
			else
				lemma = WordNet.getLemma(text, POS.NOUN);  // single token
			if (lemma == null) lemma = text;
			
			// expand entity
			for (Ontology ontology : ontologies) {
				Map<String, Double> expanded = ontology.expandEntity(lemma);
				lemmas.putAll(expanded);
			}
			
			// ensure that there are at most MAX_EXPANSIONS expansions with
			// weights of at least MIN_EXPANSION_WEIGHT
			cutOffExpansions(lemmas, true);
			
			// TODO restore plural forms if possible
		}
		
		term.setExpansionLemmas(lemmas);
		term.setExpansions((expansions.size() > 0) ? expansions : lemmas);
	}
	
	/**
	 * Expands all terms by looking up related terms in ontologies.
	 * 
	 * @param terms the terms
	 * @param ps predicates in the same sentence
	 * @param ontologies ontologies used to expand the term
	 */
	public static void expandTerms(Term[] terms, Predicate[] ps,
			Ontology[] ontologies) {
		for (Term term : terms) expandTerm(term, ps, ontologies);
	}
	
	/**
	 * Expands a phrase by replacing the terms that occur within the phrase by
	 * their expansions. All possible combinations of expansions of the
	 * individual terms are formed and each is assigned the product of the
	 * weights of the expansions as a combined weight. The (at most)
	 * <code>MAX_EXPANSIONS</code> resulting phrases with the highest weights
	 * are returned.
	 * 
	 * @param phrase phrase to expand
	 * @param terms expanded terms that potentially occur within the phrase
	 * @return expansions and their weights
	 */
	public static Map<String, Double> expandPhrase(String phrase,
			Term[] terms) {
		// regular expressions that match the terms
		List<String> patterns = new ArrayList<String>();
		// maps the terms to their expansions
		Map<String, Map<String, Double>> expansionsMap =
			new Hashtable<String, Map<String, Double>>();
		for (Term term : terms) {
			Map<String, Double> expansions = term.getExpansions();
			if (expansions.size() > 0) {
				String pattern = "\\b" + term.getText() + "\\b";
				patterns.add(pattern);
				expansionsMap.put(pattern, expansions);
			}
		}
		
		Map<String, Double> phraseExps = new Hashtable<String, Double>();
		phraseExps.put(phrase, 1d);
		
		// obtain phrase expansions by combining term expansions
		while (patterns.size() > 0) {
			String[] phrases =
				phraseExps.keySet().toArray(new String[phraseExps.size()]);
			String pattern = patterns.get(0);
			Map<String, Double> expansions = expansionsMap.get(pattern);
			
			for (String phraseExp : phrases) {
				Matcher m =
					Pattern.compile(".*?" + pattern + ".*+").matcher(phraseExp);
				if (m.matches()) {
					for (String expansion : expansions.keySet()) {
						String expanded = phraseExp.replaceFirst(pattern,
							expansion);
						Double weight = phraseExps.get(phraseExp) *
							expansions.get(expansion);
						phraseExps.put(expanded, weight);
					}
				} else {
					// no (further) occurrences of the term
					patterns.remove(0);
					break;
				}
			}
		}
		
		// ensure that there are at most MAX_EXPANSIONS phrases with weights of
		// at least MIN_EXPANSION_WEIGHT
		phraseExps.remove(phrase);
		cutOffExpansions(phraseExps, true);
		return phraseExps;
	}
}

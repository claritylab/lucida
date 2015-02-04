package info.ephyra.questionanalysis;

import info.ephyra.answerselection.filters.TruncationFilter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.util.Dictionary;
import info.ephyra.util.StringUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Extracts single- and multi-token terms from a sentence. Multi-token terms are
 * named entities or compound terms found in dictionaries.
 * 
 * @author Nico Schlaefer
 * @version 2007-05-28
 */
public class TermExtractor {
	/** Maximum length of a term in tokens. */
	private static final int MAX_TERM_LENGTH = 4;
	
	/**
	 * Checks if the given term is among the named entities and returns the
	 * types of the entities that match it.
	 * 
	 * @param term a term, potentially a named entity
	 * @param nes named entities
	 * @return types of matching entities
	 */
	private static String[] getNeTypes(String term, String[][] nes) {
		List<String> neTypes = new ArrayList<String>();
		Set<String> neTypesSet = new HashSet<String>();
		
		for (int neId = 0; neId < nes.length; neId++)
			for (String ne : nes[neId])
				if (term.equals(ne)) {
					String neType = NETagger.getNeType(neId);
					if (neTypesSet.add(neType))
						// there may be multiple taggers (IDs) for one type
						neTypes.add(neType);
					break;
				}
		
		return neTypes.toArray(new String[neTypes.size()]);
	}
	
	/**
	 * Extracts named entities from the given sentence.
	 * 
	 * @param sentence sentence to analyze
	 * @return named entities in the sentence
	 */
	public static String[][] getNes(String sentence) {
		String[] tokens = NETagger.tokenize(sentence);
		String[][] nes = NETagger.extractNes(new String[][] {tokens})[0];
		
		// untokenize named entities
		for (int i = 0; i < nes.length; i++)
			for (int j = 0; j < nes[i].length; j++)
				nes[i][j] = OpenNLP.untokenize(nes[i][j], sentence);
		
		return nes;
	}
	
	/**
	 * Extracts named entities from the given sentence and context string.
	 * 
	 * @param sentence sentence to analyze
	 * @param context context string
	 * @return named entities in the sentence and context string
	 */
	public static String[][] getNes(String sentence, String context) {
		// extract NEs from sentence
		String[][] sentenceNes = getNes(sentence);
		if (context == null || context.length() == 0) return sentenceNes;
		
		// extract NEs from context string
		String[][] contextNes = getNes(context);
		
		// merge NEs
		String[][] nes = new String[sentenceNes.length][];
		for (int i = 0; i < nes.length; i++) {
			if (sentenceNes[i].length == 0) nes[i] = contextNes[i];
			else if (contextNes[i].length == 0) nes[i] = sentenceNes[i];
			else {
				ArrayList<String> nesL = new ArrayList<String>();
				for (String ne : sentenceNes[i]) nesL.add(ne);
				for (String ne : contextNes[i]) nesL.add(ne);
				nes[i] = nesL.toArray(new String[nesL.size()]);
			}
		}
		return nes;
	}
	
	/**
	 * Extracts terms from the given sentence.
	 * 
	 * @param sentence sentence to analyze
	 * @param dicts dictionaries with compound terms
	 * @return terms in the sentence
	 */
	public static Term[] getTerms(String sentence, Dictionary[] dicts) {
		String[][] nes = getNes(sentence);
		
		return getTerms(sentence, nes, dicts);
	}
	
	/**
	 * Extracts terms from the given sentence, reusing named entities that have
	 * been extracted before.
	 * 
	 * @param sentence sentence to analyze
	 * @param nes named entities in the sentence
	 * @param dicts dictionaries with compound terms
	 * @return terms in the sentence
	 */
	public static Term[] getTerms(String sentence, String[][] nes,
			Dictionary[] dicts) {
		// extract tokens
		String[] tokens = OpenNLP.tokenize(sentence);
		// tag part of speech
		String[] pos = OpenNLP.tagPos(tokens);
		// tag phrase chunks
		String[] chunks = OpenNLP.tagChunks(tokens, pos);
		// mark tokens as not yet assigned to a term
		boolean[] assigned = new boolean[tokens.length];
		Arrays.fill(assigned, false);
		
		// for each token a term that starts at that token or 'null'
		Term[] terms = new Term[tokens.length];
		// normalized terms (do identify duplicates)
		Set<String> termSet = new HashSet<String>();
		
		// construct multi-token terms
		for (int length = MAX_TERM_LENGTH; length > 1; length--)
			for (int id = 0; id < tokens.length - length + 1; id++) {
				// one of the tokens is already assigned to a term?
				boolean skip = false;
				for (int offset = 0; offset < length; offset++)
					if (assigned[id + offset]) {
						skip = true;
						continue;
					}
				if (skip) continue;
				
				// get phrase spanning the tokens
				String text = tokens[id];
				for (int offset = 1; offset < length; offset++)
					text += " " + tokens[id + offset];
				text = OpenNLP.untokenize(text, sentence);
				
				// phrase is a duplicate?
				if (!termSet.add(StringUtils.normalize(text))) continue;
				// phrase does not contain keywords?
				if (KeywordExtractor.getKeywords(text).length == 0) continue;
				
				// phrase is a named entity?
				String[] neTypes = getNeTypes(text, nes);
				if (neTypes.length > 0) {
					// construct term
					terms[id] = new Term(text, Term.COMPOUND, neTypes);
					// mark tokens as assigned
					for (int offset = 0; offset < length; offset++)
						assigned[id + offset] = true;
					continue;
				}
				
				for (Dictionary dict : dicts) {
					// phrase is not a noun phrase or verb phrase?
					if (!(chunks[id].endsWith("NP") &&  // look up noun phrases
							chunks[id + length - 1].endsWith("NP"))/* &&
						!(chunks[id].endsWith("VP") &&  // look up verb phrases
							chunks[id + length - 1].endsWith("VP"))*/)
						continue;
					
					// phrase contains a special characters other than '.'?
					if (text.matches(".*?[^\\w\\s\\.].*+")) continue;
					// phrase can be truncated?
					if (!text.equals(TruncationFilter.truncate(text))) continue;
					
					// phrase is in the dictionary?
					if (dict.contains(text)) {
						// construct term
						terms[id] = new Term(text, Term.COMPOUND);
						// mark tokens as assigned
						for (int offset = 0; offset < length; offset++)
							assigned[id + offset] = true;
						continue;
					}
				}
			}
		
		// construct single-token terms
		for (int id = 0; id < tokens.length; id++) {
			// token is part of a multi-token term?
			if (assigned[id]) continue;
			
			// token is a duplicate?
			if (!termSet.add(StringUtils.normalize(tokens[id]))) continue;
			// token does not contain keywords?
			if (KeywordExtractor.getKeywords(tokens[id]).length == 0) continue;
			
			// get named entity types and construct term
			String[] neTypes = getNeTypes(tokens[id], nes);
			terms[id] = new Term(tokens[id], pos[id], neTypes);
		}
		
		// get ordered list of terms
		List<Term> termsL = new ArrayList<Term>();
		for (Term term : terms)
			if (term != null) termsL.add(term);
		
		return termsL.toArray(new Term[termsL.size()]);
	}
	
	/**
	 * Extracts terms from the given sentence and context string.
	 * 
	 * @param sentence sentence to analyze
	 * @param context context string
	 * @param nes named entities in the sentence and context string
	 * @param dicts dictionaries with compound terms
	 * @return terms in the sentence and context string
	 */
	public static Term[] getTerms(String sentence, String context,
			String[][] nes, Dictionary[] dicts) {
		// extract terms from sentence
		Term[] sentenceTerms = getTerms(sentence, nes, dicts);
		if (context == null || context.length() == 0) return sentenceTerms;
		
		// extract terms from context string
		Term[] contextTerms = getTerms(context, nes, dicts);
		if (sentenceTerms.length == 0) return contextTerms;
		if (contextTerms.length == 0) return sentenceTerms;
		
		// merge terms, eliminate duplicates
		List<Term> terms = new ArrayList<Term>();
		Set<String> termSet = new HashSet<String>();
		for (Term sentenceTerm : sentenceTerms)
			if (termSet.add(StringUtils.normalize(sentenceTerm.getText())))
				terms.add(sentenceTerm);
		for (Term contextTerm : contextTerms)
			if (termSet.add(StringUtils.normalize(contextTerm.getText())))
				terms.add(contextTerm);
		return terms.toArray(new Term[terms.size()]);
	}
	
	/**
	 * Extracts single-token terms from the given sentence.
	 * 
	 * @param sentence sentence to analyze
	 * @return single-token terms in the sentence
	 */
	public static Term[] getSingleTokenTerms(String sentence) {
		// extract tokens
		String[] tokens = OpenNLP.tokenize(sentence);
		// tag part of speech
		String[] pos = OpenNLP.tagPos(tokens);
		
		// extracted terms
		ArrayList<Term> terms = new ArrayList<Term>();
		// normalized terms (do identify duplicates)
		Set<String> termSet = new HashSet<String>();
		
		// construct single-token terms
		for (int id = 0; id < tokens.length; id++) {
			// token is a duplicate?
			if (!termSet.add(StringUtils.normalize(tokens[id]))) continue;
			// token does not contain keywords?
			if (KeywordExtractor.getKeywords(tokens[id]).length == 0) continue;
			
			// construct term
			terms.add(new Term(tokens[id], pos[id]));
		}
		
		return terms.toArray(new Term[terms.size()]);
	}
}

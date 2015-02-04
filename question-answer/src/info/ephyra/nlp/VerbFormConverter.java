package info.ephyra.nlp;

import info.ephyra.nlp.indices.IrregularVerbs;
import info.ephyra.nlp.semantics.ontologies.WordNet;

import java.util.HashSet;

/**
 * Converts English verbs between infinitive, 3rd person singular, simple past,
 * and past participle.
 * 
 * @author Nico Schlaefer
 * @version 2007-05-17
 */
public class VerbFormConverter {
	/**
	 * Converts the infinitive of a regular verb to simple past.
	 * 
	 * @param verb regular verb in infinitive
	 * @return simple past forms of the regular verb
	 */
	private static String[] infinitiveToSimplePastReg(String verb) {
		if (verb.matches(".*e")){
			// If the verb ends in "e", only the letter "d" must be added in
			// order to form the simple past.
			return new String[] {verb + "d"};
		} else if (verb.matches(".*[bcdfghjklmnpqrstvwxyz]y")) {
			// If the verb ends in "y" immediately preceded by a consonant, the
			// "y" is changed to "i" before the ending "ed" is added.
			return new String[] {verb.substring(0, verb.length() - 1) + "ied"};
		} else if (verb.matches(".*[bcdfghjklmnpqrstvwxyz][aeiou]" + 
								"[bcdfghjklmnpqrstvz]")){
			// If the verb ends in a single consonant other than "w", "x" or "y"
			// immediately preceded by a single vowel, the final consonant must
			// be doubled only if it is a one-syllable verb or if the last
			// syllable is pronounced with the heaviest stress.
			// Since this is hard to check, two forms are created, one with a
			// single consonant and one with a doubled consonant.
			String sp1 = verb + "ed";
			String sp2 = verb + verb.substring(verb.length() - 1) + "ed";
			// WordNet is then used to eliminate misspellings.
			if (WordNet.isVerb(sp1))
				if (WordNet.isVerb(sp2))
					return new String[] {sp1, sp2};
				else
					return new String[] {sp1};
			else
				if (WordNet.isVerb(sp2))
					return new String[] {sp2};
				else
					return new String[] {sp1};  // fallback: return first form
		} else {
			// If none of the above cases applies, just add "ed".
			return new String[] {verb + "ed"};
		}
	}
	
	/**
	 * Converts the infinitive of a verb to 3rd person singular.
	 * 
	 * @param verb verb in infinitive
	 * @return 3rd person singular
	 */
	public static String infinitiveToThirdPersonS(String verb) {
		verb = verb.toLowerCase();
		
		if (verb.equals("have"))
			// "have" is irregular.
			return "has";
		else if (verb.matches(".*(ch|sh|s|x|z|o)"))
			// If the verb ends in "ch", "sh", "s", "x", "z" or "o",
			// append "es".
			return verb + "es";
		else if (verb.matches(".*[bcdfghjklmnpqrstvwxyz]y"))
			// If the verb ends in "y" immediately preceded by a consonant, drop
			// the "y" and append "ies".
			return verb.substring(0, verb.length() - 1) + "ies";
		else
			// If none of the above cases applies, just append "s".
			return verb + "s";
	}
	
	/**
	 * Converts the infinitive of a verb to its gerund (present progressive).
	 * 
	 * @param verb in infinitive
	 * @return gerund
	 */
	public static String[] infinitiveToGerund(String verb) {
		verb = verb.toLowerCase();
		
		if (verb.matches(".*[^e]e")){
			// If the verb ends in a single "e", the "e" is dropped and the
			// ending "ing" is added.
			return new String[] {verb.substring(0, verb.length() - 1) + "ing"};
		} else if (verb.matches(".*ie")) {
			// If the verb ends in "ie", the "ie" is changed to "y" before the
			// ending "ing" is added.
			return new String[] {verb.substring(0, verb.length() - 2) + "ying"};
		} else if (verb.matches(".*[aeiou][bcdfghjklmnpqrstvz]")){
			// If the verb ends in a consonant immediately preceded by a vowel,
			// two forms are generated, one with a single consonant and one with
			// a doubled consonant.
			String sp1 = verb + "ing";
			String sp2 = verb + verb.substring(verb.length() - 1) + "ing";
			// WordNet is then used to eliminate misspellings.
			if (WordNet.isVerb(sp1))
				if (WordNet.isVerb(sp2))
					return new String[] {sp1, sp2};
				else
					return new String[] {sp1};
			else
				if (WordNet.isVerb(sp2))
					return new String[] {sp2};
				else
					return new String[] {sp1};  // fallback: return first form
		} else {
			// If none of the above cases applies, just add "ing".
			return new String[] {verb + "ing"};
		}
	}
	
	/**
	 * Converts the infinitive of an arbitrary verb (regular or irregular) to
	 * simple past.
	 * 
	 * @param verb verb in infinitive
	 * @return simple past forms of the verb
	 */
	public static String[] infinitiveToSimplePast(String verb) {
		verb = verb.toLowerCase();
		
		String[] sp = IrregularVerbs.getSimplePast(verb);
		if (sp == null) sp = infinitiveToSimplePastReg(verb);  // regular verb
		
		return sp;
	}
	
	/**
	 * Converts the infinitive of an arbitrary verb (regular or irregular) to
	 * past participle.
	 * 
	 * @param verb verb in infinitive
	 * @return past participle forms of the verb
	 */
	public static String[] infinitiveToPastParticiple(String verb) {
		verb = verb.toLowerCase();
		
		String[] sp = IrregularVerbs.getPastParticiple(verb);
		if (sp == null) sp = infinitiveToSimplePastReg(verb);  // regular verb
		
		return sp;
	}
	
	/**
	 * Gets all grammatical forms of a verb and drops duplicates.
	 * 
	 * @param verb verb in infinitive
	 * @return all verb forms
	 */
	public static String[] getAllForms(String verb) {
		HashSet<String> allForms = new HashSet<String>();
		
		verb = verb.toLowerCase();
		allForms.add(verb);
		allForms.add(infinitiveToThirdPersonS(verb));
		for (String gerund : infinitiveToGerund(verb))
			allForms.add(gerund);
		for (String simplePast : infinitiveToSimplePast(verb))
			allForms.add(simplePast);
		for (String pastParticiple : infinitiveToPastParticiple(verb))
			allForms.add(pastParticiple);
		
		return allForms.toArray(new String[allForms.size()]);
	}
	
	/**
	 * Converts the past participle of an arbitrary verb (regular or irregular)
	 * to simple past.
	 * 
	 * @param verb verb in past participle
	 * @return simple past forms of the verb
	 */
	public static String[] pastParticipleToSimplePast(String verb) {
		verb = verb.toLowerCase();
		
		String[] sp = IrregularVerbs.getSimplePast(verb);
		// verb is regular -> simple past = past participle
		if (sp == null)	sp = new String[] {verb};
		
		return sp;
	}
}

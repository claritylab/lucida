package info.ephyra.trec;

import java.util.ArrayList;

/**
 * A preprocessor for TREC target descriptions.
 * 
 * @author Nico Schlaefer
 * @version 2007-07-23
 */
public class TargetPreprocessor {
	/**
	 * Creates a normalized target that is used as a query for the "Other"
	 * question and a condensed target that is appended to queries for factoid
	 * and list questions. Checks if the target is a noun phrase.
	 * 
	 * @param target a TREC target
	 */
	private static void normalize(TRECTarget target) {
		String /*targetDesc*/condensed = target.getTargetDesc();
		
		// normalized target used as a query for the "Other" question
//		String norm = "";
		// condensed target appended to queries for factoid and list questions
//		String condensed = "";
		// indicates if the target is a noun phrase
//		boolean nounPhrase = true;
		
		// convert verbs to simple past (normalized target)
		// or drop verbs (condensed target)
//		String[] tokens = targetDesc.split(" ");
//		for (String token : tokens) {
//			if (token.substring(0, 1).matches("[a-z]") &&  // lower case
//				WordNet.isVerb(token) && !WordNet.isNoun(token) &&  // verb
//				!WordNet.isAdjective(token) && !WordNet.isAdverb(token))
//			{
//				if (token.endsWith("ed")) {
//					// already simple past
//					if (norm.length() > 0) norm += " ";
//					norm += token;
//				} else {
//					String inf = WordNet.getInfinitive(token);
//					String[] ps = VerbFormConverter.infinitiveToSimplePast(inf);
//					
//					for (String p : ps) {
//						if (norm.length() > 0) norm += " ";
//						norm += p;
//					}
//				}
//				
//				nounPhrase = false;  // target is not a noun phrase
//			} else {
//				if (norm.length() > 0) norm += " ";
//				norm += token;
//				
//				if (condensed.length() > 0) condensed += " ";
//				condensed += token;
//			}
//		}
		
//		// drop acronyms in parenthesis (condensed target)
//		condensed = condensed.replaceAll("\\s*\\(\\s*" +
//				"([A-Z][a-z0-9\\.&]*){2,}\\s*\\)", "");
		// drops any string in parenthesis (condensed target)
		condensed = condensed.replaceAll("\\s*\\([^\\)]*\\)", "");
		
		// update target datastructure
//		target.setTargetDesc(norm);
		target.setCondensedTarget(condensed);
//		target.setNounPhrase(nounPhrase);
	}
	
	/**
	 * Determines possible types for the target.
	 * 
	 * @param target a TREC target
	 */
	private static void determineTypes(TRECTarget target) {
		String targetDesc = target.getTargetDesc();
		
		// possible target types
		ArrayList<String> types = new ArrayList<String>();
		for (String type : TRECTarget.TARGET_TYPES) types.add(type);
		
		// not a noun phrase -> EVENT
		if (!target.isNounPhrase()) {
			types.remove("PERSON");
			types.remove("ORGANISATION");
			types.remove("THING");
		}
		
//		// some uper case words and finally a lower case word -> NO PERSON
//		if (targetDesc.matches("([A-Z]([a-z])+\\b){2}([a-z])+"))
//			types.remove("PERSON");
		
		// acronym -> ORGANISATION, EVENT or THING
		if (targetDesc.matches(".*([A-Z][a-z0-9\\.&]*){2,}.*"))
			types.remove("PERSON");
		
		// year date -> EVENT
		if (targetDesc.matches(".*\\b\\d{4,4}\\b.*")) {
			types.remove("PERSON");
			types.remove("ORGANISATION");
			types.remove("THING");
		}
		
		// one lower case word -> THING
		if (targetDesc.matches("[a-z]+")) {
			types.remove("PERSON");
			types.remove("ORGANISATION");
			types.remove("EVENT");
		}
		
		// Sir, Prof., ... -> PERSON
		// TODO include titles and name lists from GATE
		if (targetDesc.matches("(?i).*\\b" +
			"(Doctor|Dr\\.|Junior|Jr\\.|Miss|Ms\\.|Misses|Mrs\\.|Mister|Mr\\." +
			"|Prof\\.|Professor|Sir|Sr\\.)" +
			"\\b.*"))
		{
			types.remove("ORGANISATION");
			types.remove("EVENT");
			types.remove("THING");
		}
		
		// Corporation, Inc., ... -> ORGANIZATION
		if (targetDesc.matches("(?i).*\\b" +
			"(administration|agenc(y|ies)|association|authorit(y|ies)|bank" +
			"|board|brotherhood|bureau|center|centre|church|clinic|club" +
			"|college|commission|committee|communit(y|ies)|corp\\." +
			"|corporation|council|department|directorate|division|federation" +
			"|foundation|fund|group|guild|hospital|hotel|inc\\.|incorporated" +
			"|institute|lab|laboratory(ies)|ministr(y|ies)|museum|office" +
			"|school|societ(y|ies)|squadron|syndicate|universit(y|ies)|union)" +
			"(e?s)?\\b.*"))
		{
			types.remove("PERSON");
			types.remove("EVENT");
			types.remove("THING");
		}
		
		// Show, Conference, ... -> EVENT
		// TODO think of more, go through TREC targets
		if (targetDesc.matches("(?i).*\\b" +
			"(championship|conference|desaster|cup|show|tournament|tradegy" +
			"|workshop)" +
			"(e?s)?\\b.*"))
		{
			types.remove("PERSON");
			types.remove("ORGANISATION");
			types.remove("THING");
		}
		
		// update target datastructure
		if (types.size() > 0)
			target.setTargetTypes(types.toArray(new String[types.size()]));
	}
	
	/**
	 * Preprocesses the target (normalization and type determination).
	 * 
	 * @param target a TREC target
	 */
	public static void preprocess(TRECTarget target) {
		normalize(target);
		determineTypes(target);
	}
}

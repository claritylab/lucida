package info.ephyra.questionanalysis;

import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.semantics.ASSERT;
import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.util.RegexConverter;

import java.text.ParseException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Extracts predicate-argument structures from a question. At first, the question string is transformed into a statement
 * with a dummy argument, then the predicates are extracted and finally the dummy argument is dropped.
 * 
 * @author Nico Schlaefer
 * @version 2007-04-25
 */
public class PredicateExtractor {
	/** Pattern that matches any form of 'to be'. */
	private static final String BE_P = "(?i)\\b(be|is|are|was|were|been)\\b";
	/** Pattern that matches any form of 'to do'. */
	private static final String DO_P = "(?i)\\b(do|does|did|done)\\b";
	/** Pattern that matches any form of 'to have'. */
	private static final String HAVE_P = "(?i)\\b(have|has|had)\\b";
	/** Pattern that matches other verb forms which should not be considered as predicates. */
	private static final String IGNORE_P = "(?i)\\b(name|give|tell|list)\\b";
	/** Pattern that matches any interrogative or 'that'. */
	private static final String INTERROGATIVE_P = "(?i)\\b(who(m|se)?|what|which|when|where|why|how|that)\\b";
	
	/** Pattern for questions seeking a person. */
	private static final String PERSON_P = "(?i)\\bwho(m|se)?\\b";
	/** Pattern for questions seeking a thing. */
	private static final String THING_P = "(?i)\\b(what|which)\\b";
	/** Pattern for questions seeking a date/time. */
	private static final String DATE_TIME_P = "(?i)\\bwhen\\b";
	/** Pattern for questions seeking a location. */
	private static final String LOCATION_P = "(?i)\\bwhere\\b";
	/** Pattern for questions seeking a purpose. */
	private static final String PURPOSE_P = "(?i)\\bwhy\\b";
	/** Pattern for questions seeking a manner. */
	private static final String MANNER_P = "(?i)\\bhow\\b";
	
	/** Replacement if question is seeking a person. */
	private static final String PERSON_R = "a PERSON";
	/** Replacement if question is seeking a thing. */
	private static final String THING_R = "a THING";
	/** Replacement if question is seeking a date/time. */
	private static final String DATE_TIME_R = "in 1999";
	/** Replacement if question is seeking a location. */
	private static final String DURATION_R = "for one HOUR";
	/** Replacement if question is seeking a location. */
	private static final String LOCATION_R = "in AMERICA";
	/** Replacement if question is seeking a purpose. */
	private static final String PURPOSE_R = "for PURPOSE";
	/** Replacement if question is seeking a manner. */
	private static final String MANNER_R = "with MANNER";
	/** Replacement if question is seeking a quantification. */
	private static final String QUANTIFICATION_R = "NOT";
	/** Replacement if question is seeking an entity of an unknown type. */
	private static final String UNKNOWN_R = "a BIG";
	
	/**
	 * Checks if the question contains a predicate that can be labeled.
	 * 
	 * @param qn normalized question string
	 * @return <code>true</code> iff the question contains a predicate
	 */
	private static boolean containsPredicate(String qn) {
		// tag POS and phrase chunks
		String[] tokens = OpenNLP.tokenize(qn);
		String[] pos = OpenNLP.tagPos(tokens);
		String[] chunks = OpenNLP.tagChunks(tokens, pos);
		
		// check if there is a verb other than 'to be', 'to do' or 'to have' which is not on the ignore list
		for (int i = 0; i < tokens.length; i++)
			if ((pos[i].startsWith("VB") || chunks[i].endsWith("-VP")) &&
					!(tokens[i].matches(BE_P) || tokens[i].matches(DO_P) || tokens[i].matches(HAVE_P) ||
							tokens[i].matches(IGNORE_P)))
				return true;
		
		// check if there is a verb phrase that ends in a verb other than 'to be'
//		String lastToken = "";
//		boolean lastTokenVerbPhrase = false;  // last token was part of a verb phrase
//		for (int i = 0; i < tokens.length; i++) {
//			if (pos[i].startsWith("VB") || chunks[i].endsWith("-VP")) {
//				lastToken = tokens[i];
//				lastTokenVerbPhrase = true;
//			} else {
//				if (lastTokenVerbPhrase)
//					if (!lastToken.matches(BE_P) && !lastToken.matches(IGNORE_P)) return true;
//				lastTokenVerbPhrase = false;
//			}
//		}
//		if (lastTokenVerbPhrase)
//			if (!lastToken.matches(BE_P) && !lastToken.matches(IGNORE_P)) return true;
		
		return false;
	}
	
	/**
	 * Transforms a phrase into a regular expression that matches the phrase, allowing differences regarding
	 * whitespaces and punctuation and quotation marks.
	 * 
	 * @param phrase phrase to transform
	 * @return regular expression
	 */
	private static String phraseToRegex(String phrase) {
		// transform into a regular expression
		phrase = RegexConverter.strToRegex(phrase);
		// allow an arbitrary number of whitespaces
		phrase = phrase.replace(" ", "\\s*+");
		// make punctuation marks optional
		phrase = phrase.replaceAll("(\\.|\\?|!|\")", ".?");
		
		return phrase;
	}
	
	// methods to replace phrases with interrogatives
	
	private static String handleIgnore(String qn, String verbMod, String[] tokens, String[] pos, String[] chunks,
			int i) {
		// get phrase from word on ignore-list to next interrogative
		String phrase = tokens[i];
		int interrogative = 0;
		for (int j = i + 1; j < tokens.length; j++) {
			phrase += " " + tokens[j];
			if (tokens[j].matches(INTERROGATIVE_P)) {
				interrogative = j;
				break;
			}
		}
		
		if (interrogative > i + 1) {
			// is the interrogative followed by a verb?
			boolean verb = (interrogative + 1 < tokens.length &&
					(pos[interrogative + 1].startsWith("VB") || chunks[interrogative + 1].endsWith("-VP")))
					? true : false;
			
			// replace phrase
			phrase = phraseToRegex(phrase);
			Matcher m = Pattern.compile(phrase).matcher(qn);
			if (m.find()) {
				String replacement =
					m.group(0).replaceFirst(IGNORE_P, UNKNOWN_R).replaceFirst(INTERROGATIVE_P, "");
				if (verb) {
					verbMod = verbMod.replaceFirst(phrase, replacement);
				} else {
					verbMod = verbMod.replaceFirst(phrase, "") + " " + replacement;
				}
			}
		}
		return verbMod;
	}
	
	private static String handlePerson(String verbMod) {
		verbMod = verbMod.replaceFirst(PERSON_P, PERSON_R);
		return verbMod;
	}
	
	private static String handleThing(String qn, String verbMod, String[] ats, String[] tokens, String[] pos,
			String[] chunks, int i) {
		if (i + 1 == tokens.length || !chunks[i + 1].endsWith("-NP")) {
			// interrogative is not followed by a noun phrase
			
			// is the interrogative followed by an auxiliary verb that has been shifted in verbMod?
			boolean auxiliary =
				(i + 2 < tokens.length &&
						!verbMod.matches(".*?" + phraseToRegex(tokens[i + 1] + " " + tokens[i + 2]) + ".*+"))
				? true : false;
			
			// replace interrogative
			if (auxiliary)
				verbMod = verbMod.replaceFirst(THING_P, "") + " " + THING_R;
			else
				verbMod = verbMod.replaceFirst(THING_P, THING_R);
		} else {
			// interrogative is followed by noun phrases...
			
			// get interrogative + noun phrases
			String phrase = tokens[i];
			int j;
			for (j = i + 1; j < tokens.length; j++)
				if (!pos[j].startsWith("VB") && !chunks[j].endsWith("-VP") &&
						!(j == tokens.length - 1 && pos[j].equals(".")))
					phrase += " " + tokens[j];
				else break;
			
			if (i == 0 || !chunks[i - 1].endsWith("-PP")) {
				// ...and not preceded by prepositions
				
				// replace phrase
				phrase = phraseToRegex(phrase);
				// special handling for certain answer types
				boolean replaced = false;
				for (String at : ats)
					if (at.startsWith("NEdate") || at.startsWith("NEtime")) {
						verbMod = verbMod.replaceFirst(phrase, "") + " " + DATE_TIME_R;
						replaced = true;
						break;
					} else if (at.startsWith("NElocation")) {
						verbMod = verbMod.replaceFirst(phrase, "") + " " + LOCATION_R;
						replaced = true;
						break;
					}
				// general case
				if (!replaced) {
					Matcher m = Pattern.compile(phrase).matcher(qn);
					if (m.find()) {
						// is the phrase followed by an auxiliary verb that has been shifted in verbMod?
						boolean auxiliary =
							(j + 1 < tokens.length &&
									!verbMod.matches(".*?" + phraseToRegex(tokens[j] + " " + tokens[j + 1]) + ".*+"))
							? true : false;
						
						String replacement = m.group(0).replaceFirst(THING_P, UNKNOWN_R);
						if (auxiliary)
							verbMod = verbMod.replaceFirst(phrase, "") + " " + replacement;
						else
							verbMod = verbMod.replaceFirst(phrase, replacement);
					}
				}
			} else {
				// ...and preceded by prepositions
				
				// get prepositions + interrogative + noun phrases
				for (j = i - 1; j >= 0; j--)
					if (chunks[j].endsWith("-PP"))
						phrase = tokens[j] + " " + phrase;
					else break;
				
				// replace phrase
				phrase = phraseToRegex(phrase);
				// special handling for certain answer types
				boolean replaced = false;
				for (String at : ats)
					if (at.startsWith("NEdate") || at.startsWith("NEtime")) {
						verbMod = verbMod.replaceFirst(phrase, "") + " " + DATE_TIME_R;
						replaced = true;
						break;
					} else if (at.startsWith("NElocation")) {
						verbMod = verbMod.replaceFirst(phrase, "") + " " + LOCATION_R;
						replaced = true;
						break;
					}
				// general case
				if (!replaced) {
					Matcher m = Pattern.compile(phrase).matcher(qn);
					if (m.find()) {
						String replacement = m.group(0).replaceFirst(THING_P, UNKNOWN_R);
						verbMod = verbMod.replaceFirst(phrase, "") + " " + replacement;
					}
				}
			}
		}
		return verbMod;
	}
	
	private static String handleDateTime(String verbMod) {
		verbMod = verbMod.replaceFirst(DATE_TIME_P, "") + " " + DATE_TIME_R;
		return verbMod;
	}
	
	private static String handleLocation(String verbMod) {
		verbMod = verbMod.replaceFirst(LOCATION_P, "") + " " + LOCATION_R;
		return verbMod;
	}
	
	private static String handlePurpose(String verbMod) {
		verbMod = verbMod.replaceFirst(PURPOSE_P, "") + " " + PURPOSE_R;
		return verbMod;
	}
	
	private static String handleManner(String qn, String verbMod, String[] ats, String[] tokens, String[] pos,
			String[] chunks, int i) {
		if (i + 1 == tokens.length || !(pos[i + 1].matches("JJ") || pos[i + 1].matches("RB"))) {
			// interrogative is not followed by an adjective
			
			verbMod = verbMod.replaceFirst(MANNER_P, "") + " " + MANNER_R;
		} else {
			// interrogative is followed by an adjective
			
			// get interrogative + adjective + noun phrases
			String phrase = tokens[i];
			int j;
			for (j = i + 1; j < tokens.length; j++)
				if (!pos[j].startsWith("VB") && !chunks[j].endsWith("-VP") &&
						!(j == tokens.length - 1 && pos[j].equals(".")))
					phrase += " " + tokens[j];
				else break;
			
			// replace phrase
			phrase = phraseToRegex(phrase);
			// special handling for answer type 'duration'
			boolean replaced = false;
			for (String at : ats)
				if (at.startsWith("NEduration")) {
					verbMod = verbMod.replaceFirst(phrase, "") + " " + DURATION_R;
					replaced = true;
					break;
				}
			// general case
			if (!replaced) {
				Matcher m = Pattern.compile(phrase).matcher(qn);
				if (m.find()) {
					String replacement = m.group(0).replaceFirst(MANNER_P, QUANTIFICATION_R);
					
					// is the phrase followed by an auxiliary verb that has been shifted in verbMod?
					boolean auxiliary =
						(j + 1 < tokens.length &&
								!verbMod.matches(".*?" + phraseToRegex(tokens[j] + " " + tokens[j + 1]) + ".*+"))
						? true : false;
					
					if (auxiliary) {
						verbMod = verbMod.replaceFirst(phrase, "") + " " + replacement;
					} else
						verbMod = verbMod.replaceFirst(phrase, replacement);
				}
			}
		}
		return verbMod;
	}
	
	/**
	 * Transforms a question into a statement by replacing phrases with interrogatives.
	 * 
	 * @param qn normalized question string
	 * @param verbMod question string with modified verbs
	 * @param ats expected answer types
	 * @return statement
	 */
	private static String questionToStatement(String qn, String verbMod, String[] ats) {
		String[] tokens = OpenNLP.tokenize(qn);
		String[] pos = OpenNLP.tagPos(tokens);
		String[] chunks = OpenNLP.tagChunks(tokens, pos);
		
		for (int i = 0; i < tokens.length; i++)
			if (tokens[i].matches(IGNORE_P)) {
				verbMod = handleIgnore(qn, verbMod, tokens, pos, chunks, i);
				break;
			} else if (tokens[i].matches(PERSON_P)) {
				verbMod = handlePerson(verbMod);
				break;
			} else if (tokens[i].matches(THING_P)) {
				verbMod = handleThing(qn, verbMod, ats, tokens, pos, chunks, i);
				break;
			} else if (tokens[i].matches(DATE_TIME_P)) {
				verbMod = handleDateTime(verbMod);
				break;
			} else if (tokens[i].matches(LOCATION_P)) {
				verbMod = handleLocation(verbMod);
				break;
			} else if (tokens[i].matches(PURPOSE_P)) {
				verbMod = handlePurpose(verbMod);
				break;
			} else if (tokens[i].matches(MANNER_P)) {
				verbMod = handleManner(qn, verbMod, ats, tokens, pos, chunks, i);
				break;
			}
		
		verbMod = verbMod.replaceAll("\\s++", " ").trim();  // drop unnecessary whitespaces
		return verbMod;
	}
	
	/**
	 * Extracts the predicates from a question string.
	 * 
	 * @param qn normalized question string
	 * @param verbMod question string with modified verbs
	 * @param ats expected answer types
	 * @param terms question terms
	 * @return predicate-argument structures
	 */
	public static Predicate[] getPredicates(String qn, String verbMod, String[] ats, Term[] terms) {
		// check if question contains a predicate
		if (!containsPredicate(qn)) return new Predicate[0];
		
		// transform question into statement
		String statement = questionToStatement(qn, verbMod, ats);
		
		// annotate and extract predicates
		String[][] ass = ASSERT.annotatePredicates(new String[] {statement});
		String[] as = (ass.length > 0) ? ass[0] : new String[0];
		List<Predicate> predicates = new ArrayList<Predicate>();
		for (int i = 0; i < as.length; i++) {
			// build predicate
			Predicate predicate = null;
			try {
				predicate = new Predicate(statement, as[i], terms);
			} catch (ParseException e) {
//				MsgPrinter.printErrorMsg(e.getMessage());
//				System.exit(1);
				continue;
			}
			predicates.add(predicate);
		}
		
		// drop placeholders
		boolean missingArgs = false;
		for (Predicate p : predicates) {
			if (p.dropArgs(PERSON_R) |
					p.dropArgs(THING_R) |
					p.dropArgs(DATE_TIME_R) |
					p.dropArgs(DURATION_R) |
					p.dropArgs(LOCATION_R) |
					p.dropArgs(PURPOSE_R) |
					p.dropArgs(MANNER_R) |
					p.dropArgs(QUANTIFICATION_R) |
					p.dropArgs(UNKNOWN_R))
				missingArgs = true;
		}
		
		// only return predicates if at least one has a missing argument
		// (else the answer extraction does not work)
		return (missingArgs)
			? predicates.toArray(new Predicate[predicates.size()])
			: new Predicate[0];
	}
}

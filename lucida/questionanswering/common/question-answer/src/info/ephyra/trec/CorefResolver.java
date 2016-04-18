package info.ephyra.trec;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.StanfordParser;
import info.ephyra.questionanalysis.QuestionNormalizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This class resolves references within a question string to the target
 * description, previous questions or previous answers as required since the
 * TREC 13 QA track.
 * 
 * @author Petra Gieselmann, Nico Schlaefer, Manas Pathak
 * @version 2007-07-20
 */
public class CorefResolver {

	/**
	 * Regular expression for English singular third person personal pronouns
	 * for persons.
	 */
	public static final String singularThirdPersonPronounString = "((?i)(\\bhe\\b)|(\\bshe\\b)|(\\bhim\\b))";

	public static final Pattern singularThirdPersonPronounPattern = Pattern
			.compile("(.* )?".concat(singularThirdPersonPronounString).concat(
					"(.*)?"));

	/**
	 * Regular expression for English singular third person personal pronouns
	 * for things.
	 */
	public static final String singularThirdThingPronounString = "((?i)(\\bit\\b))";

	public static final Pattern singularThirdThingPronounPattern = Pattern
			.compile("(.* )?".concat(singularThirdThingPronounString).concat(
					"(.*)?"));

	/** Regular expression for English plural third person personal pronouns. */
	public static final String pluralThirdPersonPronounString = "((?i)(\\bthey\\b)|(\\bthem\\b))";

	public static final Pattern pluralThirdPersonPronounPattern = Pattern
			.compile("(.* )?".concat(pluralThirdPersonPronounString).concat(
					"(.*)?"));

	/**
	 * Regular expression for English singular third person possessive pronouns
	 * for persons.
	 */
	public static final String singularThirdPersonPronounStringGen = "((?i)(\\bhis\\b)|(\\bhers\\b))";

	public static final Pattern singularThirdPersonPronounPatternGen = Pattern
			.compile("(.* )?".concat(singularThirdPersonPronounStringGen)
					.concat("(.*)?"));

	/**
	 * Regular expression for English singular third person possessive pronouns
	 * for things.
	 */
	public static final String singularThirdThingPronounStringGen = "((?i)(\\bits\\b))";

	public static final Pattern singularThirdThingPronounPatternGen = Pattern
			.compile("(.* )?".concat(singularThirdThingPronounStringGen)
					.concat("(.*)?"));

	/** Regular expression for English plural third person possessive pronouns. */
	public static final String pluralThirdPersonPronounStringGen = "((?i)(\\btheir\\b)|(\\btheirs\\b))";

	public static final Pattern pluralThirdPersonPronounPatternGen = Pattern
			.compile("(.* )?".concat(pluralThirdPersonPronounStringGen).concat(
					"(.*)?"));

	/**
	 * Regular expression for English singular third person personal and
	 * possessive pronoun her.
	 */
	public static final String singularThirdPersonPronounStringAmb = "((?i)(\\bher\\b))";

	public static final Pattern singularThirdPersonPronounPatternAmb = Pattern
			.compile("(.* )?".concat(singularThirdPersonPronounStringAmb)
					.concat("(.*)?"));

	/** Regular expression for English singular demonstrative pronoun. */
	public static final String singularDemPronounString = "((?i)(\\bthis\\b))";

	public static final Pattern singularDemPronounPattern = Pattern
			.compile("(.* )?".concat(singularDemPronounString).concat("(.*)?"));

	/** Regular expression for English singular demonstrative pronoun. */
	public static final String pluralDemPronounString = "((?i)(\\bthose\\b)|(\\bthese\\b))";

	public static final Pattern pluralDemPronounPattern = Pattern
			.compile("(.* )?".concat(pluralDemPronounString).concat("(.*)?"));


	/** Regular expression for targets. */
	public static final String verifyTargetString = "[a-zA-Z\\s]+";

	public static final Pattern verifyTargetPattern = Pattern
			.compile(verifyTargetString);//.concat("(.*)?"));

	/**
	 * Resolves references ONLY to the target description. This method is called
	 * once for each factoid and list question in the series.
	 * 
	 * @param target
	 *            the question series including answers to previous questions
	 * @param next
	 *            the next question in the series to be answered
	 */
	public static void resolvePronounsToTarget(TRECTarget target, int next) {
		String currentTarget = target.getCondensedTarget();
		TRECQuestion[] questions = target.getQuestions();
		String currentQuestionString = questions[next].getQuestionString();

		String temp = isTargetPerson(currentTarget);
		boolean personFlag = temp != null;
		
		String currentTargetPerson = currentTarget;
		if (personFlag) {
			currentTargetPerson = temp;
		}
				
		// genitive of current Target
		String currentTargetGen = null;
		String currentTargetPersonGen = null;
		// rest of the sentence after pronoun occured
		String rest = null;
		// tokenized target
		String[] tokens = OpenNLP.tokenize(currentTarget);
		
		// create genitive of currentTarget
		if (currentTarget.endsWith("s")) {
			currentTargetGen = currentTarget.concat("'");
		} else {
			currentTargetGen = currentTarget.concat("'s");
		}
		
		// create genitive of currentTargetPerson
		if (currentTargetPerson.endsWith("s")) {
			currentTargetPersonGen = currentTargetPerson.concat("'");
		} else {
			currentTargetPersonGen = currentTargetPerson.concat("'s");
		}

	
//		Collection<String> nplist = find(parse(currentTargetGen), "NP").values();
//		System.out.println("-->" + nplist + ": " + nplist.size());
		
		
//		if (nplist.size() > 1) {
//			return;
//		}
//		
//		String max = currentTargetGen;
//		
//		for (String s : nplist) {
//			String curr = unparse(s);
//			 
//			 if (curr.length() < max.length()) {
//				 max = curr;
//			 }
//		}
//		
//		currentTargetGen = max;
			
		/*
		 * Resolve personal, possessive and demonstrative pronouns by the target
		 * as antecedent
		 */
		
		String firstPronoun = "";
		int firstIndex = Integer.MAX_VALUE;
		
//		Matcher sgpers = singularThirdPersonPronounPattern
//				.matcher(currentQuestionString);
		
		String[] splitSgpers = currentQuestionString.split(singularThirdPersonPronounString);
		int firstSgpers = splitSgpers[0].length();
		if (splitSgpers.length > 1 && firstSgpers < firstIndex) {
			firstPronoun = "sgpers";
			firstIndex = firstSgpers;
		}
		
//		Matcher sgthing = singularThirdThingPronounPattern
//				.matcher(currentQuestionString);
		
		String[] splitSgthing = currentQuestionString.split(singularThirdThingPronounString);
		int firstSgthing = splitSgthing[0].length();
		if (splitSgthing.length > 1 && firstSgthing < firstIndex) {
			firstPronoun = "sgthing";
			firstIndex = firstSgthing;
		}
		
//		Matcher plpers = pluralThirdPersonPronounPattern
//				.matcher(currentQuestionString);
		String[] splitPlpers = currentQuestionString.split(pluralThirdPersonPronounString);
		int firstPlpers = splitPlpers[0].length();
		if (splitPlpers.length > 1 && firstPlpers < firstIndex) {
			firstPronoun = "plpers";
			firstIndex = firstPlpers;
		}
		
//		Matcher sgposs = singularThirdPersonPronounPatternGen
//				.matcher(currentQuestionString);

		String[] splitSgposs = currentQuestionString.split(singularThirdPersonPronounStringGen);
		int firstSgposs = splitSgposs[0].length();
		if (splitSgposs.length > 1 && firstSgposs < firstIndex) {
			firstPronoun = "sgposs";
			firstIndex = firstSgposs;
		}

//		Matcher sgthingposs = singularThirdThingPronounPatternGen
//				.matcher(currentQuestionString);
		String[] splitSgthingposs = currentQuestionString.split(singularThirdThingPronounStringGen);
		int firstSgthingposs = splitSgthingposs[0].length();
		if (splitSgthingposs.length > 1 && firstSgthingposs < firstIndex) {
			firstPronoun = "sgthingposs";
			firstIndex = firstSgthingposs;
		}
		
//		Matcher plposs = pluralThirdPersonPronounPatternGen
//				.matcher(currentQuestionString);
		String[] splitPlposs = currentQuestionString.split(pluralThirdPersonPronounStringGen); 
		int firstPlposs = splitPlposs[0].length();
		if (splitPlposs.length > 1 && firstPlposs < firstIndex) {
			firstPronoun = "plposs";
			firstIndex = firstPlposs;
		}
		
		Matcher her = singularThirdPersonPronounPatternAmb
				.matcher(currentQuestionString);
		String[] splitHer = currentQuestionString.split(singularThirdPersonPronounStringAmb); 
		int firstHer = splitHer[0].length();
		if (splitHer.length > 1 && firstPlposs < firstIndex) {
			firstPronoun = "her";
			firstIndex = firstHer;
		}
		
		Matcher sgdem = singularDemPronounPattern
				.matcher(currentQuestionString);
		String[] splitSgdem = currentQuestionString.split(singularDemPronounString);
		int firstSgdem = splitSgdem[0].length();
		if (splitSgdem.length > 1 && firstSgdem < firstIndex) {
			firstPronoun = "sgdem";
			firstIndex = firstSgdem;
		}
		
		Matcher pldem = pluralDemPronounPattern.matcher(currentQuestionString);
		String[] splitPldem = currentQuestionString.split(pluralDemPronounString);
		int firstPldem = splitPldem[0].length();
		if (splitPldem.length > 1 && firstPldem < firstIndex) {
			firstPronoun = "pldem";
			firstIndex = firstPldem;
		}		
		
		//System.out.println("firstPronoun: +" + firstPronoun + "+");
		
		// Start replacing
		
		if (personFlag && firstPronoun.equals("sgposs")) {
			currentQuestionString = currentQuestionString.replaceFirst(
					singularThirdPersonPronounStringGen, currentTargetPersonGen);
		}
		
		if (firstPronoun.equals("sgthingposs")) {
			currentQuestionString = currentQuestionString.replaceFirst(
					singularThirdThingPronounStringGen, currentTargetGen);
		}
		
		if (firstPronoun.equals("plposs")) {
			currentQuestionString = currentQuestionString.replaceFirst(
					pluralThirdPersonPronounStringGen, currentTargetGen);
		}
		
		if (personFlag && firstPronoun.equals("her") && her.matches()) {
			rest = currentQuestionString.substring(
					currentQuestionString.indexOf(her.group(2))
							+ her.group(2).length() + 1).toLowerCase();
			String[] questionTokens = OpenNLP.tokenize(rest);
			String[] pos = OpenNLP.tagPos(questionTokens);
			// check whether her is used as possessive pronoun or as personal
			// pronoun
			if (pos[0].equalsIgnoreCase("NN")) {
				currentQuestionString = currentQuestionString.replaceFirst(
						singularThirdPersonPronounStringAmb, currentTargetPersonGen);
			} else {
				currentQuestionString = currentQuestionString.replaceFirst(
						singularThirdPersonPronounStringAmb, currentTargetPerson);
			}
		}
		
		if (firstPronoun.equals("sgdem") && sgdem.matches()) {
			// check whether target contains the same word as the rest of the
			// question string
			rest = currentQuestionString.substring(
					currentQuestionString.indexOf(sgdem.group(2))
							+ sgdem.group(2).length() + 1).toLowerCase();
			for (int i = 0; i < tokens.length; i++) {
				if (rest.contains(tokens[i].toLowerCase())) {
					currentQuestionString = currentQuestionString.replaceFirst(
							" " + tokens[i].toLowerCase() + "\\b", "");
					currentQuestionString = currentQuestionString.replaceFirst(
							"\\b" + tokens[i].toLowerCase() + " ", "");
					currentQuestionString = currentQuestionString.replaceFirst(
							singularDemPronounString, currentTarget);
				}
			}
			
			currentQuestionString = currentQuestionString.replaceFirst(
					singularDemPronounString, currentTargetGen);
		}
		
		if (firstPronoun.equals("pldem") && pldem.matches()) {
			// check whether target contains the same word as the rest of the
			// question string
			rest = currentQuestionString.substring(
					currentQuestionString.indexOf(pldem.group(2))
							+ pldem.group(2).length() + 1).toLowerCase();
			
			for (int i = 0; i < tokens.length; i++) {
				if (rest.contains(tokens[i].toLowerCase())) {
					currentQuestionString = currentQuestionString.replaceFirst(
							" " + tokens[i].toLowerCase() + "\\b", "");
					currentQuestionString = currentQuestionString.replaceFirst(
							"\\b" + tokens[i].toLowerCase() + " ", "");
					currentQuestionString = currentQuestionString.replaceFirst(
							pluralDemPronounString, currentTarget);
				}
			}
			currentQuestionString = currentQuestionString.replaceFirst(
					pluralDemPronounString, currentTargetGen);
		}
		
		if (personFlag && firstPronoun.equals("sgpers")) {
			currentQuestionString = currentQuestionString.replaceFirst(
					singularThirdPersonPronounString, currentTargetPerson);
		}
		
		if (firstPronoun.equals("sgthing")) {
			currentQuestionString = currentQuestionString.replaceFirst(
					singularThirdThingPronounString, currentTarget);
		}
		
		if (firstPronoun.equals("plpers")) {
			currentQuestionString = currentQuestionString.replaceFirst(
					pluralThirdPersonPronounString, currentTarget);
		}

		questions[next].setQuestionString(currentQuestionString);
		MsgPrinter.printResolvedQuestion(currentQuestionString);
	}

	private static String isTargetPerson(String currentTarget) {
		Matcher tgt = verifyTargetPattern.matcher(currentTarget);		
		if (!tgt.matches()) {		
			return null;
		}
		
		if (isAllUpper(currentTarget)) {
			return null;
		}
		
		String[] split = currentTarget.split("\\s+");
		
		int jc = 0;
		boolean flagUpper = true;
		for (String s : split) {
			char c = s.charAt(0);
			
			if (Character.isLowerCase(c)) {
				if (!flagUpper) {
					return null;
				}
				
				jc++;
			} else {
				flagUpper = false;
			}
		}
		
		if (flagUpper || jc > 1) {
			return null;
		}
		
		String temp = "";
		for (int i = jc; i < split.length; i++) {
			temp += " " + split[i];
		}
		
		return temp.substring(1);
	}

	private static boolean isAllUpper(String s) {
		for (int i = 0; i < s.length(); i++) {
			char c = s.charAt(i);
		
			if (Character.isLowerCase(c)) {
				return false;
			}
		}

		return true;
	}

	/**
	 * Resolves references to the target description, previous questions or
	 * answers. This method is called once for each factoid and list question in
	 * the series.
	 * 
	 * @param target
	 *            the question series including answers to previous questions
	 * @param next
	 *            the next question in the series to be answered
	 */

	public static void resolvePronouns(TRECTarget target, int next) {
		String currentTarget = target.getCondensedTarget();
		TRECQuestion[] questions = target.getQuestions();
		String currentQuestionString = questions[next].getQuestionString();
		// genitive of current Target
		String currentTargetGen = null;
		// rest of the sentence after pronoun occured
		String rest = null;
		// tokenized target
		String[] tokens = OpenNLP.tokenize(currentTarget);
		// expected answer type - 1: thing, 2: person
		int exp = 0;
		// is target a person?
		boolean targetPerson = false;
		// is target a thing? - Not used at the moment: Too many problems
		boolean targetThing = false;

		// create genitive of currentTarget
		if (currentTarget.endsWith("s")) {
			currentTargetGen = currentTarget.concat("'");
		} else {
			currentTargetGen = currentTarget.concat("'s");
		}
		// System.out.println("Target: "+currentTarget );
		// System.out.println(next+ "Original:"+currentQuestionString );

		String[] targetTypes = target.getTargetTypes();
		if ((targetTypes.length == 1) && (targetTypes[0] == "PERSON")) {
			targetPerson = true;
		}
		if ((targetTypes.length != 4)
				&& ((targetTypes[0] != "PERSON")
						|| (targetTypes[1] != "PERSON") || (targetTypes[2] != "PERSON"))) {
			targetThing = true;
		}

		/*
		 * Resolve personal, possessive and demonstrative pronouns by the target
		 * as antecedent
		 */
		Matcher sgpers = singularThirdPersonPronounPattern
				.matcher(currentQuestionString);
		Matcher sgthing = singularThirdThingPronounPattern
				.matcher(currentQuestionString);
		Matcher plpers = pluralThirdPersonPronounPattern
				.matcher(currentQuestionString);
		Matcher sgposs = singularThirdPersonPronounPatternGen
				.matcher(currentQuestionString);
		Matcher sgthingposs = singularThirdThingPronounPatternGen
				.matcher(currentQuestionString);
		Matcher plposs = pluralThirdPersonPronounPatternGen
				.matcher(currentQuestionString);
		Matcher her = singularThirdPersonPronounPatternAmb
				.matcher(currentQuestionString);
		Matcher sgdem = singularDemPronounPattern
				.matcher(currentQuestionString);
		Matcher pldem = pluralDemPronounPattern.matcher(currentQuestionString);

		if (sgposs.matches()) {
			exp = 2;
			// if targetType is a thing, do not use it
			if ((!targetThing) || (next == 0)) {
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdPersonPronounStringGen, currentTargetGen);
			} else {
				if (usePreviousAnswer(questions, next, exp) != null) {
					currentTarget = usePreviousAnswer(questions, next, exp);
				}
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdPersonPronounStringGen, currentTargetGen);
			}
		}
		if (sgthingposs.matches()) {
			exp = 1;
			// if targetType is a person, do not use it
			if ((!targetPerson) || (next == 0)) {
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdThingPronounStringGen, currentTargetGen);
			} else {
				if (usePreviousAnswer(questions, next, exp) != null) {
					currentTarget = usePreviousAnswer(questions, next, exp);
				}
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdThingPronounStringGen, currentTargetGen);
			}
		}
		if (plposs.matches()) {
			currentQuestionString = currentQuestionString.replaceAll(
					pluralThirdPersonPronounStringGen, currentTargetGen);
		}
		if (her.matches()) {
			rest = currentQuestionString.substring(
					currentQuestionString.indexOf(her.group(2))
							+ her.group(2).length() + 1).toLowerCase();
			String[] questionTokens = OpenNLP.tokenize(rest);
			String[] pos = OpenNLP.tagPos(questionTokens);
			// check whether her is used as possessive pronoun or as personal
			// pronoun
			if (pos[0].equalsIgnoreCase("NN")) {
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdPersonPronounStringAmb, currentTargetGen);
			} else {
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdPersonPronounStringAmb, currentTarget);
			}
		}
		if (sgdem.matches()) {
			// check whether target contains the same word as the rest of the
			// question string
			rest = currentQuestionString.substring(
					currentQuestionString.indexOf(sgdem.group(2))
							+ sgdem.group(2).length() + 1).toLowerCase();
			for (int i = 0; i < tokens.length; i++) {
				if (rest.contains(tokens[i].toLowerCase())) {
					currentQuestionString = currentQuestionString.replaceAll(
							" " + tokens[i].toLowerCase() + "\\b", "");
					currentQuestionString = currentQuestionString.replaceAll(
							"\\b" + tokens[i].toLowerCase() + " ", "");
					currentQuestionString = currentQuestionString.replaceAll(
							singularDemPronounString, currentTarget);
				}
			}
			currentQuestionString = currentQuestionString.replaceAll(
					singularDemPronounString, currentTargetGen);
		}
		if (pldem.matches()) {
			// check whether target contains the same word as the rest of the
			// question string
			rest = currentQuestionString.substring(
					currentQuestionString.indexOf(pldem.group(2))
							+ pldem.group(2).length() + 1).toLowerCase();
			for (int i = 0; i < tokens.length; i++) {
				if (rest.contains(tokens[i].toLowerCase())) {
					currentQuestionString = currentQuestionString.replaceAll(
							" " + tokens[i].toLowerCase() + "\\b", "");
					currentQuestionString = currentQuestionString.replaceAll(
							"\\b" + tokens[i].toLowerCase() + " ", "");
					currentQuestionString = currentQuestionString.replaceAll(
							pluralDemPronounString, currentTarget);
				}
			}
			currentQuestionString = currentQuestionString.replaceAll(
					pluralDemPronounString, currentTargetGen);
		}
		if (sgpers.matches()) {
			exp = 2;
			// check whether target has another numerus as the pronoun to be
			// replaced
			if (!(checkPl(tokens)) || (targetThing) || (next == 0)) {
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdPersonPronounString, currentTarget);
			} else {
				if (usePreviousAnswer(questions, next, exp) != null) {
					currentTarget = usePreviousAnswer(questions, next, exp);
				}
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdPersonPronounString, currentTarget);
			}
		}
		if (sgthing.matches()) {
			exp = 1;
			// check whether target has another numerus as the pronoun to be
			// replaced and is a person
			if (!(checkPl(tokens)) || (targetPerson) || (next == 0)) {
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdThingPronounString, currentTarget);
			} else {
				if (usePreviousAnswer(questions, next, exp) != null) {
					currentTarget = usePreviousAnswer(questions, next, exp);
				}
				currentQuestionString = currentQuestionString.replaceAll(
						singularThirdThingPronounString, currentTarget);
			}
		}
		if (plpers.matches()) {
			currentQuestionString = currentQuestionString.replaceAll(
					pluralThirdPersonPronounString, currentTarget);
		}
		questions[next].setQuestionString(currentQuestionString);
		// System.out.println(next+ "Replaced:" +
		// questions[next].getQuestionString());
		// System.out.println("#########################################################");
		MsgPrinter.printResolvedQuestion(questions[next].getQuestionString());
	}

	/**
	 * 
	 * @param target
	 * @param next
	 */
	public static void resolveNounPhrasesToTarget(TRECTarget target, int next) {
		String targetString = target.getCondensedTarget();
		TRECQuestion[] questionsArray = target.getQuestions();
		String question = questionsArray[next].getQuestionString();	
		
		ArrayList<String> temp = resoveNP(targetString, question);
		temp.add(question);		

		questionsArray[next].setQuestionString(temp.get(0));
		MsgPrinter.printResolvedQuestion(question);
	}
	
	/**
	 * Resolves questions given a target
	 * @param targetString target
	 * @param question question
	 * @return ArrayList<String> of resolved questions
	 */	
	private static ArrayList<String> resoveNP(String targetString, String question) {		
		String targetParse = parse(targetString);
		Map<Integer, String> nptarget = find(targetParse, "NP");
		ArrayList<String> temp = new ArrayList<String>();
		
		if (nptarget.size() > 1) {
			return temp;
		}
		
		String questionParse = parse(question);
		Map<Integer, String> npquestion = find(questionParse, "NP");
		
		String resolvedq;		
		
		for (Map.Entry<Integer, String> q : npquestion.entrySet()) {
			String npq = q.getValue();
			int iq = q.getKey();
			resolvedq = "";
			
			// Replace target nouns
			for (Map.Entry<Integer, String> t : nptarget.entrySet()) {
				String npt = t.getValue();
				
				if (npq.contains("'") && !npt.contains("'")) {
					npt = npt.substring(0, npt.length() - 2) + ") (POS '";
					if (unparse(npt).endsWith("s")) {
						npt += "))";
					} else {
						npt += "s))";
					}				 					
				}
				
				if (npq.contains("(DT the)") && !npt.contains("(DT the)")) {
					npq = npq.replaceFirst("\\(NP \\(DT the\\) ", "(NP ");
					iq += 9;
					npt = npt.replaceFirst("\\(NP ", "(NP (DT the) ");
				}
				
				if (match(npq, npt)) {
					resolvedq = unparse(substitute(questionParse, npt, npq.length(), iq));
					
					char c = question.charAt(question.length() - 1);
					resolvedq = resolvedq.substring(0, resolvedq.length() - 3) + c;
					
					resolvedq = resolvedq.replace("`` ", "\"");
					resolvedq = resolvedq.replace(" ''", "\"");				
					resolvedq = resolvedq.replace(" '", "'");				
					resolvedq = resolvedq.replace("-LRB- ", "(");
					resolvedq = resolvedq.replace(" -RRB-", ")");
					
//					System.out.println(resolvedq);
//					System.out.println("----------------------");
					
					if (!resolvedq.equalsIgnoreCase(question)) {
						temp.add(resolvedq);
					}
				}				
			}
		}
		
		return temp;
	}

	/**
	 * Returns the lexical parse of the string
	 */
	private static String parse(String q) {
	    return StanfordParser.parse(q);	    
	}

	/**
	 * Finds all POS instances in parse
	 * @param parse input parsed string
	 * @param POS part of speech tag
	 * @return A <code>Map</code> of POS tagged strings in input along with the position where they occur
	 */
	private static Map<Integer, String> find(String parse, String POS) {
		Map<Integer, String> map = new HashMap<Integer, String>();

		int i = parse.indexOf("(" + POS);
		while (i != -1) {
			// get NP from parse starting at i
			int count = -1;
			int j = i;
			String temp = "";
			
			do {
				char c = parse.charAt(j++);
				temp += c;
				
				if (c == '(') {
					count++;
				}
				
				if (c == ')') {
					count--;
				}
			} while (count != -1);
			
			map.put(i, temp);

			i = parse.indexOf("(" + POS, i + 1);
		}
		
		return map;
	}

	/**
	 * Substitutes a target noun phrase for another noun phrase within a question string
	 * @param questionParse question String
	 * @param npt target noun phrase
	 * @param lenq length of original noun phrase in question string
	 * @param iq position of original noun phrase in question string
	 * @return resolved question string
	 */
	private static String substitute(String questionParse, String npt, int lenq, int iq) {		
		String left = questionParse.substring(0, iq);
		String right = questionParse.substring(iq + lenq);
		
		questionParse = left + npt + right;
		
		return questionParse;
	}

	/**
	 * Gets the original string back from its parse
	 */
	private static String unparse(String questionParse) {
		String[] split = questionParse.split(" ");
		String temp = "";
		
		for (String s : split) {
			int i = s.indexOf(")");
			if (i > -1) {
				temp += s.substring(0, i) + " ";
			}
		}
		
		return temp;
	}

	/**
	 * Checks if the first phrase is inclusive of the second
	 * @param npq parsed string
	 * @param npt parsed string
	 */
	private static boolean match(String npq, String npt) {
		String q = unparse(npq).replace("'s", "").replace("'", "");
		String t = unparse(npt).replace("'s", "").replace("'", "");
		
		boolean exists;
		for (String token1 : q.split(" ")) {
			token1 = SnowballStemmer.stem(token1);
			
			exists = false;
			for (String token2 : t.split(" ")) {
				token2 = SnowballStemmer.stem(token2);

//				System.out.println(token1 + ":" + token2);
				
				if (token1.equalsIgnoreCase(token2)) {
					exists = true;
					break;
				}
			}
			
			if (!exists) {
				return false;
			}
		}
		
		return true;
	}

	/*
	 * check whether target is pl @param Tokens of the target String @return
	 * true iff it is pl else false
	 */
	private static boolean checkPl(String[] targetTokens) {
		if ((targetTokens.length == 1) && (targetTokens[0].endsWith("s"))) {
			return true;
		} else
			return false;
	}

	/*
	 * getAnswerType returns AnswerType of given question @param stemmed
	 * question string @return expected answer types
	 * 
	 */
	// private static String[] getAnswerType(String question){
	// String qn = QuestionNormalizer.normalize(question);
	// String stemmed = QuestionNormalizer.stemVerbsAndNouns(qn);
	// return AnswerTypeTester.getAnswerTypes(qn, stemmed);
	// }
	/*
	 * isAnswerTypePerson returns true if AnswerType is person, else false
	 * @param stemmed question string @return boolean
	 * 
	 */
	private static boolean isAnswerTypePerson(String question) {
		ArrayList<Pattern> patterns = new ArrayList<Pattern>();
		boolean f = false;
		String qn = QuestionNormalizer.normalize(question);
		String stemmed = QuestionNormalizer.stemVerbsAndNouns(qn);
		String[] tokens = new String[3];
		tokens[0] = "who";
		tokens[1] = "whom";
		tokens[2] = "(what|which|name) (.* )?(actor|actress|adventurer|architect|artist|assassin|aunt|author|boy|builder|chairman|chancellor|child|creator|dancer|daughter|designer|developer|dictator|discoverer|emperor|employee|enemy|explorer|father|founder|friend|girl|governor|graduate|guy|head|hostage|husband|individual|inventor|killer|leader|maker|man|member|minister|monarch|mother|murderer|musician|official|opponent|owner|partner|person|personnel|player|politician|president|recipient|ruler|scientist|secretary|sender|singer|slayer|son|student|terrorist|uncle|victim|wife|winner|witness|woman|writer)";
		for (int i = 0; i < tokens.length; i++) {
			patterns.add(Pattern.compile("\\b" + tokens[i] + "\\b",
					Pattern.CASE_INSENSITIVE));
		}
		for (int i = 0; i < tokens.length; i++) {
			Matcher m = patterns.get(i).matcher(stemmed);
			if (m.find()) {
				f = true;
			}
		}
		return f;
	}

	/*
	 * isAnswerTypeThing returns true if AnswerType is thing, else false @param
	 * stemmed question string @return boolean
	 * 
	 */
	private static boolean isAnswerTypeThing(String question) {
		ArrayList<Pattern> patterns = new ArrayList<Pattern>();
		boolean f = false;
		String qn = QuestionNormalizer.normalize(question);
		String stemmed = QuestionNormalizer.stemVerbsAndNouns(qn);
		String[] tokens = new String[1];
		tokens[0] = "(what|which)";
		for (int i = 0; i < tokens.length; i++) {
			patterns.add(Pattern.compile("\\b" + tokens[i] + "\\b",
					Pattern.CASE_INSENSITIVE));
		}
		for (int i = 0; i < tokens.length; i++) {
			Matcher m = patterns.get(i).matcher(stemmed);
			if (m.find()) {
				f = true;
			}
		}
		return f;
	}

	/*
	 * usePreviousAnswer returns Previous Answer @param questions @param next
	 * (current number) @param exp: expected answer type: 1: thing, 2: person
	 * @return previous answer
	 * 
	 */
	private static String usePreviousAnswer(TRECQuestion[] questions, int next,
			int exp) {
		// System.out.println("JETZT!!");
		// return null;
		if (questions.length == 0)
			return null;
		int i = next - 1;
		// TRECAnswer[] answers = questions[i].getAnswers();
		// while (i>=0) {
		// if ((i < answers.length) && (answers[i].getAnswerString()!= null)) {
		// if (((exp == 1) &&
		// (isAnswerTypeThing(questions[i].getQuestionString())))
		// || ((exp == 2) &&
		// (isAnswerTypePerson(questions[i].getQuestionString())))) {
		// return answers[i].getAnswerString();
		// }
		// }
		// i--;
		// }
		// use first answer of i-th question, not i-th answer of i-th question
		while (i >= 0) {
			TRECAnswer[] answers = questions[i].getAnswers();
			if ((answers.length != 0) && (answers[0].getAnswerString() != null)) {
				if (((exp == 1) && (isAnswerTypeThing(questions[i]
						.getQuestionString())))
						|| ((exp == 2) && (isAnswerTypePerson(questions[i]
								.getQuestionString())))) {
					return answers[0].getAnswerString();
				}
			}
			i--;
		}
		
		return null;
	}
	
	public static void main(String[] args) throws Exception {
		try {
//			StanfordParser.initialize();
			OpenNLP.createPosTagger("res/nlp/postagger/opennlp/tag.bin.gz",
									"res/nlp/postagger/opennlp/tagdict");
			OpenNLP.createTokenizer("res/nlp/tokenizer/opennlp/EnglishTok.bin.gz");
//			SnowballStemmer.create();
		} catch (Exception e) {
			e.printStackTrace();
		}				
		
		TRECTarget[] targets = TREC13To16Parser.loadTargets("res/testdata/trec/trec15questions.xml");
		
		for (TRECTarget target : targets) {
			TargetPreprocessor.preprocess(target);
			
			String t = target.getCondensedTarget();
			
			for (int i = 0; i < ("Target: " + t).length(); i++) {
				System.out.print("=");
			}			
			System.out.println();
			
			System.out.println("Target: " + t);
			for (int i = 0; i < ("Target: " + t).length(); i++) {
				System.out.print("=");
			}			
			System.out.println();			
			
			for (int i = 0; i < target.getQuestions().length; i++) {
				if (!target.getQuestions()[i].getQuestionString().equals("Other")) {
					System.out.println("Question:	" + target.getQuestions()[i].getQuestionString());
	
	//				CorefResolver.resolveNounPhrasesToTarget(target, i);					
					CorefResolver.resolvePronounsToTarget(target, i);
						
					System.out.println("Resolved:	" + target.getQuestions()[i].getQuestionString());
				}
			}
			
			System.out.println();
		}
	}
}

package info.ephyra.nlp;

import info.ephyra.io.MsgPrinter;
import info.ephyra.util.HashDictionary;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import opennlp.tools.namefind.NameFinderME;

/**
 * Applies regular expressions for named entity extraction.
 * 
 * @author Guido Sautter, Nico Schlaefer
 * @version 2008-02-10
 */
public class RegExMatcher {
	public static final String OTHER = NameFinderME.OTHER;
	public static final String START = NameFinderME.START;
	public static final String CONTINUE = NameFinderME.CONTINUE;
	
	private static final int MAX_TOKENS = 10;
	
	/**	mark all parts of a token sequence that match a regular expression
	 * @param	tokens			the token sequence to be rooted through
	 * @param	regEx			the regular expression that's matches are to be extracted
	 * @return an array of marker Strings marking all subsequences of the specified token sequence that match the specified regular expression
	 */
	public static String[] markAllMatches(String[] tokens, String regEx) {
		return markAllMatches(tokens, regEx, MAX_TOKENS);
	}
	
	/**	mark all parts of a token sequence that match a regular expression
	 * @param	tokens			the token sequence to be rooted through
	 * @param	pattern			the regular expression that's matches are to be extracted
	 * @return an array of marker Strings marking all subsequences of the specified token sequence that match the specified regular expression
	 */
	public static String[] markAllMatches(String[] tokens, Pattern pattern) {
		return markAllMatches(tokens, pattern, MAX_TOKENS);
	}
	
	/**	mark all parts of a token sequence that match a regular expression
	 * @param	tokens			the token sequence to be rooted through
	 * @param	regEx			the regular expression that's matches are to be extracted
	 * @param	maxTokens		the maximum number of tokens a matching part may contain (0 means no limit, Attention: high computation effort)
	 * @return an array of marker Strings marking all subsequences of the specified token sequence that match the specified regular expression
	 */
	public static String[] markAllMatches(String[] tokens, String regEx, int maxTokens) {
		return markAllMatches(tokens, Pattern.compile(regEx), maxTokens);
	}
	
	/**	mark all parts of a token sequence that match a regular expression
	 * @param	tokens			the token sequence to be rooted through
	 * @param	pattern			the pattern that's matches are to be extracted
	 * @param	maxTokens		the maximum number of tokens a matching part may contain (0 means no limit, Attention: high computation effort)
	 * @return an array of marker Strings marking all subsequences of the specified token sequence that match the specified regular expression
	 */
	public static String[] markAllMatches(String[] tokens, Pattern pattern, int maxTokens) {
		String[] markers = new String[tokens.length];
		int markerIndex = 0;
		
		int lastStartIndex = 0;
		int lastMatchedIndex = 0;
		int index = 0;
		StringBuffer actualPart = new StringBuffer();
		boolean foundMatch = false;
		
		while (index < tokens.length) {
			
			lastStartIndex = index;
			
			while ((index < tokens.length) && (((index - lastStartIndex) < maxTokens) || (maxTokens == 0))) {
				
				actualPart.append(((actualPart.length() == 0) ? "" : " ") + tokens[index]);
				String testPart = actualPart.toString();
				
				if (pattern.matcher(testPart).matches()) {
					foundMatch = true;
					lastMatchedIndex = index;
				}
				index++;
			}
			
			//	store longest match
			if (foundMatch) {
				
				//	fill in marker array up to start of actual match
				while (markerIndex < lastStartIndex) {
					markers[markerIndex] = OTHER;
					markerIndex ++;
				}
				
				//	mark start of match
				markers[markerIndex] = START;
				markerIndex++;
				
				//	mark rest of match
				while (markerIndex <= lastMatchedIndex) {
					markers[markerIndex] = CONTINUE;
					markerIndex ++;
				}
				
				index = lastMatchedIndex;
			} else {
				index = lastStartIndex;
			}
			
			//	tidy up
			foundMatch = false;
			actualPart.setLength(0);
			index ++;
		}
		
		//	fill in rest of marker array
		while (markerIndex < tokens.length) {
			markers[markerIndex] = OTHER;
			markerIndex ++;
		}
		
		return markers;
	}
	
	/**	extract all parts from a token sequence that match a regular expression
	 * @param	text			the token sequence to be rooted through
	 * @param	regEx			the regular expression that's matches are to be extracted
	 * @return an array containing all subsequences of the specified token sequence that match the specified regular expression
	 */
	public static String[] extractAllMatches(String text, String regEx) {
		return extractAllMatches(text, Pattern.compile(regEx));
	}
	
	/**	extract all parts from a token sequence that match a regular expression
	 * @param	text			the token sequence to be rooted through
	 * @param	pattern			the regular expression Pattern that's matches are to be extracted
	 * @return an array containing all subsequences of the specified token sequence that match the specified regular expression
	 */
	public static String[] extractAllMatches(String text, Pattern pattern) {
		Matcher matcher = pattern.matcher(text);
		ArrayList<String> matches = new ArrayList<String>();
		while (matcher.find()) {
			String match = matcher.group(0);
			matches.add(match);
		}
		return matches.toArray(new String[matches.size()]);
	}
	
	/**	load a gazetteer
	 * @param	name	the name of the list to be loaded
	 * @return the gazetteer with the specified name, packe in a HashSet for faste lookup 
	 */
	public static HashDictionary getDictionary(String name) {
		if (dictionariesByName.containsKey(name)) return dictionariesByName.get(name);
		
		HashDictionary dictionary = null;
		try {
			dictionary = new HashDictionary("./res/nlp/netagger/lists/" + name);
		} catch (IOException e) {
			MsgPrinter.printErrorMsg("File not found: " + name);
			dictionary = new HashDictionary();
		}
		dictionariesByName.put(name, dictionary);
		return dictionary;
	}
	
	//	register for lists already loaded
	private static HashMap<String, HashDictionary> dictionariesByName = new HashMap<String, HashDictionary>(); 
	
	/**	mark all parts of a String that are contained in a list of Strings
	 * @param	tokens		the token sequence to be rooted through
	 * @param	dictionary	the gazetteer containing the Strings to be found
	 * @return an array of marker Strings marking all subsequences of the specified token sequence that's String representation is contained in the specified list
	 */
	public static String[] markAllContained(String[] tokens, HashDictionary dictionary) {
		String[] markers = new String[tokens.length];
		int markerIndex = 0;
		
		int lastStartIndex = 0;
		int lastMatchedIndex = 0;
		int index = 0;
		String currentPart = null;
		boolean foundMatch = false;
		
		while (index < tokens.length) {
			lastStartIndex = index;
			
			//	find longest match
			while ((index < tokens.length) && dictionary.containsToken(tokens[index]) && (((index - lastStartIndex) < dictionary.getMaxTokens()))) {
				
				currentPart = ((currentPart == null) ? tokens[index] : (currentPart + " " + tokens[index]));
				if (dictionary.contains(currentPart)) {
					foundMatch = true;
					lastMatchedIndex = index;
				}
				index++;
			}
			
			//	store longest match
			if (foundMatch) {
				
				//	fill in marker array up to start of actual match
				while (markerIndex < lastStartIndex) {
					markers[markerIndex] = OTHER;
					markerIndex ++;
				}
				
				//	mark start of match
				markers[markerIndex] = START;
				markerIndex++;
				
				//	mark rest of match
				while (markerIndex <= lastMatchedIndex) {
					markers[markerIndex] = CONTINUE;
					markerIndex ++;
				}
				
				index = lastMatchedIndex;
			} else {
				index = lastStartIndex;
			}
			
			//	tidy up
			foundMatch = false;
			currentPart = null;
			index ++;
		}
		
		//	fill in rest of marker array
		while (markerIndex < tokens.length) {
			markers[markerIndex] = OTHER;
			markerIndex ++;
		}
		
		return markers;
	}
	
	/**	mark all parts of a String that are fuzzy-contained in a list of Strings
	 * @param	tokens		the token sequence to be rooted through
	 * @param	dictionary	the gazetteer containing the Strings to be found
	 * @param	threshold	the maximum editing distance for which a fuzzy lookup shall return true
	 * @return an array of marker Strings marking all subsequences of the specified token sequence that's String representation is contained in the specified list
	 */
	public static String[] markAllContained(String[] tokens, HashDictionary dictionary, int threshold) {
		if (threshold == 0) return markAllContained(tokens, dictionary);
		
		String[] markers = new String[tokens.length];
		int markerIndex = 0;
		
		int lastStartIndex = 0;
		int lastMatchedIndex = 0;
		int index = 0;
		String currentPart = null;
		boolean foundMatch = false;
		
		while (index < tokens.length) {
			lastStartIndex = index;
			
			//	find longest match
			while ((index < tokens.length) && dictionary.fuzzyContainsToken(tokens[index], threshold) && (((index - lastStartIndex) < dictionary.getMaxTokens()))) {
				
				currentPart = ((currentPart == null) ? tokens[index] : (currentPart + " " + tokens[index]));
				if (dictionary.fuzzyContains(currentPart, threshold)) {
					foundMatch = true;
					lastMatchedIndex = index;
				}
				index++;
			}
			
			//	store longest match
			if (foundMatch) {
				
				//	fill in marker array up to start of actual match
				while (markerIndex < lastStartIndex) {
					markers[markerIndex] = OTHER;
					markerIndex ++;
				}
				
				//	mark start of match
				markers[markerIndex] = START;
				markerIndex++;
				
				//	mark rest of match
				while (markerIndex <= lastMatchedIndex) {
					markers[markerIndex] = CONTINUE;
					markerIndex ++;
				}
				
				index = lastMatchedIndex;
			} else {
				index = lastStartIndex;
			}
			
			//	tidy up
			foundMatch = false;
			currentPart = null;
			index ++;
		}
		
		//	fill in rest of marker array
		while (markerIndex < tokens.length) {
			markers[markerIndex] = OTHER;
			markerIndex ++;
		}
		
		return markers;
	}
	
	/**	mark all parts of a String that are contained in a list of Strings
	 * @param	tokens		the token sequence to be rooted through
	 * @param	dictionary	the gazetteer containing the Strings to be found
	 * @return an array of marker Strings marking all subsequences of the specified token sequence that's String representation is contained in the specified list
	 */
	public static String[] extractAllContained(String[] tokens, HashDictionary dictionary) {
		
		//	tokenize text
		ArrayList<String> tokenList = new ArrayList<String>();
		for (int t = 0; t < tokens.length; t++)
			if (tokens[t].length() != 0) tokenList.add(tokens[t]);
		tokens = tokenList.toArray(new String[tokenList.size()]);
		
		ArrayList<String> matches = new ArrayList<String>();
		
		int lastStartIndex = 0;
		int lastMatchedIndex = 0;
		int index = 0;
		String currentPart = null;
		String match = null;
		
		while (index < tokens.length) {
			lastStartIndex = index;
			
			//	find longest match starting with actual token
			while ((index < tokens.length) && dictionary.containsToken(tokens[index]) && (((index - lastStartIndex) < dictionary.getMaxTokens()))) {
				
				currentPart = ((currentPart == null) ? tokens[index] : (currentPart + " " + tokens[index]));
				if (dictionary.contains(currentPart)) {
					match = currentPart;
					lastMatchedIndex = index;
				}
				index++;
			}
			
			//	store longest match
			if (match != null) {
				matches.add(match);
				index = lastMatchedIndex;
			} else {
				index = lastStartIndex;
			}
			
			//	tidy up
			currentPart = null;
			match = null;
			index ++;
		}
		
		return matches.toArray(new String[matches.size()]);
	}
	
	/**	mark all parts of a String that are fuzzy-contained in a list of Strings
	 * @param	tokens		the token sequence to be rooted through
	 * @param	dictionary	the gazetteer containing the Strings to be found
	 * @param	threshold	the maximum editing distance for which a fuzzy lookup shall return true
	 * @return an array of marker Strings marking all subsequences of the specified token sequence that's String representation is contained in the specified list
	 */
	public static String[] extractAllContained(String[] tokens, HashDictionary dictionary, int threshold) {
		if (threshold == 0) return extractAllContained(tokens, dictionary);
		
		//	tokenize text
		ArrayList<String> tokenList = new ArrayList<String>();
		for (int t = 0; t < tokens.length; t++)
			if (tokens[t].length() != 0) tokenList.add(tokens[t]);
		tokens = tokenList.toArray(new String[tokenList.size()]);
		
		ArrayList<String> matches = new ArrayList<String>();
		
		int lastStartIndex = 0;
		int lastMatchedIndex = 0;
		int index = 0;
		String currentPart = null;
		String match = null;
		
		while (index < tokens.length) {
			lastStartIndex = index;
			
			//	find longest match starting with actual token
			while ((index < tokens.length) && dictionary.fuzzyContainsToken(tokens[index], threshold) && (((index - lastStartIndex) < dictionary.getMaxTokens()))) {
				
				currentPart = ((currentPart == null) ? tokens[index] : (currentPart + " " + tokens[index]));
				if (dictionary.fuzzyContains(currentPart, threshold)) {
					match = currentPart;
					lastMatchedIndex = index;
				}
				index++;
			}
			
			//	store longest match
			if (match != null) {
				matches.add(match);
				index = lastMatchedIndex;
			} else {
				index = lastStartIndex;
			}
			
			//	tidy up
			currentPart = null;
			match = null;
			index ++;
		}
		
		return matches.toArray(new String[matches.size()]);
	}
	
	/**	a regular expression capturing group matching all cardinal numbers given in form of digits
	 */
	public static final String NUMBER = "([1-9][0-9]*+(\\s?+\\,\\s?+[0-9]+)*+(\\s?+\\.\\s?+[0-9]++)?)";
	
	/**	mark all numbers in a token sequence
	 * @param	tokens	the token sequence
	 * @return an array of marker Strings marking all numbers in the specified token sequence
	 */
	public static String[] extractNumbers(String[] tokens) {
		return markAllMatches(tokens, NUMBER_PATTERN, 10);
	}
	
	/**	mark all parts from a token sequence that match a regular expression
	 * @param	tokens				the token sequence to be rooted through
	 * @param	dimensionPattern	the pattern that's matches are to be extracted
	 * @param	maxTokens			the maximum number of tokens a matching part may contain (0 means no limit, Attention: high computation effort)
	 * @return an array of marker Strings marking all subsequences of the specified token sequence that match the specified regular expression
	 */
	public static String[] extractQuantities(String[] tokens, String[] numberMarkers, Pattern dimensionPattern, int maxTokens) {
		String[] markers = new String[tokens.length];
		int markerIndex = 0;
		
		int lastStartIndex = 0;
		int lastMatchedIndex = 0;
		int matchStartIndex = 0;
		int index = 0;
		StringBuffer actualPart = new StringBuffer();
		boolean foundMatch = false;
		
		while (index < tokens.length) {
			
			while ((index < tokens.length) && !START.equals(numberMarkers[index])) index++;
			
			lastStartIndex = index;
			
			while ((index < tokens.length) && !OTHER.equals(numberMarkers[index])) index++;
			
			matchStartIndex = index;
			
			while ((index < tokens.length) && ((index - matchStartIndex) < maxTokens)) {
				
				actualPart.append(((actualPart.length() == 0) ? "" : " ") + tokens[index]);
				String testPart = actualPart.toString();
				
				if (dimensionPattern.matcher(testPart).matches()) {
					foundMatch = true;
					lastMatchedIndex = index;
				}
				index++;
			}
			
			//	store longest match
			if (foundMatch) {
				
				//	fill in marker array up to start of actual match
				while (markerIndex < lastStartIndex) {
					markers[markerIndex] = OTHER;
					markerIndex ++;
				}
				
				//	mark start of match
				markers[markerIndex] = START;
				markerIndex++;
				
				//	mark rest of match
				while (markerIndex <= lastMatchedIndex) {
					markers[markerIndex] = CONTINUE;
					markerIndex ++;
				}
				
				index = lastMatchedIndex;
			} else {
				index = lastStartIndex;
			}
			
			//	tidy up
			foundMatch = false;
			actualPart.setLength(0);
			index ++;
		}
		
		//	fill in rest of marker array
		while (markerIndex < tokens.length) {
			markers[markerIndex] = OTHER;
			markerIndex ++;
		}
		
		return markers;
	}
	
	/**	a regular expression capturing group matching all ordinal numbers given in form of digits
	 */
	public static final String ORDINAL = "((([1-9][0-9]*(\\s?+\\,\\s?+[0-9]+)*(\\s?+\\,\\s?+)?)?(([02-9](1\\s?+st|2\\s?+nd|3\\s?+rd|[04-9]\\s?+th))|(1[0-9]\\s?+th)))|(1\\s?+st|2\\s?+nd|3\\s?+rd|[4-9]\\s?+th))";
	
	/**	mark all numbers in a token sequence
	 * @param	tokens	the token sequence
	 * @return an array of marker Strings marking all numbers in the specified token sequence
	 */
	public static String[] extractOrdinalNumbers(String[] tokens) {
		return markAllMatches(tokens, ORDINAL, 10);
	}
	
	/**	a regular expression capturing group matching all one digit cardinal numbers given in form of words
	 */
	public static final String NUMBER_ONE = "(one|two|three|four|five|six|seven|eight|nine)";
	
	/**	a regular expression capturing group matching all two digit cardinal numbers given in form of words whose first digit is one
	 */
	public static final String NUMBER_Xteen = "(ten|eleven|twelve|(thir|four|fif|six|seven|eigh|nine)teen)";
	
	/**	a regular expression capturing group matching all two digit cardinal numbers given in form of words
	 */
	public static final String NUMBER_TEN = "(((twenty|thirty|fourty|fifty|sixty|seventy|eighty|ninety)" + NUMBER_ONE + "?)|" + NUMBER_Xteen + ")";
	
	/**	a regular expression capturing group matching all three digit cardinal numbers given in form of words whose last two digits are zero
	 */
	public static final String NUMBER_HUNDRED = "(" + NUMBER_ONE + "?hundred)";
	
	/**	a regular expression capturing group matching all three digit cardinal numbers given in form of words whose last two digits are zero, and all four digit cardinal numbers given in form of words whose first digit is 1 and whose last two digits are zero
	 */
	public static final String NUMBER_HUNDRED_WITH_Xteen = "(((a\\s|one)?|" + NUMBER_ONE + "|" + NUMBER_Xteen + ")hundred)";
	
	/**	a regular expression capturing group matching all three digit cardinal numbers given in form of words
	 */
	public static final String NUMBER_TO_HUNDRED = "(" + NUMBER_HUNDRED + "?(" + NUMBER_ONE + "|" + NUMBER_TEN + ")?)";
	
	/**	a regular expression capturing group matching all four digit cardinal numbers given in form of words whose last three digits are zero
	 */
	public static final String NUMBER_THOUSAND = "(((a\\s|one)?|" + NUMBER_TO_HUNDRED + "?)thousand)";
	
	/**	a regular expression capturing group matching all up to six digit cardinal numbers given in form of words
	 */
	public static final String NUMBER_TO_THOUSAND = "((((a\\s|one)?|" + NUMBER_TO_HUNDRED + "?)thousand)?" + NUMBER_TO_HUNDRED + ")";
	
	/**	a regular expression capturing group matching all cardinal numbers involving 'million', 'billion' or 'trillion'
	 */
	public static final String NUMBER_Xillion = "(((a\\s|one)?\\s(m|b|tr)illion)|(" + NUMBER_TO_HUNDRED + "?\\s(m|b|tr)illion(s)?))";
	
	
	/**	a regular expression capturing group matching all one digit ordinal numbers given in form of words
	 */
	public static final String ORDINAL_ONE = "(first|second|third|fourth|fifth|sixth|seventh|eighth|ninth)";
	
	/**	a regular expression capturing group matching all two digit ordinal numbers given in form of words whose first digit is one
	 */
	public static final String ORDINAL_Xteen = "(tenth|eleventh|twelveth|(thir|four|fif|six|seven|eigh|nine)teenth)";
	
	/**	a regular expression capturing group matching all two digit ordinal numbers given in form of words
	 */
	public static final String ORDINAL_TEN = "(((twenty|thirty|fourty|fifty|sixty|seventy|eighty|ninety)(" + ORDINAL_ONE + "|th))|" + ORDINAL_Xteen + ")";
	
	/**	a regular expression capturing group matching all three digit ordinal numbers given in form of words whose last two digits are zero
	 */
	public static final String ORDINAL_HUNDRED = "(" + NUMBER_ONE + "?hundredth)";
	
	/**	a regular expression capturing group matching all three digit ordinal numbers given in form of words whose last two digits are zero, and all four digit cardinal numbers given in form of words whose first digit is 1 and whose last two digits are zero
	 */
	public static final String ORDINAL_HUNDRED_WITH_Xteen = "((" + NUMBER_ONE + "|" + NUMBER_Xteen + ")?hundredth)";
	
	/**	a regular expression capturing group matching all three digit ordinal numbers given in form of words
	 */
	public static final String ORDINAL_TO_HUNDRED = "((" + NUMBER_HUNDRED + "(" + ORDINAL_ONE + "|" + ORDINAL_TEN + "))|(" + ORDINAL_HUNDRED_WITH_Xteen + "))";
	
	/**	a regular expression capturing group matching all four digit ordinal numbers given in form of words whose last three digits are zero
	 */
	public static final String ORDINAL_THOUSAND = "(" + NUMBER_TO_HUNDRED + "?thousandth)";
	
	/**	a regular expression capturing group matching all up to six digit ordinal numbers given in form of words
	 */
	public static final String ORDINAL_TO_THOUSAND = "(" + NUMBER_TO_HUNDRED + "?thousand(th|" + ORDINAL_TO_HUNDRED + "))";
	
	
	/* *	a regular expression capturing group matching any one larger scale length unit (distance unit, in particular, '(kilo)meter' and 'mile')
	 */
	//public static final String DISTANCE_UNIT = "(((k)?m)|mi|((kilo)?meter(s)?)|(mile(s)?))";
	
	/**	a regular expression capturing group matching any one length unit (in particular, '(kilo|deci|cent|milli|micro|nano)meter', 'foot', 'inch', and 'yard')
	 */
	public static final String LENGTH_UNIT = "(((k|d|c|m|μ|n)?m\\b)|ft|yd|(mi\\b)|((kilo|deci|centi|milli|micro|nano)?meter(s)?)|(foot|feet)|(inch(es)?)|(yard(s)?)|(mile(s)?))";
	
	/**	a regular expression capturing group matching any one smaller scale length unit (this constant is equal to LENGHT_UNIT, it is only provided for clarity)
	 */
	public static final String HEIGHT_UNIT = LENGTH_UNIT;
	
	/* *	a regular expression capturing group matching any one distance (in particular, a number followed by a larger scale length unit)
	 */
	//public static final String DISTANCE = "(" + NUMBER + "\\s" + DISTANCE_UNIT + ")";
	
	/**	a regular expression capturing group matching any one length (in particular, a number followed by a length unit)
	 */
	public static final String LENGTH = "(" + NUMBER + "\\s" + LENGTH_UNIT + ")";
	
	/**	a regular expression capturing group matching any one height (this constant is equal to LENGHT, it is only provided for clarity)
	 */
	public static final String HEIGHT = LENGTH;
	
	
	/**	a regular expression capturing group matching any one US Dollar unit (in particular, '$', 'USD', and 'Dollar')
	 */
	public static final String DOLLAR_UNIT = "(\\$|USD|(Dollar(s)?))";
	
	/**	a regular expression capturing group matching any one Euro unit (in particular, '€', 'EURO', and 'Euro')
	 */
	public static final String EURO_UNIT = "(\\€|EURO|(Euro(s)?))";
	
	/**	a regular expression capturing group matching any one monetary unit (in particular, DOLLAR_UNIT, EURO_UNIT, 'YEN', and 'Yen')
	 */
	public static final String MONEY_UNIT = "(" + DOLLAR_UNIT + "|" + EURO_UNIT + "|((Yen(s)?)|YEN))";
	
	/**	a regular expression capturing group matching any one monetary amount (in particular, a number followed by a monetary unit)
	 */
	public static final String MONEY = "(" + NUMBER + "\\s" + MONEY_UNIT + ")";
	
	
	/**	a regular expression capturing group matching any one time unit (in particular, 'hour', 'minute', '(milli|micro|nano)second', 'day', 'week', 'month', 'year', 'decade', and 'century')
	 */
	public static final String TIME_UNIT = "((h\\b)|(min\\b)|(sec\\b)|((m|μ|n)?s\\b)|(hour(s)?)|(minute(s)?)|((milli|micro|nano)?second(s)?)|(day(s)?)|(week(s)?)|(month(s)?)|(year(s)?)|(decade(s)?)|(centur(y|ies)))";
	
	/**	a regular expression capturing group matching any one time (like '5:30 pm' or 'dawn')
	 */
	public static final String TIME = "(night|noontide|noonday|noon|nightfall|morning|midnight|midday|midafternoon|gloaming|evening|dusk|daybreak|dawn|afternoon|(mid\\s+\\-\\s+day)|(pre\\s+\\-\\s+dawn)|(\\b(10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|0|1|2|3|4|5|6|7|8|9){1}+(((\\s++(\\:|\\-)\\s++[0-5][0-9])?\\s+((a\\.m\\.)|am|(p\\.m\\.)|pm)\\b)|(\\s++(\\:|\\-)\\s++[0-5][0-9]){1}+)))";
	
	
	/**	a regular expression capturing group matching any one duration (in particular, a number followed by a time unit)
	 */
	public static final String DURATION_UNIT = TIME_UNIT;
	
	/**	a regular expression capturing group matching any one duration (in particular, a number followed by a time unit)
	 */
	public static final String DURATION = "(" + NUMBER + "\\s" + TIME_UNIT + ")";
	
	/**	a regular expression capturing group matching any one duration given in days (in particular, a number followed by 'day' or 'days')
	 */
	public static final String DAYS_UNIT = "day(s)?";
	
	/**	a regular expression capturing group matching any one duration given in days (in particular, a number followed by 'day' or 'days')
	 */
	public static final String DAYS = "(" + NUMBER + "\\s" + DAYS_UNIT + ")";
	
	/**	a regular expression capturing group matching any one duration given in years (in particular, a number followed by 'year' or 'years')
	 */
	public static final String YEARS_UNIT = "year(s)?";
	
	/**	a regular expression capturing group matching any one duration given in years (in particular, a number followed by 'year' or 'years')
	 */
	public static final String YEARS = "(" + NUMBER + "\\s" + YEARS_UNIT + ")";
	
	
	/**	a regular expression capturing group matching any one frequency (like 'once', '88 times', 'twice an hour', '25 times per day', or 'every 37 minutes')
	 */
	public static final String FREQUENCY = "((((" + NUMBER + "\\s++times)|once|twice|thrice)(\\s++(per|a(n)?+)\\s++" + TIME_UNIT + ")?)|(every\\s++" + DURATION + "))";
	
	
	/**	a regular expression capturing group matching any one percentage (like '10%', or '25 percent')
	 */
	public static final String PERCENTAGE = "((" + NUMBER + "\\s++(\\%|percent|((out\\s++)?+of\\s++hundred)))|half)";
	
	
	/**	a regular expression capturing group matching any one larger scale area unit (in particular, 'square (kilo)meter', 'square mile', and 'acre')
	 */
	public static final String LARGE_AREA_UNIT = "(((k)?m\\s?+2)|(sq\\s?+\\.\\s?+(k)?m\\b)|(square\\s(kilo)?meter(s)?)|(sq\\s?+\\.\\s?+mi\\b)|(square\\smile(s)?)|(acre(s)?))";
	
	/**	a regular expression capturing group matching any one smaller scale area unit (in particular, 'square (deci|cent|milli|micro|nano)meter', and 'square yard')
	 */
	public static final String SMALL_AREA_UNIT = "(((d|c|m|μ|n)?m\\s?+2)|(sq\\s?+\\.\\s?+(d|c|m|μ|n)?m\\b)|(square\\s(deci|cent|milli|micro|nano)?meter(s)?)|(sq\\s?\\.\\s?yd)|(square\\syard(s)?))";
	
	/**	a regular expression capturing group matching any one area unit (in particular, 'square (kilo|deci|cent|milli|micro|nano)meter', 'square yard', 'square mile', and 'acre')
	 */
	public static final String AREA_UNIT = "(((k|d|c|m|μ|n)?m\\s?+2)|(sq\\s?+\\.\\s?+(k|d|c|m|μ|n)?m\\b)|(square\\s?+(kilo|deci|cent|milli|micro|nano)?meter(s)?)|sq\\s?+\\.\\s?yd|square\\syard(s)?|(sq\\s?+\\.\\s?+mi\\b)|(square\\smile(s)?)|(acre(s)?))";
	
	/**	a regular expression capturing group matching any one larger scale area measure (in particular, a number followed by an larger scale area unit)
	 */
	public static final String AREA_LARGE = "(" + NUMBER + "\\s" + LARGE_AREA_UNIT + ")";
		
	/**	a regular expression capturing group matching any one smaller scale area measure (in particular, a number followed by an smaller scale area unit)
	 */
	public static final String AREA_SMALL = "(" + NUMBER + "\\s" + SMALL_AREA_UNIT + ")";
	
	/**	a regular expression capturing group matching any one area measure (in particular, a number followed by an area unit)
	 */
	public static final String AREA = "(" + NUMBER + "\\s(" + SMALL_AREA_UNIT + "|" + LARGE_AREA_UNIT + "))";
	
	
	/**	a regular expression capturing group matching any one volume unit
	 */
	public static final String VOLUME_UNIT = "(((k|d|c|m|μ|n)?m\\s?+3)|(l\\b)|(ccm\\b)|(gal\\s?+\\.)|(cubic\\s?+(kilo|deci|cent|milli|micro|nano)?meter(s)?)|(liter(s)?)|(litre(s)?)|(gallon(s)?))";
	
	/**	a regular expression capturing group matching any one area measure (in particular, a number followed by a volume unit)
	 */
	public static final String VOLUME = "(" + NUMBER + "\\s" + VOLUME_UNIT + ")";
	
	
	/**	a regular expression capturing group matching any one size measure (in particular, a number followed by a length, area or volume unit)
	 */
	public static final String SIZE_UNIT = "(" + LENGTH_UNIT + "|" + LARGE_AREA_UNIT + "|" + SMALL_AREA_UNIT + "|" + VOLUME_UNIT + ")";
	
	/**	a regular expression capturing group matching any one size measure (in particular, a number followed by a length, area or volume unit)
	 */
	public static final String SIZE = "(" + NUMBER + "\\s(" + LENGTH_UNIT + "|" + LARGE_AREA_UNIT + "|" + SMALL_AREA_UNIT + "|" + VOLUME_UNIT + "))";
	
	
	/**	a regular expression capturing group matching any one volume unit
	 */
	public static final String WEIGHT_UNIT = "(((k|m|μ|n)?g\\b)|(t\\b)|(ibs\\b)|((kilo|milli|micro|nano)?gram(s)?)|(kilo(gram)?(s)?)|(ton(s)?)|pound|stone)";
	
	/**	a regular expression capturing group matching any one area measure (in particular, a number followed by a volume unit)
	 */
	public static final String WEIGHT = "(" + NUMBER + "\\s" + WEIGHT_UNIT + ")";
	
	
	/**	a regular expression capturing group matching any one speed unit (in particular, a length or distance unit divided by a time unit)
	 */
	public static final String SPEED_UNIT = "(mph|mps|kmh|(" + LENGTH_UNIT + "(\\sper\\s|\\s?+\\/\\s?+)" + TIME_UNIT + "))";
	
	/**	a regular expression capturing group matching any one speed (in particular, a number followed by a speed unit)
	 */
	public static final String SPEED = "(" + NUMBER + "\\s" + SPEED_UNIT + ")";
	
	
	/**	a regular expression capturing group matching any one temperature unit (in particular, a length or distance unit divided by a time unit)
	 */
	public static final String TEMPERATURE_UNIT = "(((\\°\\s?+)?+((F\\b)|(C\\b)))|(\\°)|Kelvin)";
	
	/**	a regular expression capturing group matching any one temperature (in particular, a number followed by a temperature unit)
	 */
	public static final String TEMPERATURE = "(" + NUMBER + "\\s?+" + TEMPERATURE_UNIT + ")";
	
	
	/** a regular expression capturing group matching any one angle unit (in particular, 'degree' or °)
	 */
	public static final String ANGLE_UNIT = "(degree(s)?|\\°)";
	
	/** a regular expression capturing group matching any one angle (in particular, a number followed by 'degree' or °)
	 */
	public static final String ANGLE = "(" + NUMBER +"\\s" + ANGLE_UNIT + ")";
	
	
	/**	a regular expression capturing group matching any one month name
	 */
	public static final String MONTH_NAME = "(January|February|March|April|May|June|July|August|September|October|November|December)";
	
	/**	a regular expression capturing group matching any one weekday
	 */
	public static final String WEEKDAY = "(Monday|Tuesday|Wednesday|Thursday|Friday|Saturday|Sunday)";
	
	/**	a regular expression capturing group matching any one day date (in particular, numbers 1 through 31)
	 */
	public static final String DAY = "(0[1-9]|(1|2)[0-9]|30|31|[1-9])";
	
	/**	a regular expression capturing group matching any one ordinal number representing a day date (in particular, numbers 1st through 31st)
	 */
	public static final String ORDINAL_DAY = "((1[0-9]\\s?+th)|([02]?(1\\s?+st|2\\s?+nd|3\\s?+rd|[04-9]\\s?+th))|(30\\s?+th)|(31\\s?+st))";
	
	/**	a regular expression capturing group matching any one month date (in particular, numbers 1 through 12)
	 */
	public static final String MONTH = "(10|11|12|(0?+[1-9]))";
	
	/**	a regular expression capturing group matching any one year date (in particular, numbers 1 through 2999 and their two digit counterparts, the latter optionally preceded by a single quote)
	 */
	public static final String YEAR = "(((1|2)[0-9]{3}+)|([1-9][0-9]{0,2}+)|((\\'\\s?+){1}+[0-9]{2}+))";
	
	/**	a regular expression capturing group matching any one character usually used to separate the digit groups in a date (in particular, the characters ',', '.', '-', '/', and '|')
	 */
	public static final String DATE_SEPARATOR = "(\\,|\\.|\\-|\\/|\\|)";
	
	
	/**	a regular expression capturing group matching any one full date, optionally including the weekday (for instance, 'Tuesday, May 9th, 2006')
	 */
	public static final String DATE_FULL = "((" + WEEKDAY + "(\\s\\,)?\\s)?+" + MONTH_NAME + "\\s(" + ORDINAL_DAY + "|" + DAY + ")((\\s\\,)?\\s" + YEAR + ")?)";
	
	/**	a regular expression capturing group matching any one date given in digits (for instance, '05-09-06')
	 */
	public static final String DATE_DIGITAL = "(" + MONTH + "\\s?+" + DATE_SEPARATOR + "\\s?+" + DAY + "\\s?+" + DATE_SEPARATOR + "\\s?+" + YEAR + ")";
	
	/**	a regular expression capturing group matching any one date
	 */
	public static final String DATE = "(" + DATE_FULL + "|" + DATE_DIGITAL + "|([1-2][0-9]{3}+))";
	
	/**	a regular expression capturing group matching any one day in a month (e.g. 'June 12' or 'May 11th')
	 */
	public static final String DAY_MONTH = "(" + MONTH_NAME + "\\s?+" + DAY + "(\\s?+(th|st|nd|rd)))";
	
	
	
	/**	a regular expression capturing group matching any one decade (like '80s', '1920s' or 'sixties')
	 */
	public static final String DECADE = "(((twen|thir|four|fif|six|seven|eigh|nin)ties)|(([1-2][0-9])?[0-9](0s)))";
	
	/**	a regular expression capturing group matching any one century (like 'eighteenth century', or '21st century')
	 */
	public static final String CENTURY = "((" + ORDINAL_ONE + "|" + ORDINAL_Xteen + "|twentieth|twentyfirst|(1[0-9]\\s?+th)|(20th)|(21st)){1}+(\\s++century))";
	
	
	/**	a regular expression capturing group matching any sequence of two or more upper case letters (probably acronyms)
	 */
	public static final String ALL_UPPER_CASE_ACRONYM = "([A-Z]{2,}+)";
	
	/**	a regular expression capturing group matching any sequence of two or more upper case letters, intermixed with punctuation marks like dots and ampersands (probably acronyms)
	 */
	public static final String PUNCTUATED_ALL_UPPER_CASE_ACRONYM = "([A-Z]{1,3}((\\s?+[\\.\\&]\\s?+)?([A-Z](\\s?\\.\\s?)?)))";
	
	/**	a regular expression capturing group matching any sequence of two or more upper case letters with intermediate or tailing lower case ones (probably acronyms)
	 */
	public static final String MIXED_CASE_ACRONYM = "([A-Z][a-z]*+([A-Z]+[a-z]*+)++)";
	
	/**	a regular expression capturing group matching any probable acronym (disjunctive combination of MIXED_CASE_ACRONYM and PUNCTUATED_ALL_UPPER_CASE_ACRONYM)
	 */
	public static final String ACRONYM = "(" + PUNCTUATED_ALL_UPPER_CASE_ACRONYM + "|" + MIXED_CASE_ACRONYM + ")";
	
	
	/**	a regular expression capturing group matching any probable single score from sports events (like soccer or basketball scores, '5:3' or '89:109')
	 */
	public static final String SINGLE_SCORE = "([0-9]{1,4}+\\s*+\\:\\s*+[0-9]{1,4}+)";
	
	/**	a regular expression capturing group matching any probable multi score from sports events (like tennis, '6:3, 6:1, 6:7, 7:5')
	 */
	public static final String MULTI_SCORE = SINGLE_SCORE + "(\\s*+(\\,)?+\\s*+" + SINGLE_SCORE + ")++";
	
	/**	a regular expression capturing group matching any probable single or multi score from sports events (like soccer or tennis, '5:3' or '6:3, 6:1, 6:7, 7:5')
	 */
	public static final String SCORE = SINGLE_SCORE + "(\\s*+(\\,)?+\\s*+" + SINGLE_SCORE + ")*+";
	
	
	/**	a regular expression capturing group matching the protocol part of any URL (like 'http://')
	 */
	public static final String URL_PROTOCOL = "(((http)|(ftp)|(file)|(svn))(\\s?+\\:\\s?+\\/\\/){1}+)";
	
	/**	a regular expression capturing group matching the authority part of any URL (like 'www.uni-karlsruhe.de')
	 */
	public static final String URL_AUTHORITY = "(([A-Za-z]|(\\s?+\\-\\s?+))++(\\s?+\\.\\s?+(([A-Za-z]|(\\s?+\\-\\s?+))++)++)++)";
	
	/**	a regular expression capturing group matching the port part of any URL (like '8080')
	 */
	public static final String URL_PORT = "(\\s?+\\:\\s?+[0-9]{1,5}+)";
	
	/**	a regular expression capturing group matching the path part of any URL (like '/res/index.html')
	 */
	public static final String URL_FILE = "((\\s?+\\/\\s?+([A-Za-z]|(\\s?+(\\-|\\.|\\~)\\s?+))++)++)";
	
	/**	a regular expression capturing group matching any URL (like 'http://www.uni-karlsruhe.de:8080/res/index.html')
	 */
	//public static final String URL = "(" + URL_PROTOCOL + "?+" + URL_AUTHORITY + URL_PORT + "?+" + URL_FILE + "?(\\/)?+)";
	public static final String URL = "(" + URL_PROTOCOL + "?+" + "(([A-Za-z]|(\\s?+\\-\\s?+))++(\\s?+\\.\\s?+(([A-Za-z]|(\\s?+\\-\\s?+))++)++)+(\\s?+\\.\\s?+(aero|arpa|biz|com|coop|edu|gov|info|int|jobs|mil|mobi|museum|name|net|org|pro|travel|com|de|net|uk|org|info|eu|nl|biz|it))))";
	
	
	/**	a regular expression capturing group matching any legal sentence ('not guilty' and 'guilty')
	 */
	public static final String LEGAL_SENTENCE = "((not\\s)?+guilty)";
	
	
	/**	a regular expression capturing group matching any proper name (sequence of capitalized words, with only 'of the', 'of', 'the', 'and' allowed in lower case)
	 */
	public static final String PROPER_NAME = "(([A-Z][a-z]*+)(\\s++(((of\\s?the)|of|the|and)\\s++)?+([A-Z][a-z]*+\\b))*+)";
	
	
	/**	a regular expression capturing group matching any street (in particular, a proper name followed by a synonym of 'street', like 'Madison Avenue', 'Capitol Beltway', 'Burbon Street', etc)
	 */
	public static final String STREET = "(" + PROPER_NAME + "\\s++(alley|ave(\\s++\\.)?|avenue|beltway|blvd(\\s++\\.)?|boulevard|((memorial\\s++)?drive)|freeway|highway|interstate|lane|((memorial\\s++)?parkway)|road|route|st(\\s++\\.)?|street))";
	
	
	/**	a regular expression capturing group matching any county name (in particular, a proper name followed by 'County')
	 */
	public static final String COUNTY = "(" + PROPER_NAME + "\\s++(County))";
	
	
	/**	a regular expression capturing group matching any reef name (in particular, a proper name followed by 'Reef')
	 */
	public static final String REEF = "(" + PROPER_NAME + "\\s++(Reef))";
	
	
	/**	a regular expression capturing group matching any educational institution (in particular, a proper name followed by one of 'university', 'college', 'high school', or 'elementary', or 'university of' followed by a proper name)
	 */
	public static final String EDUCATIONAL_INSTITUTION = "((University\\s++(Of|of)\\s++" + PROPER_NAME + ")|(([A-Z][a-z]+)(\\s+(((of\\s?the)|of|the|and)\\s+)?([A-Z][a-z]+))*\\s++(University|College|(High(\\s++School)?+))|(Elementary(\\s++School)?+)))";
	
	/** case sensitivity default for regular expression patterns */
	public static final boolean PATTERNS_CASE_INSENSITIVE = false;
	
	/**	create a Pattern from a regular expression String, using default case sensitivity
	 * @param	regEx	the regular expression String
	 * @return a Pattern compiled from the specified regular expression String
	 */
	public static Pattern compile(String regEx) {
		return compile(regEx, PATTERNS_CASE_INSENSITIVE);
	}
	
	/**	create a Pattern from a regular expression String
	 * @param	regEx			the regular expression String
	 * @param	caseSensitive	create a case sensitive Pattern or not?
	 * @return a Pattern compiled from the specified regular expression String
	 */
	public static Pattern compile(String regEx, boolean caseSensitive) {
		regEx = "\\b" + regEx + "\\b";
		if (caseSensitive) return Pattern.compile(regEx);
		else return Pattern.compile(regEx, Pattern.CASE_INSENSITIVE);
	}
	
	
	/** a regular expression capturing group matching all cardinal numbers given in form of digits
	 */ 
	public static final Pattern NUMBER_PATTERN = compile(NUMBER);
	public static final int NUMBER_MAX_TOKENS = 9;
	
	/** a regular expression capturing group matching all ordinal numbers given in form of digits
	 */
	public static final Pattern ORDINAL_PATTERN = compile(ORDINAL, false);
	public static final int ORDINAL_MAX_TOKENS = 10;
	
	/** a regular expression capturing group matching all one digit cardinal numbers given in form of words
	 */ 
	public static final Pattern NUMBER_ONE_PATTERN = compile(NUMBER_ONE, false);
	public static final int NUMBER_ONE_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all two digit cardinal numbers given in form of words whose first digit is one
	 */ 
	public static final Pattern NUMBER_Xteen_PATTERN = compile(NUMBER_Xteen, false);
	public static final int NUMBER_Xteen_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all two digit cardinal numbers given in form of words
	 */ 
	public static final Pattern NUMBER_TEN_PATTERN = compile(NUMBER_TEN, false);
	public static final int NUMBER_TEN_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all three digit cardinal numbers given in form of words whose last two digits are zero
	 */ 
	public static final Pattern NUMBER_HUNDRED_PATTERN = compile(NUMBER_HUNDRED, false);
	public static final int NUMBER_HUNDRED_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all three digit cardinal numbers given in form of words whose last two digits are zero, and all four digit cardinal numbers given in form of words whose first digit is 1 and whose last two digits are zero
	 */ 
	public static final Pattern NUMBER_HUNDRED_WITH_Xteen_PATTERN = compile(NUMBER_HUNDRED_WITH_Xteen, false);
	public static final int NUMBER_HUNDRED_WITH_Xteen_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all three digit cardinal numbers given in form of words
	 */ 
	public static final Pattern NUMBER_TO_HUNDRED_PATTERN = compile(NUMBER_TO_HUNDRED, false);
	public static final int NUMBER_TO_HUNDRED_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all four digit cardinal numbers given in form of words whose last three digits are zero
	 */ 
	public static final Pattern NUMBER_THOUSAND_PATTERN = compile(NUMBER_THOUSAND, false);
	public static final int NUMBER_THOUSAND_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all up to six digit cardinal numbers given in form of words
	 */ 
	public static final Pattern NUMBER_TO_THOUSAND_PATTERN = compile(NUMBER_TO_THOUSAND, false);
	public static final int NUMBER_TO_THOUSAND_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all cardinal numbers involving 'million', 'billion' or 'trillion'
	 */ 
	public static final Pattern NUMBER_Xillion_PATTERN = compile(NUMBER_Xillion, false);
	public static final int NUMBER_Xillion_MAX_TOKENS = 1;
	
	
	/** a regular expression capturing group matching all one digit ordinal numbers given in form of words
	 */ 
	public static final Pattern ORDINAL_ONE_PATTERN = compile(ORDINAL_ONE, false);
	public static final int ORDINAL_ONE_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all two digit ordinal numbers given in form of words whose first digit is one
	 */ 
	public static final Pattern ORDINAL_Xteen_PATTERN = compile(ORDINAL_Xteen, false);
	public static final int ORDINAL_Xteen_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all two digit ordinal numbers given in form of words
	 */ 
	public static final Pattern ORDINAL_TEN_PATTERN = compile(ORDINAL_TEN, false);
	public static final int ORDINAL_TEN_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all three digit ordinal numbers given in form of words whose last two digits are zero
	 */ 
	public static final Pattern ORDINAL_HUNDRED_PATTERN = compile(ORDINAL_HUNDRED, false);
	public static final int ORDINAL_HUNDRED_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all three digit ordinal numbers given in form of words whose last two digits are zero, and all four digit cardinal numbers given in form of words whose first digit is 1 and whose last two digits are zero
	 */ 
	public static final Pattern ORDINAL_HUNDRED_WITH_Xteen_PATTERN = compile(ORDINAL_HUNDRED_WITH_Xteen, false);
	public static final int ORDINAL_HUNDRED_WITH_Xteen_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all three digit ordinal numbers given in form of words
	 */ 
	public static final Pattern ORDINAL_TO_HUNDRED_PATTERN = compile(ORDINAL_TO_HUNDRED, false);
	public static final int ORDINAL_TO_HUNDRED_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all four digit ordinal numbers given in form of words whose last three digits are zero
	 */ 
	public static final Pattern ORDINAL_THOUSAND_PATTERN = compile(ORDINAL_THOUSAND, false);
	public static final int ORDINAL_THOUSAND_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching all up to six digit ordinal numbers given in form of words
	 */ 
	public static final Pattern ORDINAL_TO_THOUSAND_PATTERN = compile(ORDINAL_TO_THOUSAND, false);
	public static final int ORDINAL_TO_THOUSAND_MAX_TOKENS = 1;
	
	
	/** a regular expression capturing group matching any one length unit (in particular, '(kilo|deci|cent|milli|micro|nano)meter', 'foot', 'inch', and 'yard')
	 */ 
	public static final Pattern LENGTH_UNIT_PATTERN = compile(LENGTH_UNIT, false);
	public static final int LENGTH_UNIT_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one smaller scale length unit (this constant is equal to LENGHT_UNIT, it is only provided for clarity)
	 */ 
	public static final Pattern HEIGHT_UNIT_PATTERN = compile(HEIGHT_UNIT, false);
	public static final int HEIGHT_UNIT_MAX_TOKENS = LENGTH_UNIT_MAX_TOKENS;
	
	/** a regular expression capturing group matching any one length (in particular, a number followed by a length unit)
	 */ 
	public static final Pattern LENGTH_PATTERN = compile(LENGTH, false);
	public static final int LENGTH_MAX_TOKENS = NUMBER_MAX_TOKENS + LENGTH_UNIT_MAX_TOKENS;
	
	/** a regular expression capturing group matching any one height (this constant is equal to LENGHT, it is only provided for clarity)
	 */ 
	public static final Pattern HEIGHT_PATTERN = compile(HEIGHT, false);
	public static final int HEIGHT_MAX_TOKENS = NUMBER_MAX_TOKENS + HEIGHT_UNIT_MAX_TOKENS;
	
	
	/** a regular expression capturing group matching any one US Dollar unit (in particular, '$', 'USD', and 'Dollar')
	 */ 
	public static final Pattern DOLLAR_UNIT_PATTERN = compile(DOLLAR_UNIT, false);
	public static final int DOLLAR_UNIT_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one Euro unit (in particular, '€', 'EURO', and 'Euro')
	 */ 
	public static final Pattern EURO_UNIT_PATTERN = compile(EURO_UNIT, false);
	public static final int EURO_UNIT_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one monetary unit (in particular, DOLLAR_UNIT, EURO_UNIT, 'YEN', and 'Yen')
	 */ 
	public static final Pattern MONEY_UNIT_PATTERN = compile(MONEY_UNIT, false);
	public static final int MONEY_UNIT_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one monetary amount (in particular, a number followed by a monetary unit)
	 */ 
	public static final Pattern MONEY_PATTERN = compile(MONEY, false);
	public static final int MONEY_MAX_TOKENS = NUMBER_MAX_TOKENS + MONEY_UNIT_MAX_TOKENS;
	
	
	/** a regular expression capturing group matching any one time unit (in particular, 'hour', 'minute', '(milli|micro|nano)second', 'day', 'week', 'month', 'year', 'decade', and 'century')
	 */ 
	public static final Pattern TIME_UNIT_PATTERN = compile(TIME_UNIT, false);
	public static final int TIME_UNIT_MAX_TOKENS = 1;
	
	/**	a regular expression capturing group matching any one time (like '5:30 pm' or 'dawn')
	 */
	public static final Pattern TIME_PATTERN = compile(TIME, false);
	public static final int TIME_MAX_TOKENS = 7;
	
	/** a regular expression capturing group matching any one duration unit (in particular, a number followed by a time unit)
	 */ 
	public static final Pattern DURATION_UNIT_PATTERN = compile(DURATION_UNIT, false);
	public static final int DURATION_UNIT_MAX_TOKENS = TIME_UNIT_MAX_TOKENS;
	
	/** a regular expression capturing group matching any one duration (in particular, a number followed by a time unit)
	 */ 
	public static final Pattern DURATION_PATTERN = compile(DURATION, false);
	public static final int DURATION_MAX_TOKENS = NUMBER_MAX_TOKENS + TIME_UNIT_MAX_TOKENS;
	
	/** a regular expression capturing group matching any one duration given in days (in particular, a number followed by a time unit)
	 */ 
	public static final Pattern DAYS_UNIT_PATTERN = compile(DAYS_UNIT, false);
	public static final int DAYS_UNIT_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one duration given in days (in particular, a number followed by a time unit)
	 */ 
	public static final Pattern DAYS_PATTERN = compile(DAYS, false);
	public static final int DAYS_MAX_TOKENS = NUMBER_MAX_TOKENS + 1;
	
	/** a regular expression capturing group matching any one duration (in particular, a number followed by a time unit)
	 */ 
	public static final Pattern YEARS_UNIT_PATTERN = compile(YEARS_UNIT, false);
	public static final int YEARS_UNIT_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one duration (in particular, a number followed by a time unit)
	 */ 
	public static final Pattern YEARS_PATTERN = compile(YEARS, false);
	public static final int YEARS_MAX_TOKENS = NUMBER_MAX_TOKENS + 1;
	
	
	/**	a regular expression capturing group matching any one frequency (like 'once', '88 times', 'twice an hour', '25 times per day', or 'every 37 minutes')
	 */
	public static final Pattern FREQUENCY_PATTERN = compile(FREQUENCY, false);
	public static final int FREQUENCY_MAX_TOKENS = DURATION_MAX_TOKENS + 2; // 'every' + DURATION, or NUMBER + 'times + per' + TIME_UNIT
	
	
	/**	a regular expression capturing group matching any one percentage (like '20 out of hundred', '34 percent', or '100 %')
	 */
	public static final Pattern PERCENTAGE_PATTERN = compile(PERCENTAGE, false);
	public static final int PERCENTAGE_MAX_TOKENS = NUMBER_MAX_TOKENS + 3; // NUMBER 'out of hundred'
	
	/** a regular expression capturing group matching any one larger scale area unit (in particular, 'square (kilo)meter', 'square mile', and 'acre')
	 */ 
	public static final Pattern LARGE_AREA_UNIT_PATTERN = compile(LARGE_AREA_UNIT, false);
	public static final int LARGE_AREA_UNIT_MAX_TOKENS = 3;
	
	/** a regular expression capturing group matching any one smaller scale area unit (in particular, 'square (deci|cent|milli|micro|nano)meter', and 'square yard')
	 */ 
	public static final Pattern SMALL_AREA_UNIT_PATTERN = compile(SMALL_AREA_UNIT, false);
	public static final int SMALL_AREA_UNIT_MAX_TOKENS = 3;
	
	/** a regular expression capturing group matching any one smaller scale area unit (in particular, 'square (deci|cent|milli|micro|nano)meter', and 'square yard')
	 */ 
	public static final Pattern AREA_UNIT_PATTERN = compile(AREA_UNIT, false);
	public static final int AREA_UNIT_MAX_TOKENS = 3;
	
	/** a regular expression capturing group matching any one larger scale area measure (in particular, a number followed by an larger scale area unit)
	 */ 
	//public static final Pattern AREA_LARGE_PATTERN = compile(AREA_LARGE);
	//public static final int AREA_LARGE_MAX_TOKENS = ;
	
	/** a regular expression capturing group matching any one smaller scale area measure (in particular, a number followed by an smaller scale area unit)
	 */ 
	//public static final Pattern AREA_SMALL_PATTERN = compile(AREA_SMALL);
	//public static final int AREA_SMALL_MAX_TOKENS = ;
	
	/** a regular expression capturing group matching any one area measure (in particular, a number followed by an area unit)
	 */ 
	public static final Pattern AREA_PATTERN = compile(AREA, false);
	public static final int AREA_MAX_TOKENS = NUMBER_MAX_TOKENS + AREA_UNIT_MAX_TOKENS;
	
	
	/** a regular expression capturing group matching any one volume unit
	 */ 
	public static final Pattern VOLUME_UNIT_PATTERN = compile(VOLUME_UNIT, false);
	public static final int VOLUME_UNIT_MAX_TOKENS = 2;
	
	/** a regular expression capturing group matching any one area measure (in particular, a number followed by a volume unit)
	 */ 
	public static final Pattern VOLUME_PATTERN = compile(VOLUME, false);
	public static final int VOLUME_MAX_TOKENS =NUMBER_MAX_TOKENS + VOLUME_UNIT_MAX_TOKENS;
	
	
	/** a regular expression capturing group matching any one size measure (in particular, a number followed by a length, area or volume unit)
	 */ 
	public static final Pattern SIZE_UNIT_PATTERN = compile(SIZE_UNIT, false);
	public static final int SIZE_UNIT_MAX_TOKENS = 3;
	
	/** a regular expression capturing group matching any one size measure (in particular, a number followed by a length, area or volume unit)
	 */ 
	public static final Pattern SIZE_PATTERN = compile(SIZE, false);
	public static final int SIZE_MAX_TOKENS = NUMBER_MAX_TOKENS + SIZE_UNIT_MAX_TOKENS;
	
	
	/** a regular expression capturing group matching any one volume unit
	 */ 
	public static final Pattern WEIGHT_UNIT_PATTERN = compile(WEIGHT_UNIT, false);
	public static final int WEIGHT_UNIT_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one area measure (in particular, a number followed by a volume unit)
	 */ 
	public static final Pattern WEIGHT_PATTERN = compile(WEIGHT, false);
	public static final int WEIGHT_MAX_TOKENS = NUMBER_MAX_TOKENS + WEIGHT_UNIT_MAX_TOKENS;
	
	
	/** a regular expression capturing group matching any one speed unit (in particular, a length or distance unit divided by a time unit)
	 */ 
	public static final Pattern SPEED_UNIT_PATTERN = compile(SPEED_UNIT, false);
	public static final int SPEED_UNIT_MAX_TOKENS = 3;
	
	/** a regular expression capturing group matching any one speed (in particular, a number followed by a speed unit)
	 */ 
	public static final Pattern SPEED_PATTERN = compile(SPEED, false);
	public static final int SPEED_MAX_TOKENS = NUMBER_MAX_TOKENS + SPEED_UNIT_MAX_TOKENS;
	
	
	/** a regular expression capturing group matching any one temperature unit (in particular, a length or distance unit divided by a time unit)
	 */ 
	public static final Pattern TEMPERATURE_UNIT_PATTERN = compile(TEMPERATURE_UNIT, false);
	public static final int TEMPERATURE_UNIT_MAX_TOKENS = 2;
	
	/** a regular expression capturing group matching any one temperature (in particular, a number followed by a temperature unit)
	 */ 
	public static final Pattern TEMPERATURE_PATTERN = compile(TEMPERATURE, false);
	public static final int TEMPERATURE_MAX_TOKENS = NUMBER_MAX_TOKENS + TEMPERATURE_UNIT_MAX_TOKENS;
	
	
	/** a regular expression capturing group matching any one angle unit (in particular, 'degree' or °)
	 */ 
	public static final Pattern ANGLE_UNIT_PATTERN = compile(ANGLE_UNIT, false);
	public static final int ANGLE_UNIT_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one angle (in particular, a number followed by 'degree' or °)
	 */ 
	public static final Pattern ANGLE_PATTERN = compile(ANGLE, false);
	public static final int ANGLE_MAX_TOKENS = NUMBER_MAX_TOKENS + ANGLE_UNIT_MAX_TOKENS;
	
	/** a regular expression capturing group matching
	 *  any one month name
	 */ 
	public static final Pattern MONTH_NAME_PATTERN = compile(MONTH_NAME, false);
	public static final int MONTH_NAME_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one weekday
	 */ 
	public static final Pattern WEEKDAY_PATTERN = compile(WEEKDAY, false);
	public static final int WEEKDAY_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one day date (in particular, numbers 1 through 31)
	 */ 
	public static final Pattern DAY_PATTERN = compile(DAY);
	public static final int DAY_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one month date (in particular, numbers 1 through 12)
	 */ 
	public static final Pattern MONTH_PATTERN = compile(MONTH_NAME);
	public static final int MONTH_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any one year date (in particular, numbers 1 through 2999 and their two digit counterparts, the latter optionally preceded by a single quote)
	 */ 
	public static final Pattern YEAR_PATTERN = compile("([1-2][0-9]{3}+)");//Pattern.compile(YEAR);
	public static final int YEAR_MAX_TOKENS = 1;//2;
	
	/** a regular expression capturing group matching any one character usually used to separate the digit groups in a date (in particular, the characters ',', '.', '-', '/', and '|')
	 */ 
	public static final Pattern DATE_SEPARATOR_PATTERN = compile(DATE_SEPARATOR);
	public static final int DATE_SEPARATOR_MAX_TOKENS = 1;
	
	
	/** a regular expression capturing group matching any one full date, optionally including the weekday (for instance, 'Tuesday, May 9th, 2006')
	 */ 
	public static final Pattern DATE_FULL_PATTERN = compile(DATE_FULL, false);
	public static final int DATE_FULL_MAX_TOKENS = 8;
	
	/** a regular expression capturing group matching any one date given in digits (for instance, '05-09-06')
	 */ 
	public static final Pattern DATE_DIGITAL_PATTERN = compile(DATE_DIGITAL);
	public static final int DATE_DIGITAL_MAX_TOKENS = 5;
	
	/** a regular expression capturing group matching any one date
	 */ 
	public static final Pattern DATE_PATTERN = compile(DATE, false);
	public static final int DATE_MAX_TOKENS = 8;
	
	/** a regular expression capturing group matching any one day in a month
	 */ 
	public static final Pattern DAY_MONTH_PATTERN = compile(DAY_MONTH, false);
	public static final int DAY_MONTH_MAX_TOKENS = 3;
	
	
	/**	a regular expression capturing group matching any one decade (like '80s', '1920s' or 'sixties')
	 */
	public static final Pattern DECADE_PATTERN = compile(DECADE, false);
	public static final int DECADE_MAX_TOKENS = 2;
	
	/**	a regular expression capturing group matching any one century (like 'eighteenth century', or '21st century')
	 */
	public static final Pattern CENTURY_PATTERN = compile(CENTURY, false);
	public static final int CENTURY_MAX_TOKENS = 3;
	
	
	/** a regular expression capturing group matching any sequence of two or more upper case letters (probably acronyms)
	 */ 
	public static final Pattern ALL_UPPER_CASE_ACRONYM_PATTERN = compile(ALL_UPPER_CASE_ACRONYM, true);
	public static final int ALL_UPPER_CASE_ACRONYM_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any sequence of two or more upper case letters, intermixed with punctuation marks like dots and ampersands (probably acronyms)
	 */ 
	public static final Pattern PUNCTUATED_ALL_UPPER_CASE_ACRONYM_PATTERN = compile(PUNCTUATED_ALL_UPPER_CASE_ACRONYM, true);
	public static final int PUNCTUATED_ALL_UPPER_CASE_ACRONYM_MAX_TOKENS = 9;
	
	/** a regular expression capturing group matching any sequence of two or more upper case letters with intermediate or tailing lower case ones (probably acronyms)
	 */ 
	public static final Pattern MIXED_CASE_ACRONYM_PATTERN = compile(MIXED_CASE_ACRONYM, true);
	public static final int MIXED_CASE_ACRONYM_MAX_TOKENS = 1;
	
	/** a regular expression capturing group matching any probable acronym (disjunctive combination of MIXED_CASE_ACRONYM and PUNCTUATED_ALL_UPPER_CASE_ACRONYM)
	 */ 
	public static final Pattern ACRONYM_PATTERN = compile(ACRONYM, true);
	public static final int ACRONYM_MAX_TOKENS = 9;
	
	
	/**	a regular expression capturing group matching any probable single score from sports events (like soccer or basketball scores, '5:3' or '89:109')
	 */
	public static final Pattern SINGLE_SCORE_PATTERN = compile(SINGLE_SCORE);
	public static final int SINGLE_SCORE_MAX_TOKENS = 3;
	
	/**	a regular expression capturing group matching any probable multi score from sports events (like tennis, '6:3, 6:1, 6:7, 7:5')
	 */
	public static final Pattern MULTI_SCORE_PATTERN = compile(MULTI_SCORE);
	public static final int MULTI_SCORE_MAX_TOKENS = 19;
	
	/**	a regular expression capturing group matching any probable single or multi score from sports events (like soccer or tennis, '5:3' or '6:3, 6:1, 6:7, 7:5')
	 */
	public static final Pattern SCORE_PATTERN = compile(SCORE);
	public static final int SCORE_MAX_TOKENS = MULTI_SCORE_MAX_TOKENS;
	
	
	/**	a regular expression capturing group matching any URL (like 'http://www.uni-karlsruhe.de:8080/res/index.html')
	 */
	public static final Pattern URL_PATTERN = compile(URL, false);
	public static final int URL_MAX_TOKENS = 3 + 10;//3 + 10 + 2 + 10; // for 'protocol + authority + port + file'
	
	
	/**	a regular expression capturing group matching any legal sentence ('not guilty' and 'guilty')
	 */
	public static final Pattern LEGAL_SENTENCE_PATTERN = compile(LEGAL_SENTENCE, false);
	public static final int LEGAL_SENTENCE_MAX_TOKENS = 2;
	
	
	/**	a regular expression capturing group matching any proper name (sequence of capitalized words, with only 'of the', 'of', 'the', 'and' allowed in lower case)
	 */
	public static final Pattern PROPER_NAME_PATTERN = compile(PROPER_NAME, true);
	public static final int PROPER_NAME_MAX_TOKENS = 7;
	
	
	/**	a regular expression capturing group matching any street (in particular, a proper name followed by a synonym of 'street', like 'Madison Avenue', 'Capitol Beltway', 'Burbon Street', etc)
	 */
	public static final Pattern STREET_PATTERN = compile(STREET);
	public static final int STREET_MAX_TOKENS = 7;
	
	
	/**	a regular expression capturing group matching any county name (in particular, a proper name followed by 'County')
	 */
	public static final Pattern COUNTY_PATTERN = compile(COUNTY);
	public static final int COUNTY_MAX_TOKENS = 4;
	
	
	/**	a regular expression capturing group matching any reef name (in particular, a proper name followed by 'Reef')
	 */
	public static final Pattern REEF_PATTERN = compile(REEF);
	public static final int REEF_MAX_TOKENS = 4;
	
	
	/**	a regular expression capturing group matching any educational institution (in particular, a proper name followed by one of 'University', 'College', 'High (School)', or 'Elementary (School)', or 'University of' followed by a proper name)
	 */
	public static final Pattern EDUCATIONAL_INSTITUTION_PATTERN = compile(EDUCATIONAL_INSTITUTION);
	public static final int EDUCATIONAL_INSTITUTION_MAX_TOKENS = 6;
	
	
	public static final String FEET_UNIT = "(foot|feet|ft)";
	public static final int FEET_UNIT_MAX_TOKENS = 1;
	public static final Pattern FEET_UNIT_PATTERN = compile(FEET_UNIT, false);
	public static final String FEET = "(" + NUMBER + "\\s" + FEET_UNIT + ")";
	public static final int FEET_MAX_TOKENS = NUMBER_MAX_TOKENS + FEET_UNIT_MAX_TOKENS;
	public static final Pattern FEET_PATTERN = compile(FEET, false);
	
	public static final String GALLONS_UNIT = "(gallon(s)?|gl)";
	public static final int GALLONS_UNIT_MAX_TOKENS = 1;
	public static final Pattern GALLONS_UNIT_PATTERN = compile(GALLONS_UNIT, false);
	public static final String GALLONS = "(" + NUMBER + "\\s" + GALLONS_UNIT + ")";
	public static final int GALLONS_MAX_TOKENS = NUMBER_MAX_TOKENS + GALLONS_UNIT_MAX_TOKENS;
	public static final Pattern GALLONS_PATTERN = compile(GALLONS, false);
	
	public static final String GRAMS_UNIT = "(gram(s)?|g)";
	public static final int GRAMS_UNIT_MAX_TOKENS = 1;
	public static final Pattern GRAMS_UNIT_PATTERN = compile(GRAMS_UNIT, false);
	public static final String GRAMS = "(" + NUMBER + "\\s" + GRAMS_UNIT + ")";
	public static final int GRAMS_MAX_TOKENS = NUMBER_MAX_TOKENS + GRAMS_UNIT_MAX_TOKENS;
	public static final Pattern GRAMS_PATTERN = compile(GRAMS, false);
	
	public static final String LITERS_UNIT = "(liter(s)?|l)";
	public static final int LITERS_UNIT_MAX_TOKENS = 1;
	public static final Pattern LITERS_UNIT_PATTERN = compile(LITERS_UNIT, false);
	public static final String LITERS = "(" + NUMBER + "\\s" + LITERS_UNIT + ")";
	public static final int LITERS_MAX_TOKENS = NUMBER_MAX_TOKENS + LITERS_UNIT_MAX_TOKENS;
	public static final Pattern LITERS_PATTERN = compile(LITERS, false);
	
	public static final String MILES_UNIT = "(mile(s)?|mi)";
	public static final int MILES_UNIT_MAX_TOKENS = 1;
	public static final Pattern MILES_UNIT_PATTERN = compile(MILES_UNIT, false);
	public static final String MILES = "(" + NUMBER + "\\s" + MILES_UNIT + ")";
	public static final int MILES_MAX_TOKENS = NUMBER_MAX_TOKENS + MILES_UNIT_MAX_TOKENS;
	public static final Pattern MILES_PATTERN = compile(MILES, false);
	
	public static final String MPH_UNIT = "((miles\\s(per|\\/)\\shour)|mph)";
	public static final int MPH_UNIT_MAX_TOKENS = 3;
	public static final Pattern MPH_UNIT_PATTERN = compile(MPH_UNIT, false);
	public static final String MPH = "(" + NUMBER + "\\s" + MPH_UNIT + ")";
	public static final int MPH_MAX_TOKENS = NUMBER_MAX_TOKENS + MPH_UNIT_MAX_TOKENS;
	public static final Pattern MPH_PATTERN = compile(MPH, false);
	
	public static final String OUNCES_UNIT = "(ounce(s)?|oz)";
	public static final int OUNCES_UNIT_MAX_TOKENS = 1;
	public static final Pattern OUNCES_UNIT_PATTERN = compile(OUNCES_UNIT, false);
	public static final String OUNCES = "(" + NUMBER + "\\s" + OUNCES_UNIT + ")";
	public static final int OUNCES_MAX_TOKENS = NUMBER_MAX_TOKENS + OUNCES_UNIT_MAX_TOKENS;
	public static final Pattern OUNCES_PATTERN = compile(OUNCES, false);
	
//	public static final String PHONENUMBER_UNIT = "";
//	public static final int PHONENUMBER_UNIT_MAX_TOKENS = 1;
//	public static final String PHONENUMBER = "(" + NUMBER + "\\s" + PHONENUMBER_UNIT + ")";
//	public static final int PHONENUMBER_MAX_TOKENS = NUMBER_MAX_TOKENS + PHONENUMBER_UNIT_MAX_TOKENS;
//	public static final Pattern PHONENUMBER_PATTERN = compile(PHONENUMBER, false);
	
	public static final String POUNDS_UNIT = "(pound(s)?|lb(s)?)";
	public static final int POUNDS_UNIT_MAX_TOKENS = 1;
	public static final Pattern POUNDS_UNIT_PATTERN = compile(POUNDS_UNIT, false);
	public static final String POUNDS = "(" + NUMBER + "\\s" + POUNDS_UNIT + ")";
	public static final int POUNDS_MAX_TOKENS = NUMBER_MAX_TOKENS + POUNDS_UNIT_MAX_TOKENS;
	public static final Pattern POUNDS_PATTERN = compile(POUNDS, false);
	
	public static final String RANGE_UNIT = LENGTH_UNIT;
	public static final int RANGE_UNIT_MAX_TOKENS = LENGTH_UNIT_MAX_TOKENS;
	public static final Pattern RANGE_UNIT_PATTERN = LENGTH_UNIT_PATTERN;//compile(RANGE, false);
	public static final String RANGE = LENGTH;//"(" + NUMBER + "\\s" + RANGE_UNIT + ")";
	public static final int RANGE_MAX_TOKENS = LENGTH_MAX_TOKENS;//NUMBER_MAX_TOKENS + RANGE_UNIT_MAX_TOKENS;
	public static final Pattern RANGE_PATTERN = LENGTH_PATTERN;//compile(RANGE, false);
	
//	public static final String RATE_UNIT = "";
//	public static final int RATE_UNIT_MAX_TOKENS = 1;
//	public static final Pattern RATE_UNIT_PATTERN = compile(RATE_UNIT, false);
//	
	public static final String RATE = "(" + NUMBER + "\\s([A-Za-z]{3,}\\s)?(per|\\/)\\s[A-Za-z]{3,})";
	public static final int RATE_MAX_TOKENS = NUMBER_MAX_TOKENS + 4;
	public static final Pattern RATE_PATTERN = compile(RATE, false);
	
	public static final String SQUARE_MILES_UNIT = "(square\\smile(s)?|sq\\s\\.\\smi\\s\\.|sq\\smi)";
	public static final int SQUARE_MILES_UNIT_MAX_TOKENS = 5;
	public static final Pattern SQUARE_MILES_UNIT_PATTERN = compile(SQUARE_MILES_UNIT, false);
	public static final String SQUARE_MILES = "(" + NUMBER + "\\s" + SQUARE_MILES_UNIT + ")";
	public static final int SQUARE_MILES_MAX_TOKENS = NUMBER_MAX_TOKENS + SQUARE_MILES_UNIT_MAX_TOKENS;
	public static final Pattern SQUARE_MILES_PATTERN = compile(SQUARE_MILES, false);
	
	public static final String TONS_UNIT = "((metric\\s)?ton(s)?)";
	public static final int TONS_UNIT_MAX_TOKENS = 2;
	public static final Pattern TONS_UNIT_PATTERN = compile(TONS_UNIT, false);
	public static final String TONS = "(" + NUMBER + "\\s" + TONS_UNIT + ")";
	public static final int TONS_MAX_TOKENS = NUMBER_MAX_TOKENS + TONS_UNIT_MAX_TOKENS;
	public static final Pattern TONS_PATTERN = compile(TONS, false);
	
//	public static final String UNIT_UNIT = "";
//	public static final int UNIT_UNIT_MAX_TOKENS = 1;
//	public static final String UNIT = "(" + NUMBER + "\\s" + UNIT_UNIT + ")";
//	public static final int UNIT_MAX_TOKENS = NUMBER_MAX_TOKENS + UNIT_UNIT_MAX_TOKENS;
//	public static final Pattern UNIT_PATTERN = compile(UNIT, false);
	
	public static final String ZIPCODE = "([1-9][0-9]{4}(\\-[0-9]{4})?)";
	public static final int ZIPCODE_MAX_TOKENS = 3;
	public static final Pattern ZIPCODE_PATTERN = compile(ZIPCODE, false);
	
	public static final String PHONE_NUMBER = "(((\\1\\s\\-\\s[0-9]{3}+)|(\\(\\s[0-9]{3}+\\s\\))|([0-9]{3}+\\s\\-))\\s([0-9]{3}+\\s\\-\\s[0-9]{4}+))";
	public static final int PHONE_NUMBER_MAX_TOKENS = 7;
	public static final Pattern PHONE_NUMBER_PATTERN = compile(PHONE_NUMBER, false);
}
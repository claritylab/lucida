package info.ephyra.nlp;

import info.ephyra.io.MsgPrinter;
import info.ephyra.util.FileUtils;
import info.ephyra.util.StringUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileFilter;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

import opennlp.maxent.MaxentModel;
import opennlp.maxent.io.SuffixSensitiveGISModelReader;
import opennlp.tools.lang.english.NameFinder;
import opennlp.tools.namefind.NameFinderME;
import opennlp.tools.parser.Parse;
import opennlp.tools.util.Span;

/**
 * <p>This class combines model-based, pattern-based and list-based named entity
 * taggers.</p>
 * 
 * <p>The pattern-based taggers are optimized for the tokenizer provided in this
 * class. Do not use other tokenizers.</p>
 * 
 * @author Nico Schlaefer, Guido Sautter
 * @version 2007-07-24
 */
public class NETagger {
	// ===================
	// Model-based taggers
	// ===================
	
	/** NE types with model-based taggers. */
	private static String[] MODEL_TYPES =
		{"NElocation", "NEorganization", "NEperson"};
	
	/** Name finders from the OpenNLP project, created from different models. */
	private static NameFinder[] finders = new NameFinder[0];
	
	/**
	 * NE types that are recognized by the OpenNLP name finders. There may be
	 * multiple taggers for the same NE type. IMPORTANT: NE types must be
	 * prefix-free.
	 */
	private static String[] finderNames = new String[0];
	
	/**
	 * NE types that are recognized by the Stanford NE tagger. There may be
	 * multiple taggers for the same NE type. IMPORTANT: NE types must be
	 * prefix-free.
	 */
	private static String[] stanfordNames = {
		"NEperson",
		"NElocation",
		"NEorganization",
		};
	
	/**
	 * Creates the OpenNLP name finders and sets the named entity types that are
	 * recognized by the finders.
	 * 
	 * @param dir directory containing the models for the name finders
	 * @return true, iff the name finders were created successfully
	 */
	public static boolean loadNameFinders(String dir) {
		File[] files = FileUtils.getFiles(dir);
		
		finders = new NameFinder[files.length];
		finderNames = new String[files.length];
		
		try {
		    for (int i = 0; i < files.length; i++) {
		    	MaxentModel model =
		    		new SuffixSensitiveGISModelReader(files[i]).getModel();
		    	
		    	finders[i] = new NameFinder(model);
		    	finderNames[i] = files[i].getName().split("\\.")[0];
				MsgPrinter.printStatusMsg("    ...for " + finderNames[i]);
		    }
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	// ==================
	// List-based taggers
	// ==================
	
	/** File names of lists that match different types of NEs. */
	private static String[] lists = new String[0];
	
	/**
	 * NE types of the entries in the lists. There may be multiple taggers for
	 * the same NE type. IMPORTANT: NE types must be prefix-free.
	 */
	private static String[] listNames = new String[0];
	
	/** Edit distance threshold for fuzzy-lookups in dictionaries. */
	private static int fuzzyListLookupThreshold = 0;
	
	/**
	 * Initializes the list-based NE taggers.
	 * 
	 * @param listDirectory path of the directory the list files are located in
	 */
	public static void loadListTaggers(String listDirectory) {
		if (lists.length > 0) return;
		
		MsgPrinter.printStatusMsg("  ...loading lists");
		
		ArrayList<String> listsList = new ArrayList<String>(); 
		ArrayList<String> listNamesList = new ArrayList<String>();
		
		File[] listFiles = new File(listDirectory).listFiles(new FileFilter() {
			public boolean accept(File pathname) {
				return pathname.getName().endsWith(".lst");
			}
		});
		Arrays.sort(listFiles);
		
		for (File list : listFiles) {
			String listName = list.getName();
			listsList.add(list.getName());
			listName = listName.substring(0, (listName.length() - 4));
			listNamesList.add("NE" + listName);
			MsgPrinter.printStatusMsg("    ...for NE" + listName);
		}
		
		lists = listsList.toArray(new String[listsList.size()]);
		listNames = listNamesList.toArray(new String[listNamesList.size()]);
	}
	
	// =====================
	// Pattern-based taggers
	// =====================
	
	/** Regular expression patterns that match different types of NEs. */
	private static Pattern[] patterns = new Pattern[0];
	
	/** Maximum number of tokens per instance for the different types of NEs. */
	private static int[] patternMaxTokens = new int[0];
	
	/**
	 * NE types that are matched by the regular expressions. There may be
	 * multiple taggers for the same NE type. IMPORTANT: NE types must be
	 * prefix-free.
	 */
	private static String[] patternNames = new String[0];
	
	/**
	 * Regular expression patterns that match different types of quantity NEs
	 * (number + unit).
	 */
	private static Pattern[] quantityPatterns = new Pattern[0];
	
	/**
	 * Regular expression patterns that match different measurement units. 
	 */
	private static Pattern[] quantityUnitPatterns = new Pattern[0];
	
	/**
	 * Maximum number of tokens per instance for the different types of quantity
	 * units.
	 */
	private static int[] quantityUnitPatternMaxTokens = new int[0];
	
	/**
	 * NE types that are matched by the regular expressions. There may be
	 * multiple taggers for the same NE type. IMPORTANT: NE types must be
	 * prefix-free.
	 */
	private static String[] quantityPatternNames = new String[0];
	
	/** Collection of all NE types extracted with regular expressions. */ 
	private static String[] allPatternNames =
		new String[patterns.length + 1 + quantityUnitPatterns.length];
	
	/**
	 * Initializes the regular expression based NE taggers.
	 * 
	 * @param regExListFileName path and name of the file the names of the
	 *                          patterns in use are found in
	 */
	public static void loadRegExTaggers(String regExListFileName) {
		if (patterns.length > 0) return;
		
		MsgPrinter.printStatusMsg("  ...loading patterns");
		
		ArrayList<String> patternNameList = new ArrayList<String>();
		ArrayList<Pattern> patternList = new ArrayList<Pattern>();
		ArrayList<Integer> patternMaxTokensList = new ArrayList<Integer>();
		
		ArrayList<String> quantityPatternNameList = new ArrayList<String>();
		ArrayList<Pattern> quantityPatternList = new ArrayList<Pattern>();
		ArrayList<Integer> quantityPatternMaxTokensList = new ArrayList<Integer>();
		
		ArrayList<String> quantityUnitPatternNameList = new ArrayList<String>();
		ArrayList<Pattern> quantityUnitPatternList = new ArrayList<Pattern>();
		ArrayList<Integer> quantityUnitPatternMaxTokensList = new ArrayList<Integer>();
		
		try {
			BufferedReader br = new BufferedReader(new FileReader(regExListFileName));// new BufferedReader(new FileReader("./res/nlp/netagger/patterns.lst"));
			String line;
			while ((line = br.readLine()) != null) {
				String neName = "NE" + line;
				String patternFieldNamePrefix = "";
				for (int c = 0; c < line.length(); c++) {
					char ch = line.charAt(c);
					if (Character.isUpperCase(ch)) patternFieldNamePrefix += "_" + ch;
					else patternFieldNamePrefix += Character.toUpperCase(ch);
				}
				
				String regExFieldName = patternFieldNamePrefix;
				String patternFieldName = patternFieldNamePrefix + "_PATTERN";
				String maxTokensFieldName = patternFieldNamePrefix + "_MAX_TOKENS";
				
				try {
					Field regExField = RegExMatcher.class.getField(regExFieldName);
					Field patternField = RegExMatcher.class.getField(patternFieldName);
					Field maxTokensField = RegExMatcher.class.getField(maxTokensFieldName);
					
					String regEx = regExField.get(null).toString();
					Pattern pattern = ((Pattern) patternField.get(null));
					int maxTokens = maxTokensField.getInt(null);
					
					boolean isQuantity = ((regEx.indexOf(RegExMatcher.NUMBER) != -1) && !regEx.equals(RegExMatcher.NUMBER));
					
					if (isQuantity) {
						try {
							String unitPatternFieldName = patternFieldNamePrefix + "_UNIT_PATTERN";
							String unitMaxTokensFieldName = patternFieldNamePrefix + "_UNIT_MAX_TOKENS";
							
							Field unitPatternField = RegExMatcher.class.getField(unitPatternFieldName);
							Field unitMaxTokensField = RegExMatcher.class.getField(unitMaxTokensFieldName);
							
							Pattern unitPattern = ((Pattern) unitPatternField.get(null));
							int unitMaxTokens = unitMaxTokensField.getInt(null);
							
							quantityPatternNameList.add(neName);
							quantityPatternList.add(pattern);
							quantityPatternMaxTokensList.add(new Integer(maxTokens));
							
							quantityUnitPatternNameList.add(neName);
							quantityUnitPatternList.add(unitPattern);
							quantityUnitPatternMaxTokensList.add(new Integer(unitMaxTokens));
						} catch (Exception e) {
							isQuantity = false;
						}
					}
					
					if (!isQuantity) {
						patternNameList.add(neName);
						patternList.add(pattern);
						patternMaxTokensList.add(new Integer(maxTokens));
					}
					
					MsgPrinter.printStatusMsg("    ...for " + neName);
				} catch (Exception e) {
					MsgPrinter.printErrorMsg("    ...could not add " + neName);
				}
			}
			
			patternNames = new String[patternNameList.size()];
			patterns = new Pattern[patternList.size()];
			patternMaxTokens = new int[patternMaxTokensList.size()];
			for (int p = 0; p < patternNameList.size(); p++) {
				patternNames[p] = patternNameList.get(p);
				patterns[p] = patternList.get(p);
				patternMaxTokens[p] = patternMaxTokensList.get(p).intValue();
			}
			
			quantityPatternNames = new String[quantityPatternNameList.size()];
			quantityPatterns = new Pattern[quantityPatternList.size()];
			quantityUnitPatterns = new Pattern[quantityUnitPatternList.size()];
//			quantityPatternMaxTokens = new int[quantityPatternMaxTokensList.size()];
			quantityUnitPatternMaxTokens = new int[quantityUnitPatternMaxTokensList.size()];
			for (int p = 0; p < quantityPatternNameList.size(); p++) {
				quantityPatternNames[p] = quantityPatternNameList.get(p);
				quantityPatterns[p] = quantityPatternList.get(p);
				quantityUnitPatterns[p] = quantityUnitPatternList.get(p);
//				quantityPatternMaxTokens[p] = quantityPatternMaxTokensList.get(p);
				quantityUnitPatternMaxTokens[p] = quantityUnitPatternMaxTokensList.get(p);
			}
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		allPatternNames = new String[patterns.length + 1 + quantityUnitPatterns.length];
		for (int i = 0; i < patternNames.length; i++) allPatternNames[i] = patternNames[i];
		allPatternNames[patternNames.length] = "NEnumber";
		for (int i = 0; i < quantityPatternNames.length; i++) allPatternNames[patternNames.length + i + 1] = quantityPatternNames[i];
	}
	
	// =================
	// Tagger statistics
	// =================
	
	/**
	 * Returns the number of NE taggers.
	 * 
	 * @return number of name finders and regular expressions
	 */
	public static int getNumberOfTaggers() {
		return finderNames.length + allPatternNames.length + listNames.length + stanfordNames.length;
	}
	
	/**
	 * Returns the NE type that is recognized by the tagger with the given ID.
	 * 
	 * @param neId ID of a NE tagger
	 * @return corresponding NE type or <code>null</code>, if the ID is invalid
	 */
	public static String getNeType(int neId) {
		if (neId < 0) return null;
		
		if (neId < finderNames.length) return finderNames[neId];
		neId -= finderNames.length;
		
		if (neId < allPatternNames.length) return allPatternNames[neId];
		neId -= allPatternNames.length;
		
		if (neId < listNames.length) return listNames[neId];
		neId -= listNames.length;
		
		if (neId < stanfordNames.length) return stanfordNames[neId];
		
		return null;
	}
	
	/**
	 * Returns the IDs of the taggers for the given NE type (there may be more
	 * than one).
	 * 
	 * @param neType NE type
	 * @return IDs of the NE taggers
	 */
	public static int[] getNeIds(String neType) {
		ArrayList<Integer> idList = new ArrayList<Integer>();
		
		for (int i = 0; i < finderNames.length; i++)
			if (finderNames[i].equals(neType))
				idList.add(i);
		
		for (int i = 0; i < allPatternNames.length; i++)
			if (allPatternNames[i].equals(neType))
				idList.add(finderNames.length + i);
		
		for (int i = 0; i < listNames.length; i++)
			if (listNames[i].equals(neType))
				idList.add(finderNames.length + allPatternNames.length + i);
		
		for (int i = 0; i < stanfordNames.length; i++)
			if (stanfordNames[i].equals(neType))
				idList.add(finderNames.length + allPatternNames.length + listNames.length + i);
		
		int[] ids = new int[idList.size()];
		for (int i = 0; i < ids.length; i++) ids[i] = idList.get(i);
		
		return ids;
	}
	
	/**
	 * Checks if there is a model-based tagger for the given NE type.
	 * 
	 * @param neType NE type
	 * @return <code>true</code> iff there is a model-based tagger for this type
	 */
	public static boolean isModelType(String neType) {
		if (neType == null) return false;
		
		for (String modelType : MODEL_TYPES)
			if (neType.matches(modelType)) return true;
		
		return false;
	}
	
	/**
	 * Checks if there is a model-based tagger for one of the given NE types.
	 * 
	 * @param neTypes NE types
	 * @return <code>true</code> iff there is a model-based tagger for one of
	 *         these types
	 */
	public static boolean hasModelType(String[] neTypes) {
		if (neTypes == null) return false;
		
		for (String neType : neTypes)
			if (isModelType(neType)) return true;
		
		return false;
	}
	
	/**
	 * Checks if there is a model-based tagger for each of the given NE types.
	 * 
	 * @param neTypes NE types
	 * @return <code>true</code> iff there is a model-based tagger for each of
	 *         these types
	 */
	public static boolean allModelType(String[] neTypes) {
		if (neTypes == null) return false;
		
		for (String neType : neTypes)
			if (!isModelType(neType)) return false;
		
		return true;
	}
	
	/**
	 * Gets the current value of the edit distance threshold for fuzzy-lookups
	 * in dictionaries.
	 * 
	 * @return the current value of the fuzzy-lookups threshold
	 */
	public static int getFuzzyMatchingThreshold() {
		return fuzzyListLookupThreshold;
	}
	
	/**
	 * Sets the threshold for fuzzy-lookups in gazetteer lists (aka
	 * dictionaries). Setting the threshold to zero (the initial value) will
	 * disable fuzzy lookups. The extractNes() and tagNes() methods will then
	 * behave as they used to. Setting a higher threshold, in turn, will result
	 * in more strings extracted, thus in a certain tolerance with regard to
	 * typos in the documents. A side effect is a growth of the processing time
	 * for the extractNes() and tagNes() methods, especially for large
	 * dictionaries.
	 * 
	 * @param threshold the new value for the edit distance threshold for
	 *                  fuzzy-lookups in dictionaries
	 */
	public static void setFuzzyMatchingThreshold(int threshold) {
		fuzzyListLookupThreshold = threshold;
	}
	
	// ==========
	// NE tagging
	// ==========
	
	/**
	 * Adds named entity information to parses.
	 * 
	 * @param tag named entity type
	 * @param names spans of tokens that are named entities
	 * @param tokens parses for the tokens
	 */
	private static void addNames(String tag, List names, Parse[] tokens) {
		for (int i = 0; i < names.size(); i++) {
			Span nameTokenSpan = (Span) names.get(i);
			Parse startToken = tokens[nameTokenSpan.getStart()];
			Parse endToken = tokens[nameTokenSpan.getEnd()];
			Parse commonP = startToken.getCommonParent(endToken);
			
			if (commonP != null) {
				Span nameSpan = new Span(startToken.getSpan().getStart(),
										 endToken.getSpan().getEnd());
				
				if (nameSpan.equals(commonP.getSpan())) {
					// common parent matches exactly the named entity
					commonP.insert(new Parse(commonP.getText(), nameSpan, tag,
							1.0));
				} else {
					// common parent includes the named entity
					Parse[] kids = commonP.getChildren();
					boolean crossingKids = false;
					
					for (int j = 0; j < kids.length; j++)
						if (nameSpan.crosses(kids[j].getSpan()))
							crossingKids = true;
					
					if (!crossingKids) {
						// named entity does not cross children
						commonP.insert(new Parse(commonP.getText(), nameSpan,
								tag, 1.0));
					} else {
						// NE crosses children
						if (commonP.getType().equals("NP")) {
							Parse[] grandKids = kids[0].getChildren();
							
							Parse last = grandKids[grandKids.length - 1];
							if (grandKids.length > 1 &&
								nameSpan.contains(last.getSpan()))
								commonP.insert(new Parse(commonP.getText(),
										commonP.getSpan(), tag,1.0));
						}
					}
				}
			}
		}
	}
	
	/**
	 * Recursive method called by <code>extractNes(Parse)</code> to extract NEs
	 * from a parse tree augmented with NE tags.
	 * 
	 * @param parse a node of a parse tree
	 * @param nes NEs found so far
	 */
	private static void extractNesRec(Parse parse, ArrayList<String>[] nes) {
		String type = parse.getType();
		if (type.startsWith("NE")) {
			String text = parse.getText().substring(parse.getSpan().getStart(),
													parse.getSpan().getEnd());
			nes[getNeIds(type)[0]].add(text.trim());
		}
		
		for (Parse child : parse.getChildren())
			extractNesRec(child, nes);
	}
	
	/**
	 * A rule-based tokenizer used to prepare a sentence for NE extraction.
	 * 
	 * @param text text to tokenize
	 * @return array of tokens
	 */
	public static String[] tokenize(String text) {
		Span[] spans = NameFinder.tokenizeToSpans(text);
		return NameFinder.spansToStrings(spans, text);
	}
	
	/**
	 * Applies the rule-based tokenizer and concatenates the tokens with spaces.
	 * 
	 * @param text text to tokenize
	 * @return string of space-delimited tokens
	 */
	public static String tokenizeWithSpaces(String text) {
		String[] tokens = tokenize(text);
		return StringUtils.concatWithSpaces(tokens);
	}
	
	/** THIS METHOD IS NOT USED
	 * Performs named entity tagging on an array of (not tokenized) sentences.
	 * 
	 * @param sentences array of sentences
	 * @return array of tagged sentences
	 */
	// TODO avoid duplicate tags if there are multiple taggers for the same type
	@SuppressWarnings("unchecked")
	public static String[] tagNes(String[] sentences) {
		String[] results = new String[sentences.length];
		for (int s = 0; s < results.length; s++) results[s] = "";
		
		// initialize prevTokenMaps
		Map[] prevTokenMaps = new HashMap[finders.length];
		for (int i = 0; i < finders.length; i++)
			prevTokenMaps[i] = new HashMap();
		
		for (int s = 0; s < sentences.length; s++) {
			// tokenize sentence
			Span[] spans = NameFinder.tokenizeToSpans(sentences[s]);
			String[] tokens = tokenize(sentences[s]);
			
			// find named entities
			String[][] finderTags = new String[finders.length][];
			for (int i = 0; i < finders.length; i++)
				finderTags[i] = finders[i].find(tokens, prevTokenMaps[i]);
			
			// update prevTokenMaps
			for (int i = 0; i < prevTokenMaps.length; i++)
				for (int j = 0; j < tokens.length; j++)
					prevTokenMaps[i].put(tokens[j], finderTags[i][j]);
			
			// apply regular expressions
			String[][] regExTags = new String[patterns.length + 1 + quantityUnitPatterns.length][];
			
			//	don't tag NEproperName here
			regExTags[0] = new String[tokens.length];
			for (int i = 0; i < tokens.length; i++) regExTags[0][i] = NameFinderME.OTHER;
			
			for (int i = 1; i < patterns.length; i++)
				regExTags[i] = RegExMatcher.markAllMatches(tokens, patterns[i], patternMaxTokens[i]);
			
			String[] numberMarkers = RegExMatcher.extractNumbers(tokens);
			regExTags[patterns.length] = numberMarkers;
			
			for (int i = 0; i < quantityUnitPatterns.length; i++)
				regExTags[patterns.length + i + 1] = RegExMatcher.extractQuantities(tokens, numberMarkers, quantityUnitPatterns[i], quantityUnitPatternMaxTokens[i]);
			
			//	apply lists
			String[][] listTags = new String[lists.length][];
			for (int i = 0; i < lists.length; i++)
				listTags[i] = RegExMatcher.markAllContained(tokens, RegExMatcher.getDictionary(lists[i]), fuzzyListLookupThreshold);
			
			for (int i = 0; i < tokens.length; i++) {
				//check for end tags
				for (int j = 0; j < finders.length; j++)
					if (i != 0)
						if ((finderTags[j][i].equals(NameFinderME.START) ||
							finderTags[j][i].equals(NameFinderME.OTHER)) &&
							(finderTags[j][i - 1].equals(NameFinderME.START) ||
							finderTags[j][i - 1].equals(NameFinderME.CONTINUE)))
							results[s] += "</" + finderNames[j] + ">";
				
				//check for end tags
				for (int j = 0; j < allPatternNames.length; j++)
					if (i != 0)
						if ((regExTags[j][i].equals(NameFinderME.START) ||
							regExTags[j][i].equals(NameFinderME.OTHER)) &&
							(regExTags[j][i - 1].equals(NameFinderME.START) ||
							regExTags[j][i - 1].equals(NameFinderME.CONTINUE)))
							results[s] += "</" + allPatternNames[j] + ">";
				
				//check for end tags
				for (int j = 0; j < listNames.length; j++)
					if (i != 0)
						if ((regExTags[j][i].equals(NameFinderME.START) ||
							regExTags[j][i].equals(NameFinderME.OTHER)) &&
							(regExTags[j][i - 1].equals(NameFinderME.START) ||
							regExTags[j][i - 1].equals(NameFinderME.CONTINUE)))
							results[s] += "</" + listNames[j] + ">";
				
				if (i > 0 && spans[i - 1].getEnd() < spans[i].getStart())
					results[s] += sentences[s].substring(spans[i - 1].getEnd(),
														 spans[i].getStart());
				
				//check for start tags
				for (int j = 0; j < finders.length; j++)
					if (finderTags[j][i].equals(NameFinderME.START))
						results[s] += "<" + finderNames[j] + ">";
				
				//check for start tags
				for (int j = 0; j < allPatternNames.length; j++)
					if (regExTags[j][i].equals(NameFinderME.START))
						results[s] += "<" + allPatternNames[j] + ">";
				
				//check for start tags
				for (int j = 0; j < listNames.length; j++)
					if (regExTags[j][i].equals(NameFinderME.START))
						results[s] += "<" + listNames[j] + ">";
				
		        results [s]+= tokens[i];
			}
			
			if (tokens.length != 0) {
				int last = tokens.length - 1;
	
				//final end tags
				for (int i = 0; i < finders.length; i++)
					if (finderTags[i][last].equals(NameFinderME.START) ||
						finderTags[i][last].equals(NameFinderME.CONTINUE))
						results[s] += "</" + finderNames[i] + ">";
				
				//final end tags
				for (int i = 0; i < allPatternNames.length; i++)
					if (regExTags[i][last].equals(NameFinderME.START) ||
						regExTags[i][last].equals(NameFinderME.CONTINUE))
						results[s] += "</" + allPatternNames[i] + ">";
				
				//final end tags
				for (int i = 0; i < listNames.length; i++)
					if (regExTags[i][last].equals(NameFinderME.START) ||
						regExTags[i][last].equals(NameFinderME.CONTINUE))
						results[s] += "</" + listNames[i] + ">";
				
				if (spans[last].getEnd() < sentences[s].length())
					results[s] += sentences[s].substring(spans[last].getEnd());
			}
		}
		
		return results;
	}
	
	/**
	 * Performs named entity tagging on an array of full parses of sentences.
	 * 
	 * @param parses array of full parses of sentences
	 */
	// TODO only works with OpenNLP taggers so far
	@SuppressWarnings("unchecked")
	public static void tagNes(Parse[] parses) {
		String[] results = new String[parses.length];
		for (int s = 0; s < results.length; s++) results[s] = "";
		
		// initialize prevTokenMaps
		Map[] prevTokenMaps = new HashMap[finders.length];
		for (int i = 0; i < finders.length; i++)
			prevTokenMaps[i] = new HashMap();
		
		for (Parse parse : parses) {
			// get tokens
			Parse[] tokens = parse.getTagNodes();
			
			// find named entites
			String[][] finderTags = new String[finders.length][];
			for (int i = 0; i < finders.length; i++)
				finderTags[i] = finders[i].find(tokens, prevTokenMaps[i]);
			
			// update prevTokenMaps
			for (int i = 0; i < prevTokenMaps.length; i++)
				for (int j = 0; j < tokens.length; j++)
					prevTokenMaps[i].put(tokens[j], finderTags[i][j]);
			
			for (int i = 0; i < finders.length; i++) {
				int start = -1;
				List<Span> names = new ArrayList<Span>(5);
				
				// determine spans of tokens that are named entities
				for (int j = 0; j < tokens.length; j++) {
					if ((finderTags[i][j].equals(NameFinderME.START) ||
						 finderTags[i][j].equals(NameFinderME.OTHER))) {
						if (start != -1) names.add(new Span(start, j - 1));
						start = -1;
					}
					if (finderTags[i][j].equals(NameFinderME.START)) start = j;
				}
				if (start != -1) names.add(new Span(start, tokens.length - 1));
				
				// add name entity information to parse
				addNames(finderNames[i], names, tokens);
			}
	    }
	}
	
	/**
	 * Extracts NEs from an array of tokenized sentences.
	 * 
	 * @param sentences array of tokenized sentences
	 * @return NEs per sentence and NE type
	 */
	// TODO only works with OpenNLP taggers, lists and patterns so far
	@SuppressWarnings("unchecked")
	public static String[][][] extractNes(String[][] sentences) {
		String[][][] nes = new String[sentences.length][][];
		
		// initialize prevTokenMaps
		Map[] prevTokenMaps = new HashMap[finders.length];
		for (int i = 0; i < finders.length; i++)
			prevTokenMaps[i] = new HashMap();
		
		for (int s = 0; s < sentences.length; s++) {
			String[] tokens = sentences[s];
			nes[s] = new String[finders.length + allPatternNames.length + lists.length + stanfordNames.length][];
			
			// find named entities
			String[][] finderTags = new String[finders.length][];
			for (int i = 0; i < finders.length; i++)
				finderTags[i] = finders[i].find(tokens, prevTokenMaps[i]);
			
			// update prevTokenMaps
			for (int i = 0; i < prevTokenMaps.length; i++)
				for (int j = 0; j < tokens.length; j++)
					prevTokenMaps[i].put(tokens[j], finderTags[i][j]);
			
			// extract named entities
			for (int i = 0; i < finders.length; i++) {
				ArrayList<String> neList = new ArrayList<String>();
				
				String ne = "";
				for (int j = 0; j < tokens.length; j++) {
					if ((finderTags[i][j].equals(NameFinderME.START) ||
						finderTags[i][j].equals(NameFinderME.OTHER)) &&
						ne.length() > 0) {
						neList.add(ne.trim());
						ne = "";
					}
					
					if (finderTags[i][j].equals(NameFinderME.START))
						ne = tokens[j];
					
					if (finderTags[i][j].equals(NameFinderME.CONTINUE))
						ne += " " + tokens[j];
				}
		        if (ne.length() > 0) neList.add(ne);
		        
		        nes[s][i] = neList.toArray(new String[neList.size()]);
			}
			
			// apply regular expressions
			String[][] regExTags = new String[allPatternNames.length][];
			
			//	don't tag NEproperName here
			regExTags[0] = new String[tokens.length];
			for (int i = 0; i < tokens.length; i++) regExTags[0][i] = NameFinderME.OTHER;
			
			for (int i = 1; i < patterns.length; i++)
				regExTags[i] = RegExMatcher.markAllMatches(tokens, patterns[i], patternMaxTokens[i]);
			
			String[] numberMarkers = RegExMatcher.extractNumbers(tokens);
			regExTags[patterns.length] = numberMarkers;
			
			for (int i = 0; i < quantityUnitPatterns.length; i++)
				regExTags[patterns.length + i + 1] = RegExMatcher.extractQuantities(tokens, numberMarkers, quantityUnitPatterns[i], quantityUnitPatternMaxTokens[i]);
			
			for (int i = 0; i < allPatternNames.length; i++) {
				ArrayList<String> neList = new ArrayList<String>();
				
				String ne = "";
				for (int j = 0; j < tokens.length; j++) {
					if ((regExTags[i][j].equals(NameFinderME.START) ||
							regExTags[i][j].equals(NameFinderME.OTHER)) &&
						ne.length() > 0) {
						neList.add(ne.trim());
						ne = "";
					}
					
					if (regExTags[i][j].equals(NameFinderME.START))
						ne = tokens[j];
					
					if (regExTags[i][j].equals(NameFinderME.CONTINUE))
						ne += " " + tokens[j];
				}
		        if (ne.length() > 0) neList.add(ne);
		        
		        nes[s][finders.length + i] = neList.toArray(new String[neList.size()]);
			}
			
			// apply lists
			String[][] listTags = new String[listNames.length][];
			for (int i = 0; i < lists.length; i++)
				listTags[i] = RegExMatcher.markAllContained(tokens, RegExMatcher.getDictionary(lists[i]), fuzzyListLookupThreshold);
			
			for (int i = 0; i < lists.length; i++) {
				ArrayList<String> neList = new ArrayList<String>();
				
				String ne = "";
				for (int j = 0; j < tokens.length; j++) {
					if ((listTags[i][j].equals(NameFinderME.START) ||
							listTags[i][j].equals(NameFinderME.OTHER)) &&
						ne.length() > 0) {
						neList.add(ne.trim());
						ne = "";
					}
					
					if (listTags[i][j].equals(NameFinderME.START))
						ne = tokens[j];
					
					if (listTags[i][j].equals(NameFinderME.CONTINUE))
						ne += " " + tokens[j];
				}
		        if (ne.length() > 0) neList.add(ne);
		        
		        nes[s][finders.length + allPatternNames.length + i] = neList.toArray(new String[neList.size()]);
			}
			
                    /*    PrintWriter pw = null;   
                        try {
                            pw = new PrintWriter(new FileOutputStream(new File("StanfordNeTagger_data.txt"),true));
                        } catch (FileNotFoundException ex) {
                            System.out.println("File not found exception!!");
                        }*/

			//	apply stanford tagger
			HashMap <String, String[]> allStanfordNEs = StanfordNeTagger.extractNEs(StringUtils.concatWithSpaces(sentences[s]));
                        
                        //pw.printf("%s\n", StringUtils.concatWithSpaces(sentences[s]));
                      //  pw.printf("%s ----- %s\n", StringUtils.concatWithSpaces(sentences[s]), nes.toString());
                        
			for (int i = 0; i < stanfordNames.length; i++) {
				String[] stanfordNEs = allStanfordNEs.get(stanfordNames[i]);
				if (stanfordNEs == null) stanfordNEs = new String[0];
				nes[s][finders.length + allPatternNames.length + lists.length + i] = stanfordNEs;
			}
                        //pw.close();
		}
		
		return nes;
	}
	
	/**
	 * Extracts NEs of a particular type from an array of tokenized sentences.
	 * 
	 * @param sentences array of tokenized sentences
	 * @param neId ID of a name finder or regular expression
	 * @return NEs of the particular type per sentence or <code>null</code>, if
	 * 		   the ID is invalid
	 */
	@SuppressWarnings("unchecked")
	public static String[][] extractNes(String[][] sentences, int neId) {
		if (neId < 0 || neId >= finderNames.length + allPatternNames.length + listNames.length + stanfordNames.length)
			return null;  // invalid ID
		
		String[][] nes = new String[sentences.length][];
		
		if (neId < finderNames.length) {
			// initialize prevTokenMap
			Map prevTokenMap = new HashMap();
			
			for (int s = 0; s < sentences.length; s++) {
				String[] tokens = sentences[s];
				
				// find named entities
				String[] tags = finders[neId].find(tokens, prevTokenMap);
				
				// update prevTokenMap
				for (int i = 0; i < tokens.length; i++)
					prevTokenMap.put(tokens[i], tags[i]);
				
				// extract named entities
				ArrayList<String> neList = new ArrayList<String>();
				String ne = "";
				for (int i = 0; i < tokens.length; i++) {
					if ((tags[i].equals(NameFinderME.START) ||
						tags[i].equals(NameFinderME.OTHER)) &&
						ne.length() > 0) {
						neList.add(ne.trim());
						ne = "";
					}
					
					if (tags[i].equals(NameFinderME.START))
						ne = tokens[i];
					
					if (tags[i].equals(NameFinderME.CONTINUE))
						ne += " " + tokens[i];
				}
				if (ne.length() > 0) neList.add(ne);
				
			    nes[s] = neList.toArray(new String[neList.size()]);
			}
		} else {
			// adjust ID
			int i = neId - finderNames.length;
			
			if (i < allPatternNames.length) {
				
				//	select pattern
				Pattern regEx;
				if (i < patterns.length) {
					regEx = patterns[i];
				} else if (i == patterns.length) {
					regEx = RegExMatcher.NUMBER_PATTERN;
				} else {
					regEx = quantityPatterns[i - patterns.length - 1];
				}
				
				for (int s = 0; s < sentences.length; s++) {
					// apply regular expression
					String sentence = StringUtils.concatWithSpaces(sentences[s]);
					nes[s] = RegExMatcher.extractAllMatches(sentence, regEx);
				}
				
			} else {
				i -= allPatternNames.length;
				
				if (i < listNames.length) {
					for (int s = 0; s < sentences.length; s++)
						nes[s] = RegExMatcher.extractAllContained(sentences[s], RegExMatcher.getDictionary(lists[i]), fuzzyListLookupThreshold);
					
				} else {
					i -= listNames.length;
					
					//	apply stanford tagger
					for (int s = 0; s < sentences.length; s++) {
						HashMap <String, String[]> allStanfordNEs = StanfordNeTagger.extractNEs(StringUtils.concatWithSpaces(sentences[s]));
						String[] stanfordNEs = allStanfordNEs.get(stanfordNames[i]);
						if (stanfordNEs == null) stanfordNEs = new String[0];
						nes[s] = stanfordNEs;
					}
				}
			}
		}
		
		return nes;
	}
	
	/** THIS METHOD IS NOT USED 
	 * Extracts NEs from a parse tree that has been augmented with NE tags.
	 * 
	 * @param parse a parse tree augmented with NE tags
	 * @return NEs per NE type
	 */
	// TODO only works with OpenNLP taggers so far
	@SuppressWarnings("unchecked")
	public static String[][] extractNes(Parse parse) {
		// initialize dynamic arrays
		ArrayList[] nes = new ArrayList[finders.length];
		for (int i = 0; i < nes.length; i++) nes[i] = new ArrayList();
		
		// depth-first search on the parse tree
		extractNesRec(parse, nes);
		
		// copy to static arrays
		String[][] results = new String[finders.length][];
		for (int i = 0; i < nes.length; i++)
			results[i] = (String[]) nes[i].toArray(new String[nes[i].size()]);
		
		return results;
	}
}

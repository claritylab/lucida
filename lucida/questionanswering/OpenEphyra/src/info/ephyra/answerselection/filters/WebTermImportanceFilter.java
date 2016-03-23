package info.ephyra.answerselection.filters;

import info.ephyra.io.Logger;
import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.OpenNLP;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.StanfordNeTagger;
import info.ephyra.nlp.indices.WordFrequencies;
import info.ephyra.querygeneration.Query;
import info.ephyra.querygeneration.generators.BagOfWordsG;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.questionanalysis.KeywordExtractor;
import info.ephyra.questionanalysis.QuestionNormalizer;
import info.ephyra.search.Result;
import info.ephyra.trec.TREC13To16Parser;
import info.ephyra.trec.TRECTarget;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashSet;

/**
 * <p>A web reinforcement approach that ranks answer candidates for definitional
 * questions. Several variations of the target of the question are generated and
 * are used to retrieve relevant text snippets from the web. The frequencies of
 * content words in these snippets are counted and the scores of the answers are
 * adjusted to assign higher scores to candidates that cover frequent keywords.
 * This approach is based on the assumption that terms that often cooccur with
 * the target provide relevant information on the target that should be covered 
 * by the answers.</p>
 * 
 * <p>Several instances of this web term importance filter have been implemented
 * that use different sources for text snippets.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public abstract class WebTermImportanceFilter extends Filter {
	
	protected static final String person = "person";
	protected static final String organization = "organization";
	protected static final String location = "location";
	protected static final String event = "event";
	
	public static final int NO_NORMALIZATION = 0;
	public static final int LINEAR_LENGTH_NORMALIZATION = 1;
	public static final int SQUARE_ROOT_LENGTH_NORMALIZATION = 2;
	public static final int LOG_LENGTH_NORMALIZATION = 3;
	public static final int LOG_10_LENGTH_NORMALIZATION = 4;
	
	private final int normalizationMode;
	private final int tfNormalizationMode;
	private final boolean isCombined;
	
//	protected static final String WIKIPEDIA = "wikipedia";
	
	/**
	 */
	protected WebTermImportanceFilter(int normalizationMode, int tfNormalizationMode, boolean isCombined) {
		this.normalizationMode = normalizationMode;
		this.tfNormalizationMode = tfNormalizationMode;
		this.isCombined = isCombined;
	}

	/**
	 * fetch the term frequencies in the top X result snippets of a web search
	 * for some target
	 * 
	 * @param targets an array of strings containing the targets
	 * @return a HashMap mapping the terms in the web serach results to their
	 *         frequency in the snippets
	 */
	public abstract HashMap<String, TermCounter> getTermCounters(String[] targets);
	
	/**
	 * @author sautter
	 * 
	 * Mutable integer class to avoid creating new objects all the time
	 */
	protected class TermCounter {
		private int value = 0;
		
		/** Constructor
		 */
		protected TermCounter() {}
		
		/**
		 * Constructor
		 * @param value the initial value
		 */
		protected TermCounter(int value) {
			this.value = value;
		}
		
		/**	@return	the value of this TermCounter
		 */
		public int getValue() {
			return this.value;
		}
		
		/**	increment the value of this TermCounter by 1
		 */
		public void increment() {
			this.value++;
		}
		
		/** increment the value of this TermCounter by <code>inc</code>
		 * @param inc 
		 */
		public void increment(int inc) {
			this.value += inc;
		}
		
		/**	decrement the value of this TermCounter by 1
		 */
		public void decrement() {
			this.value--;
		}
		
		/** decrement the value of this TermCounter by <code>dec</code>
		 * @param dec 
		 */
		public void decrement(int dec) {
			this.value -= dec;
		}
		
		/** multiply the value of this TermCounter times <code>fact</code>
		 * @param fact 
		 */
		public void multiplyValue(int fact) {
			this.value *= fact;
		}
		
		/** devide the value of this TermCounter times <code>denom</code>
		 * @param denom 
		 */
		public void divideValue(int denom) {
			this.value /= denom;
		}
	}
	
	/**
	 * produce the target variations for a given target
	 * 
	 * @param target the original traget String
	 * @return an array of strings containing the variations of the target
	 *         String, including the original target
	 */
	public String[] getTargets(String target) {
		ArrayList<String> targets = new ArrayList<String>();
		targets.add(target);
		
		boolean isPerson = false;
		boolean brackets = false;
		
		// If target starts with "the", "a", or "an", remove it.
		if (target.startsWith("the ")) {
			targets.add(target.substring(4, target.length()));
		} else if (target.startsWith("an ")) {
			targets.add(target.substring(3, target.length()));
		} else if (target.startsWith("a ")) {
			targets.add(target.substring(2, target.length()));
		}
		
		String targetType = this.checkType(target);
		if (TEST_TARGET_GENERATION) {
			if (targetType == null) System.out.println(" target type could not be determined");
			else System.out.println(" target type is " + targetType);
		}
		if (person.equals(targetType)) {
			// (complete) target is of type Person, no further processing is necessary
			isPerson = true;
			
		// split parts in brackets from parts not in brackets:
		// "Norwegian Cruise Lines (NCL)" --> "Norwegian Cruise Lines" + "NCL"
		} else if (target.contains("(") && target.contains(")")) {
			int i1 = target.indexOf("(");
			int i2 = target.indexOf(")");
			String s1 = target.substring(0, i1 - 1);
			String s2 = target.substring(i1 + 1, i2);
			// Log.println("*** '"+s1+"' '"+s2+"'", true);
			targets.clear();
			targets.add(s1);
			targets.add(s2);
			// Log.println(" "+target+" contains brackest. No further processing
			// necessary.", true);
			brackets = true;
		} else if (this.cutExtension(target, targets)) {
			//	do nothing, it's in the cutExtensions method
			
		} else if (target.endsWith("University")) {
			// chop off "University"
			String toAdd = target.substring(0, target.length() - 11);
			targets.add(toAdd);
		} else if (target.endsWith("International")) {
			// chop off International"
			String toAdd = target.substring(0, target.length() - 14);
			targets.add(toAdd);
		} else if (target.endsWith("Corporation")) {
			// chop off "Corporation"
			String toAdd = target.substring(0, target.length() - 12);
			targets.add(toAdd);
		} else {
			this.extractUpperCaseParts(targets);
			HashSet<String> duplicateFreeTargets = new LinkedHashSet<String>(targets);
			for (Iterator<String> iter = duplicateFreeTargets.iterator(); iter.hasNext();) {
				String item = iter.next();
				String type = this.checkType(item);
				if (person.equals(type)) {
					// after removing the first NP, check again if target is
					// Person (example: "philanthropist Alberto Vilar")
					// Log.println(" "+item+" is Person. No further processing
					// necessary.", true);
					
					
					//	attention, this also discarts events containing person names!!!
					//	maybe remove this call
					//targets.clear();
					
					targets.add(item);
				}
			}
		}
		
		if (isPerson) {
			targets.add("\"" + target + "\"");
			
//			//	own extension: add 'wikipedia' to target
//			targets.add(target + " " + WIKIPEDIA);
//			targets.add("\"" + target + "\" " + WIKIPEDIA);
			
		} else if (!brackets) { //	maybe remove condition
			
			//targets = this.processLongTargets(targets);
			this.extractUpperCaseParts(targets);
			
			//targets = this.checkForEvent(targets);
			//	described effect done in extractUpperCaseParts(), uses NLP stuff we don't have
			
			//targets = this.checkForDeterminer(targets);
			//	bad thing, uses to many miraculous external classen we don't have
			
			//targets = this.removeAttachedPP(targets);
			//	done in extractUpperCaseParts()
			
			//targets = this.cutFirstNpInNpSequence(targets);
			this.cutFirstNpInNpSequence(targets);
			
			//targets = this.removeNounAfterNounGroup(targets);
			//	done in extractUpperCaseParts()
			
			//	own extension: extract acronyms 'Basque ETA' --> 'ETA'
			this.extractAcronyms(targets);
			
			//targets = this.postProcess(targets);
			this.postProcess(targets);
		}
    	
		HashSet<String> duplicateFreeTargets = new LinkedHashSet<String>(targets);
		for (Iterator<String> iter = duplicateFreeTargets.iterator(); iter.hasNext();) {
			String item = iter.next();
			String type = this.checkType(item);
			if (organization.equals(type)/* && !brackets*/) {
				targets.add("the " + item);
				if (!brackets) 
					targets.add("the " + target);
				
			} else if (person.equals(type)) {
				targets.add("\"" + item + "\"");
				
//				//	own extension: add 'wikipedia' to target
//				targets.add(item + " " + WIKIPEDIA);
//				targets.add("\"" + item + "\" " + WIKIPEDIA);
			}
			
			//	own extension: add determiner to acronyms
			if (item.matches("([A-Z]){3,}"))
				targets.add("the " + item);
			else if (item.matches("([A-Z]\\.){2,}"))
				targets.add("the " + item);
		}
		
		//	own extension: add quoted version of title case targets like 'The Daily Show'
		duplicateFreeTargets = new LinkedHashSet<String>(targets);
		for (Iterator<String> iter = duplicateFreeTargets.iterator(); iter.hasNext();) {
			String item = iter.next();
			if (item.matches("([A-Z][a-z]++)++")) {
				targets.add("\"" + item + "\"");
				
//				//	own extension: add 'wikipedia' to target
//				targets.add(item + " " + WIKIPEDIA);
//				targets.add("\"" + item + "\" " + WIKIPEDIA);
			}
		}
		
		//	own extension: always use quoted version of original target if it has more than one word
		String[] targetTokens = NETagger.tokenize(target);
		if (targetTokens.length > 1) {
			targets.add("\"" + target + "\"");
			
//			//	own extension: add 'wikipedia' to target
//			targets.add(target + " " + WIKIPEDIA);
//			targets.add("\"" + target + "\" " + WIKIPEDIA);
		}
		
		duplicateFreeTargets = new LinkedHashSet<String>(targets);
		return duplicateFreeTargets.toArray(new String[duplicateFreeTargets.size()]);
	}
	
	/** 
	 * find the NE type of a target
	 * 
	 * @param target the target String to check
	 * @return the NE type of target, or null, if the type couldn't be determined
	 */
	private String checkType(String target) {
		if (!StanfordNeTagger.isInitialized()) StanfordNeTagger.init();
		HashMap<String, String[]> nesByType = StanfordNeTagger.extractNEs(target);
		ArrayList<String> neTypes = new ArrayList<String>(nesByType.keySet());
		for (int t = 0; t < neTypes.size(); t++) {
			String type = neTypes.get(t);
			String[] nes = nesByType.get(type);
			for (int n = 0; n < nes.length; n++)
				if (nes[n].equals(target))
					return type.replace("NE", "");
		}
		return null;
	}
	
	/**
	 * cut tailing words like "University", "International", "Corporation":
	 * "Microsoft Corporation" --> "Microsoft" and add the non-cut part to target list
	 * 
	 * @param target the target String to cut
	 * @param targets the target list to add the cut part to
	 * @return true if a cut target was added, false otherwise
	 */
	private boolean cutExtension(String target, ArrayList<String> targets) {
		if (this.extensionList.isEmpty())
			for (int i = 0; i < extensions.length; i++)
				this.extensionList.add(extensions[i]);
			
		String[] targetTokens = target.split("\\s");
		String last = targetTokens[targetTokens.length - 1];
		if (this.extensionList.contains(last) && (targetTokens.length > 1)) {
			String cutTarget = targetTokens[0];
			for (int i = 1; i < (targetTokens.length - 1); i++)
				cutTarget += " " + targetTokens[i];
			targets.add(cutTarget);
			return true;
		}
		return false;
	}
	
	private HashSet<String> extensionList = new HashSet<String>();
	private static final String[] extensions = {
		"University",
		"Corporation",
		"International",
			//	last year's winner's list ends here
		"Incorporated",
		"Inc.",
		"Comp.",
		"Corp.",
		"Co.",
		"Museum",
		"<to be extended>"
	};
	
	/**	extract non lower case parts from the targets:
	 * "the film 'Star Wars'" --> "'Star Wars'"
	 * "1998 indictment and trial of Susan McDougal" --> "Susan McDougal"
	 * "Miss Universe 2000 crowned" --> "Miss Universe 2000"
	 * "Abraham from the bible" --> "Abraham"
	 * "Gobi desert" --> "Gobi"
	 * 
	 * @param targets the list of targets
	 */
	private void extractUpperCaseParts(ArrayList<String> targets) {
		HashSet<String> duplicateFreeTargets = new LinkedHashSet<String>(targets);
		for (Iterator<String> iter = duplicateFreeTargets.iterator(); iter.hasNext();) {
			String target = iter.next();
			String[] targetTokens = target.split("\\s");
			
			String upperCasePart = null;
			int i = 0;
			
			while (i < targetTokens.length) {
				
				//	find start of next upper case part
				while ((i < targetTokens.length) && !Character.isUpperCase(targetTokens[i].charAt(0))) i++;
				
				//	start upper case part
				if (i < targetTokens.length) {
					upperCasePart = targetTokens[i];
					i++;
				}
				
				//	collect non-lower-case part
				while ((i < targetTokens.length) && !Character.isLowerCase(targetTokens[i].charAt(0))) {
					upperCasePart += " " +  targetTokens[i];
					i++;
				}
				
				if (upperCasePart != null) {
					targets.add(upperCasePart);
					upperCasePart = null;
				}
			}
		}
	}
	
	/**	extract acronyms from the targets:
	 * "Basque ETA" --> "ETA"
	 * 
	 * @param targets the list of targets
	 */
	private void extractAcronyms(ArrayList<String> targets) {
		HashSet<String> duplicateFreeTargets = new LinkedHashSet<String>(targets);
		for (Iterator<String> iter = duplicateFreeTargets.iterator(); iter.hasNext();) {
			String target = iter.next();
			String[] targetTokens = target.split("\\s");
			for (String t : targetTokens) {
				if (t.matches("([A-Z]){3,}")) {
					targets.add(t);
				} else if (t.matches("([A-Z]\\.){2,}")) {
					targets.add(t);
				}
			}
		}
	}
	
	/**	remove first NP in a sequence of NPs:
	 * "the film 'Star Wars'" --> "'Star Wars'"
	 * 
	 * @param targets the list of targets
	 */
	private void cutFirstNpInNpSequence(ArrayList<String> targets) {
		HashSet<String> duplicateFreeTargets = new LinkedHashSet<String>(targets);
		for (Iterator<String> iter = duplicateFreeTargets.iterator(); iter.hasNext();) {
			String target = iter.next();
			
			//	tokenize and tag sentence
			String[] targetTokens = OpenNLP.tokenize(target);
			String[] posTags = OpenNLP.tagPos(targetTokens);
			String[] chunkTags = OpenNLP.tagChunks(targetTokens, posTags);
			
			String np = null;
			int i = 0;
			
			//	find first NP
			while ((i < targetTokens.length) && !"B-NP".equals(chunkTags[i])) i++;
			
			//	skip first NP
			i++;
			
			//	find next NP
			while (( i < targetTokens.length) && !"B-NP".equals(chunkTags[i])) i++;
			
			//	start NP
			if (i < targetTokens.length) {
				np = targetTokens[i];
				i++;
			}
			
			//	add rest of NP
			while (i < targetTokens.length) {
				np += " " +  targetTokens[i];
				i++;
			}
			
			if (np != null) targets.add(np);
		}
	}
	
	/**	take care of remaining brackets
	 * 
	 * @param targets the list of targets
	 */
	private void postProcess(ArrayList<String> targets) {
		HashSet<String> duplicateFreeTargets = new LinkedHashSet<String>(targets);
		targets.clear();
		
		for (Iterator<String> iter = duplicateFreeTargets.iterator(); iter.hasNext();) {
			String target = iter.next().trim();
			boolean add = true;
			if (target.startsWith("(") && target.endsWith(")"))
				target = target.substring(1, target.length() - 1).trim();
			
			if (target.startsWith("(") != target.endsWith(")")) add = false;
			
			//	own extension: cut leading and tailing apostrophes
			while (target.startsWith("'")) target = target.substring(1).trim();
			while (target.endsWith("'")) target = target.substring(0, (target.length() - 1)).trim();
			
			//	own extension: cut leading singel letters, but keep determiner "a"
			while (target.matches("[b-z]\\s.++")) target = target.substring(2);
			
			//	own extension: filter one-char targets
			if (target.length() < 2) add = false;
			
			if (add) targets.add(target);
		}
	}
	
	/**
	 * Increment the score of each result snippet for each word in it according
	 * to the number of top-100 web search engine snippets containing this
	 * particular word. This favors snippets that provide information given
	 * frequently and thus likely to be more important with regard to the
	 * target.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	@SuppressWarnings("unchecked")
	public Result[] apply(Result[] results) {
		
		//	catch empty result 
		if (results.length == 0) return results;
		
		//	produce target variations
		String target = results[0].getQuery().getOriginalQueryString();
		
		System.out.println("WebTermImportanceFilter:\n processing target '" + target + "'");
		
		HashMap<String, TermCounter> rawTermCounters = this.cacheLookup(target);
		
		//	query generation test
		if (TEST_TARGET_GENERATION) {
			String[] targets = this.getTargets(target);
			
			System.out.println(" generated web serach Strings:");
			for (String t : targets) System.out.println(" - " + t);
			
			//	query generation test only
			return results;
			
		//	cache miss
		} else if (rawTermCounters == null) {
			
			String[] targets = this.getTargets(target);
			System.out.println(" web serach Strings are");
			for (String t : targets) System.out.println(" - " + t);
			
			rawTermCounters = this.getTermCounters(targets);
			this.cache(target, rawTermCounters);
		}
		
		//	get target tokens
		HashSet<String> rawTargetTerms = new HashSet<String>();
		String[] targetTokens = OpenNLP.tokenize(target);
		for (String tt : targetTokens)
			if (Character.isLetterOrDigit(tt.charAt(0)))
				rawTargetTerms.add(tt);
		
		//	stem terms, collect target terms
		HashMap<String, TermCounter> termCounters = new HashMap<String, TermCounter>();//this.getTermCounters(targets);
		HashSet<String> targetTerms = new HashSet<String>();
		ArrayList<String> rawTerms = new ArrayList<String>(rawTermCounters.keySet());
		for (String rawTerm : rawTerms) {
			
			String stemmedTerm = SnowballStemmer.stem(rawTerm.toLowerCase());
			if (!termCounters.containsKey(stemmedTerm))
				termCounters.put(stemmedTerm, new TermCounter());
			termCounters.get(stemmedTerm).increment(rawTermCounters.get(rawTerm).getValue());
			
			if (rawTargetTerms.contains(rawTerm))
				targetTerms.add(stemmedTerm);
		}
		
		//	get overall recall (since 20070718)
		int termCount = this.getCountSum(termCounters);
		int termCountLog = ((termCount > 100) ? ((int) Math.log10(termCount)) : 2);
		System.out.println("WebTermImportanceFilter: termCountLog is " + termCountLog);
		
		//	score results
		ArrayList<Result> resultList = new ArrayList<Result>();
		boolean goOn;
		do {
			goOn = false;
			ArrayList<Result> rawResults = new ArrayList<Result>();
			
			//	score all results
			for (Result r : results) {
				if (r.getScore() != Float.NEGATIVE_INFINITY) {
					
					//	tokenize sentence
					String[] sentence = NETagger.tokenize(r.getAnswer());
					float importance = 0;
					
					//	scan sentence for terms from web result
					for (int i = 0; i < sentence.length; i++) {
						String term = sentence[i];
						if ((term.length() > 1)/* && !StringUtils.isSubsetKeywords(term, r.getQuery().getAnalyzedQuestion().getQuestion()) && !FunctionWords.lookup(term)*/) {
							term = SnowballStemmer.stem(term.toLowerCase());
							TermCounter count = termCounters.get(term);
							if (count != null) {
								double tf; // 20070706
								if (this.tfNormalizationMode == NO_NORMALIZATION) tf = 1;
								else if (this.tfNormalizationMode == LOG_LENGTH_NORMALIZATION) {
									tf = WordFrequencies.lookup(sentence[i].toLowerCase());
									if (tf > Math.E) tf = Math.log(tf);
									else tf = 1;
								} else if (this.tfNormalizationMode == LOG_LENGTH_NORMALIZATION) {
									tf = WordFrequencies.lookup(sentence[i].toLowerCase());
									if (tf > 10) tf = Math.log10(tf);
									else tf = 1;
								} else tf = 1;
								importance += (count.getValue() / tf);
							}
						}
					}
					
					//	don't throw out 0-scored results for combining approaches
					if (this.isCombined || (importance > 0)) {
						if (this.normalizationMode == NO_NORMALIZATION)
							r.setScore(importance);
						else if (this.normalizationMode == LINEAR_LENGTH_NORMALIZATION)
							r.setScore(importance / sentence.length); // try normalized score
						else if (this.normalizationMode == SQUARE_ROOT_LENGTH_NORMALIZATION)
							r.setScore(importance / ((float) Math.sqrt(sentence.length))); // try normalized score
						else if (this.normalizationMode == LOG_LENGTH_NORMALIZATION)
							r.setScore(importance / (1 + ((float) Math.log(sentence.length)))); // try normalized score
						else if (this.normalizationMode == LOG_10_LENGTH_NORMALIZATION)
							r.setScore(importance / (1 + ((float) Math.log10(sentence.length)))); // try normalized score
						
						rawResults.add(r);
					}
				}
			}
			
			if (rawResults.size() != 0) {
				//	find top result
				Collections.sort(rawResults);
				Collections.reverse(rawResults);
				Result top = rawResults.remove(0);
				resultList.add(top);
				
				//	decrement scores of top result terms
				String[] sentence = NETagger.tokenize(top.getAnswer());
				for (int i = 0; i < sentence.length; i++) {
					String term = SnowballStemmer.stem(sentence[i].toLowerCase());
					TermCounter count = termCounters.get(term);
					
					if (count != null) {
//						if (targetTerms.contains(term)) count.divideValue(2);
//						else count.divideValue(5);
//						if (targetTerms.contains(term)) count.divideValue(2);
//						else count.divideValue(3);
//						if (targetTerms.contains(term)) count.divideValue(2);
//						else count.divideValue(2);
						
						//	20070718
						if (targetTerms.contains(term)) count.divideValue(2);
						else count.divideValue(termCountLog);
						
						if (count.getValue() == 0) termCounters.remove(term);
					}
				}
				
				//	prepare remaining results for next round
				results = rawResults.toArray(new Result[rawResults.size()]);
				goOn = true;
			}
			
		} while (goOn);
		
		Collections.sort(resultList);
		Collections.reverse(resultList);
		
		//	set position-dependent extra score for combining approaches
		if (this.isCombined) {
			float eScore = 100;
			for (Result r : resultList) {
				r.addExtraScore((this.getClass().getName() + this.normalizationMode), eScore);
				eScore *= 0.9f;
			}
		}
		
		return resultList.toArray(new Result[resultList.size()]);
	}
	
//	private static String lastTarget = null;
//	private static String lastCacherClassName = null;
//	private static HashMap<String, TermCounter> lastTargetTermCounters = null;
	
	private static class CacheEntry {
		String target;
		HashMap<String, TermCounter> termCounters;
		public CacheEntry(String target, HashMap<String, TermCounter> termCounters) {
			this.target = target;
			this.termCounters = termCounters;
		}
	}
	
	private static HashMap<String, CacheEntry> cache = new HashMap<String, CacheEntry>();
	
	private void cache(String target, HashMap<String, TermCounter> termCounters) {
		String className = this.getClass().getName();
		System.out.println("WebTermImportanceFilter: caching web lookup result for target '" + target + "' from class '" + className + "'");
		CacheEntry ce = new CacheEntry(target, termCounters);
		cache.put(className, ce);
//		lastTarget = target;
//		lastCacherClassName = className;
//		lastTargetTermCounters = termCounters;
	}
	
	private HashMap<String, TermCounter> cacheLookup(String target) {
		String className = this.getClass().getName();
		System.out.println("WebTermImportanceFilter: doing cache lookup result for target '" + target + "', class '" + className + "'");
		CacheEntry ce = cache.get(className);
		if (ce == null) {
			System.out.println("  --> cache miss, no entry for '" + className + "' so far");
			return null;
		} else if (target.equals(ce.target)) {
			System.out.println("  --> cache hit");
			return ce.termCounters;
		} else {
			System.out.println("  --> cache miss, last target for '" + className + "' is '" + ce.target + "'");
			return null;
		}
	}
	
	/**	add all the term counters in source to target (perform a union of the key sets, summing up the counters)
	 * @param source
	 * @param target
	 */
	protected void addTermCounters(HashMap<String, TermCounter> source, HashMap<String, TermCounter> target) {
		for (Iterator<String> keys = source.keySet().iterator(); keys.hasNext();) {
			String key = keys.next();
			int count = source.get(key).getValue();
			if (!target.containsKey(key))
				target.put(key, new TermCounter());
			target.get(key).increment(count);
		}
	}
	
	/**	get the maximum count out of a set of counters 
	 * @param counters
	 */
	protected int getMaxCount(HashMap<String, TermCounter> counters) {
		int max = 0;
		for (Iterator<String> keys = counters.keySet().iterator(); keys.hasNext();)
			max = Math.max(max, counters.get(keys.next()).getValue());
		return max;
	}
	
	/**	get the sum of a set of counters 
	 * @param counters
	 */
	protected int getCountSum(HashMap<String, TermCounter> counters) {
		int sum = 0;
		for (Iterator<String> keys = counters.keySet().iterator(); keys.hasNext();)
			sum += counters.get(keys.next()).getValue();
		return sum;
	}
	
	/**	get the sum of a set of counters, each one minus the count in another set of counters 
	 * @param counters
	 * @param compare
	 */
	protected int sumDiff(HashMap<String, TermCounter> counters, HashMap<String, TermCounter> compare) {
		int diffSum = 0;
		for (Iterator<String> keys = counters.keySet().iterator(); keys.hasNext();) {
			String key = keys.next();
			int count = counters.get(key).getValue();
			int comp = (compare.containsKey(key) ? compare.get(key).getValue() : 0);
			diffSum += Math.max((count - comp), 0);
		}
		return diffSum;
	}
	
	protected static boolean TEST_TARGET_GENERATION = false;
	
	public static void main(String[] args) {
		TEST_TARGET_GENERATION = true;
		
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		// create tokenizer
		MsgPrinter.printStatusMsg("Creating tokenizer...");
		if (!OpenNLP.createTokenizer("res/nlp/tokenizer/opennlp/EnglishTok.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create tokenizer.");
//		LingPipe.createTokenizer();
		
		// create sentence detector
//		MsgPrinter.printStatusMsg("Creating sentence detector...");
//		if (!OpenNLP.createSentenceDetector("res/nlp/sentencedetector/opennlp/EnglishSD.bin.gz"))
//			MsgPrinter.printErrorMsg("Could not create sentence detector.");
//		LingPipe.createSentenceDetector();
		
		// create stemmer
		MsgPrinter.printStatusMsg("Creating stemmer...");
		SnowballStemmer.create();
		
		// create part of speech tagger
		MsgPrinter.printStatusMsg("Creating POS tagger...");
		if (!OpenNLP.createPosTagger("res/nlp/postagger/opennlp/tag.bin.gz",
									 "res/nlp/postagger/opennlp/tagdict"))
			MsgPrinter.printErrorMsg("Could not create OpenNLP POS tagger.");
//		if (!StanfordPosTagger.init("res/nlp/postagger/stanford/" +
//				"train-wsj-0-18.holder"))
//			MsgPrinter.printErrorMsg("Could not create Stanford POS tagger.");
		
		// create chunker
		MsgPrinter.printStatusMsg("Creating chunker...");
		if (!OpenNLP.createChunker("res/nlp/phrasechunker/opennlp/" +
								   "EnglishChunk.bin.gz"))
			MsgPrinter.printErrorMsg("Could not create chunker.");
		
		// create named entity taggers
		MsgPrinter.printStatusMsg("Creating NE taggers...");
		NETagger.loadListTaggers("res/nlp/netagger/lists/");
		NETagger.loadRegExTaggers("res/nlp/netagger/patterns.lst");
		MsgPrinter.printStatusMsg("  ...loading models");
//		if (!NETagger.loadNameFinders("res/nlp/netagger/opennlp/"))
//			MsgPrinter.printErrorMsg("Could not create OpenNLP NE tagger.");
		if (!StanfordNeTagger.isInitialized() && !StanfordNeTagger.init())
			MsgPrinter.printErrorMsg("Could not create Stanford NE tagger.");
		MsgPrinter.printStatusMsg("  ...done");
		
		WebTermImportanceFilter wtif = new TargetGeneratorTest(NO_NORMALIZATION);
		TRECTarget[] targets = TREC13To16Parser.loadTargets(args[0]);
		for (TRECTarget target : targets) {
			String question = target.getTargetDesc();
			
			// query generation
			MsgPrinter.printGeneratingQueries();
			String qn = QuestionNormalizer.normalize(question);
			MsgPrinter.printNormalization(qn);  // print normalized question string
			Logger.logNormalization(qn);  // log normalized question string
			String[] kws = KeywordExtractor.getKeywords(qn);
			AnalyzedQuestion aq = new AnalyzedQuestion(question);
			aq.setKeywords(kws);
			aq.setFactoid(false);
			
			Query[] queries = new BagOfWordsG().generateQueries(aq);
			for (int q = 0; q < queries.length; q++)
				queries[q].setOriginalQueryString(question);
			
			Result[] results = new Result[1];
			results[0] = new Result("This would be the answer", queries[0]);
			wtif.apply(results);
		}
	}
	
	private static class TargetGeneratorTest extends WebTermImportanceFilter {
		
		TargetGeneratorTest(int normalizationMode) {
			super(normalizationMode, normalizationMode, false);
		}

		public HashMap<String, TermCounter> getTermCounters(String[] targets) {
			return new HashMap<String, TermCounter>();
		}
	}
}

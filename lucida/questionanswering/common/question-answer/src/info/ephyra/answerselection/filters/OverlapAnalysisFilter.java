package info.ephyra.answerselection.filters;

import info.ephyra.io.MsgPrinter;
import info.ephyra.search.Result;
import info.ephyra.trec.TRECPattern;
import info.ephyra.util.StringUtils;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Hashtable;

/**
 * <p>Generates overlap analyses for different answer extraction strategies. The
 * filter can be added at more than one point in the pipeline, using different
 * IDs to generate independent analyses.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-02-10
 */
public class OverlapAnalysisFilter extends Filter {
	/** Pattern used to evaluate answer candidates. */
	private static TRECPattern pattern;
	/** Overlap analyses, usually at different points in the pipeline. */
	private static Hashtable<String, Hashtable<String, Integer>> analyses =
		new Hashtable<String, Hashtable<String, Integer>>();
	/** Current overlap analysis. */
	private Hashtable<String, Integer> overlapAnalysis =
		new Hashtable<String, Integer>();
	/**
	 * If this flag is set, an overlap analysis is printed every time the filter
	 * is applied.
	 */
	private boolean printing;
	
	/**
	 * Sets the current pattern used to evaluate answer candidates.
	 * 
	 * @param pattern evaluation pattern
	 */
	public static void setPattern(TRECPattern pattern) {
		OverlapAnalysisFilter.pattern = pattern;
	}
	
	/**
	 * Creates the filter and adds an overlap analysis for the given ID if it
	 * did not occur before.
	 * 
	 * @param id overlap analysis ID
	 * @param printing enable printing flag
	 */
	public OverlapAnalysisFilter(String id, boolean printing) {
		overlapAnalysis = analyses.get(id);
		if (overlapAnalysis == null) {
			overlapAnalysis = new Hashtable<String, Integer>();
			analyses.put(id, overlapAnalysis);
		}
		this.printing = printing;
	}
	
	/**
	 * Returns the current overlap analysis.
	 * 
	 * @return overlap analysis
	 */
	public Hashtable<String, Integer> getOverlapAnalysis() {
		return overlapAnalysis;
	}
	
	/**
	 * Prints the current overlap analysis.
	 */
	public void printOverlapAnalysis() {
		MsgPrinter.printStatusMsg("Overlap analysis:");
		for (String key : overlapAnalysis.keySet())
			MsgPrinter.printStatusMsg(key + ": " + overlapAnalysis.get(key));
	}
	
	/**
	 * Evaluates the answer candidates and updates the current overlap analysis.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return identical array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		if (pattern == null) return results;
		
		HashSet<String> techsSet = new HashSet<String>();
		for (Result result : results) {
			// check if part of answer string matches the pattern
			String answer = result.getAnswer();
			for (String regex : pattern.getRegexs())
				if (answer.matches(".*?" + regex + ".*+")) {
					String[] techniques = result.getExtractionTechniques();
					if (techniques == null || techniques.length == 0) {
						techsSet.add("Passage");
					} else {
						for (String technique : techniques)
							techsSet.add(technique);
					}
				}
		}
		if (techsSet.size() == 0) techsSet.add("None");
		
		String[] techs = techsSet.toArray(new String[techsSet.size()]);
		Arrays.sort(techs);
		String key = StringUtils.concat(techs, ", ");
		Integer count = overlapAnalysis.get(key);
		if (count != null) overlapAnalysis.put(key, count + 1);
		else overlapAnalysis.put(key, 1);
		
		if (printing) printOverlapAnalysis();
		
		// don't do anything to the results
		return results;
	}
}

package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>Trims answer candidates for definitional questions to improve the
 * precision by chopping of leading introductions of indirect speech.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-10
 */
public class CutStatementProviderFilter extends Filter {
	private static final Pattern PROVIDER_AT_START = Pattern.compile("(([A-Z][a-z]++\\s*+){2,}+said\\s++([A-Za-z]+\\s+)?that\\s++){1}+.++");
//	private static final Pattern PROVIDER_AT_END = Pattern.compile(".+\\,\\s+said(\\s+[A-Z][a-z]+)+");
	
	/**
	 * Cut leading or tailing introductory parts of information given in
	 * indirect speech. This is in order to not waste result length with these
	 * parts. It would cut, for instance, 'XYZ said that' from 'XYZ said that
	 * ... some useful information ...'
	 * 
	 * @param result a <code>Result</code> object
	 * @return the same <code>Result</code> object
	 */
	public Result apply(Result result) {
		String text = result.getAnswer();
		Matcher matcher = PROVIDER_AT_START.matcher(text);
		if (matcher.matches()) result.setAnswer(text.substring(matcher.group(1).length()).trim());
//		else {
//			matcher = PROVIDER_AT_END.matcher(text);
//			if (matcher.matches()) result.setAnswer(text.substring(0, (text.length() - matcher.group(1).length())).trim());
//		}
		return result;
	}
	
	/**
	 * Cut leading or tailing introductory parts of information given in
	 * indirect speech. This is in order to not waste result length with these
	 * parts. It would cut, for instance, 'XYZ said that' from 'XYZ said that
	 * ... some useful information ...'
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		for (int r = 0; r < results.length; r++) results[r] = this.apply(results[r]);
		return results;
	}
}

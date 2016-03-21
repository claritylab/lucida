package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>Trims answer candidates for definitional questions to improve the
 * precision by chopping of leading news agency acronyms and cities.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-10
 */
public class CutKeywordsFilter extends Filter {
	//private static final Pattern KEYWORDS_AND_LOCATION = Pattern.compile("(([A-Z\\-\\,\\_]++\\s++)++\\([A-Za-z]++\\)\\s++(\\_|(\\-\\-))\\s++){1}+.++");
	private static final Pattern KEYWORDS = Pattern.compile("(([A-Z\\-\\_\\.\\:\\,\\;]++\\s++)++){1}+.++");
	private static final Pattern BRACKETS = Pattern.compile("(\\([A-Za-z\\-\\_\\.\\:\\/\\,\\;]++(\\s++[A-Za-z\\-\\_\\.\\:\\/\\,\\;]++)*+\\)){1}+.++");
	
	
	/**
	 * Cut the leading news agency acronym and city from a snippet, for instance
	 * from 'MOSCOW (ITAR-TAS) ... some useful information ...', in order to not
	 * waste result length
	 * 
	 * @param result a <code>Result</code> object
	 * @return the same <code>Result</code> object
	 */
	public Result apply(Result result) {
		String text = result.getAnswer().trim();
		boolean changed = true;
		Matcher matcher;
		while (changed) {
			changed = false;
			matcher = KEYWORDS.matcher(text);
			if (matcher.matches()) {
				text = text.substring(matcher.group(1).length()).trim();
				changed = true;
			}
			matcher = BRACKETS.matcher(text);
			if (matcher.matches()) {
				text = text.substring(matcher.group(1).length()).trim();
				changed = true;
			}
		}
		result.setAnswer(text);
		return result;
	}
	
	/**
	 * Cut the leading news agency acronym and city from a snippet, for instance
	 * from 'MOSCOW (ITAR-TAS) ... some useful information ...', in order to not
	 * waste result length
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		for (int r = 0; r < results.length; r++) results[r] = this.apply(results[r]);
		return results;
	}
}

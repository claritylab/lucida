package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.ArrayList;

/**
 * <p>A filter that cuts off results if the total length of non-whitespace
 * characters exceeds a threshold.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter, Nico Schlaefer
 * @version 2006-07-15
 */
public class ResultLengthFilter extends Filter {
	/** Maximum number of non-whitespace characters per question. */
	private static final int MAX_NON_WHITESPACE = 7000;
	
	private int cutoffLength = MAX_NON_WHITESPACE;
	
	/**
	 */
	public ResultLengthFilter() {}

	/**
	 * @param cutoffLength
	 */
	public ResultLengthFilter(int cutoffLength) {
		this.cutoffLength = ((cutoffLength < MAX_NON_WHITESPACE) ? cutoffLength : MAX_NON_WHITESPACE);
	}

	/**
	 * Ensures that the total number of non-whitespace characters in all answer
	 * strings does not exceed <code>cutoffLength</code>.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return filtered array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		ArrayList<Result> filtered = new ArrayList<Result>();
		
		int length = 0;
		for (Result r : results) {
			length += r.getAnswer().replaceAll("\\s", "").length();
			if (length <= this.cutoffLength)
				filtered.add(r);
			else
				break;
		}
		
		return filtered.toArray(new Result[filtered.size()]);
	}
}

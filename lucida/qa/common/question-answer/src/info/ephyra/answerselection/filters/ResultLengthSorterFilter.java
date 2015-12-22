package info.ephyra.answerselection.filters;

import info.ephyra.search.Result;

import java.util.Arrays;
import java.util.Comparator;

/**
 * <p>The <code>ResultLengthSorterFilter</code> sorts the results by the lengths
 * of the answer strings in ascending order.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-02-15
 */
public class ResultLengthSorterFilter extends Filter {
	/**
	 * Sorts the results by the lengths of the answer strings in ascending
	 * order.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return sorted array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		Arrays.sort(results, new ResultLengthComparator<Result>());
		
		return results;
	}
}

/**
 * The <code>ResultLengthComparator</code> compares two <code>Result</code>
 * objects by comparing the lengths of the answer strings.
 * 
 * @author Nico Schlaefer
 * @version 2008-02-15
 */
class ResultLengthComparator<T> implements Comparator<T> {
	/**
	 * Compares its two arguments for order. Returns a negative integer, zero,
	 * or a positive integer as the first argument is less than, equal to, or
	 * greater than the second.
	 * 
	 * @param o1 the first object to be compared
	 * @param o2 the second object to be compared
	 * @return a negative integer, zero, or a positive integer as the first
	 *         argument is less than, equal to, or greater than the second
	 */
	public int compare(Object o1, Object o2) {
		if (!(o1 instanceof Result) || !(o2 instanceof Result))
			throw new ClassCastException();
		Result r1 = (Result) o1;
		Result r2 = (Result) o2;
		
		return r1.getAnswer().length() - r2.getAnswer().length();
	}
}

package info.ephyra.util;

import java.util.ArrayList;

/**
 * A collection of array transformation utilities.
 * 
 * @author Nico Schlaefer
 * @version 2007-05-03
 */
public class ArrayUtils {
	/**
	 * Gets all subsets of the given array of objects.
	 * 
	 * @param objects array of objects
	 * @return all subsets
	 */
	public static Object[][] getAllSubsets(Object[] objects) {
		ArrayList<Object[]> subsets = new ArrayList<Object[]>();
		
		getAllSubsetsRec(subsets, new ArrayList<Object>(), objects, 0);
		
		return subsets.toArray(new Object[subsets.size()][]);
	}
	
	// recursive implementation of getAllSubsets(Object)
	private static void getAllSubsetsRec(ArrayList<Object[]> subsets,
			ArrayList<Object> subset, Object[] objects, int i) {
		if (i == objects.length) {
			subsets.add(subset.toArray(new Object[subset.size()]));
		} else {
			ArrayList<Object> subset1 = new ArrayList<Object>();
			for (Object o : subset) subset1.add(o);
			
			ArrayList<Object> subset2 = new ArrayList<Object>();
			for (Object o : subset) subset2.add(o);
			subset2.add(objects[i]);
			
			i++;
			getAllSubsetsRec(subsets, subset1, objects, i);
			getAllSubsetsRec(subsets, subset2, objects, i);
		}
	}
	
	/**
	 * Gets all non-empty subsets of the given array of objects.
	 * 
	 * @param objects array of objects
	 * @return all non-empty subsets
	 */
	public static Object[][] getNonemptySubsets(Object[] objects) {
		Object[][] subsets = getAllSubsets(objects);
		
		Object[][] nonempty = new Object[subsets.length - 1][];
		for (int i = 0; i < nonempty.length; i++)
			nonempty[i] = subsets[i + 1];
		
		return nonempty;
	}
}

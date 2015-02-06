package info.ephyra.util;

/**
 * A <code>Dictionary</code> data structure is used to store and look up words.
 * 
 * @author Nico Schlaefer
 * @version 2007-02-11
 */
public interface Dictionary {
	/**
	 * Looks up a word.
	 * 
	 * @param word the word to look up
	 * @return <code>true</code> iff the word was found
	 */
	public abstract boolean contains(String word);
}

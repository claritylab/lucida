package info.ephyra.trec;

/**
 * A <code>TRECPattern</code> is a simple data structure consisting of the
 * ID of the pattern and regular expressions that describe correct answers.
 * 
 * @author Nico Schlaefer
 * @version 2006-07-07
 */
public class TRECPattern {
	/** The ID of the pattern. */
	private String id;
	/** The regular expressions that describe correct answers. */
	private String[] regexs;
	
	/**
	 * Creates a <code>TRECPattern</code> data structure and sets the ID and the
	 * regular expressions.
	 * 
	 * @param id the ID of the pattern
	 * @param regexs the regular expressions
	 */
	public TRECPattern(String id, String[] regexs) {
		this.id = id;
		this.regexs = regexs;
	}
	
	/**
	 * Returns the ID of the pattern.
	 * 
	 * @return ID
	 */
	public String getId() {
		return id;
	}
	
	/**
	 * Returns the regular expressions.
	 * 
	 * @return regular expression
	 */
	public String[] getRegexs() {
		return regexs;
	}
}

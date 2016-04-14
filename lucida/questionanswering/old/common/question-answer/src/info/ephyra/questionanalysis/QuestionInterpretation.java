package info.ephyra.questionanalysis;

import java.io.Serializable;

/**
 * <p>A <code>QuestionInterpretation</code> is a data structure comprising
 * the TARGET of a question, the CONTEXT and the PROPERTY the question asks for.
 * </p>
 * 
 * <p>This class implements the interfaces <code>Comparable</code> and
 * <code>Serializable</code>. Note: it has a natural ordering that is
 * inconsistent with <code>equals()</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-01-29
 */
public class QuestionInterpretation implements
		Comparable<QuestionInterpretation>, Serializable {
	/** Version number used during deserialization. */
	private static final long serialVersionUID = 20070501;
	
	/** The TARGET object of the question. */
	private String target;
	/** The CONTEXT objects of the question. */
	private String[] context;
	/** The PROPERTY the question asks for. */
	private String property;
	/** The LENGTH of this interpretation. */
	private int length;
	
	/**
	 * Creates a new <code>QuestionInterpretation</code> object and sets
	 * the TARGET, CONTEXT, PROPERTY and LENGTH fields.
	 * 
	 * @param target the TARGET of the question
	 * @param context the CONTEXT of the question
	 * @param property the PROPERTY that the question asks for
	 */
	public QuestionInterpretation(String target, String[] context,
								  String property) {
		this.target = target;
		this.context = context;
		this.property = property;
		
		calculateLength();
	}
	
	/**
	 * <p>Calculates the LENGTH of the QuestionInterpretation, which is defined
	 * as the sum of the length of the TARGET object and all CONTEXT
	 * objects.</p>
	 * 
	 * <p>The smaller the value, the more likely it is that the question was
	 * interpreted correctly. A smaller value also means a greater abstraction
	 * from the original formulation of the question and therefore the search
	 * will return more results.</p>
	 * 
	 * <p>On the other hand, a large value means - if the search does not fail -
	 * a high probability of a correct answer.</p>
	 */
	private void calculateLength() {
		length = target.length();
		
		for (String c : context) length += c.length();
	}
	
	/**
	 * Returns the TARGET of the question.
	 * 
	 * @return TARGET
	 */
	public String getTarget() {
		return target;
	}
	
	/**
	 * Returns the CONTEXT of the question.
	 * 
	 * @return CONTEXT
	 */
	public String[] getContext() {
		return context;
	}
	
	/**
	 * Returns the PROPERTY that the question asks for.
	 * 
	 * @return PROPERTY
	 */
	public String getProperty() {
		return property;
	}
	
	/**
	 * Returns the LENGTH of the interpretation.
	 * 
	 * @return LENGTH
	 */
	public int getLength() {
		return length;
	}
	
	/**
	 * Sets the TARGET of the question.
	 * 
	 * @param target the TARGET object
	 */
	public void setTarget(String target) {
		this.target = target;
		
		calculateLength();
	}
	
	/**
	 * Sets the CONTEXT of the question.
	 * 
	 * @param context the CONTEXT objects
	 */
	public void setContext(String[] context) {
		this.context = context;
		
		calculateLength();
	}
	
	/**
	 * Drops the CONTEXT without recalculating the length.
	 */
	public void dropContext() {
		this.context = new String[0];
	}
	
	/**
	 * <p>Compares this <code>QuestionInterpretation</code> to another
	 * <code>QuestionInterpretation</code> object.</p>
	 * 
	 * @param qi the <code>QuestionInterpretation</code> to be compared
	 * @return a negative integer, zero, or a positive integer as this
	 * 		   <code>QuestionInterpretation</code> is less than, equal to, or
	 * 		   greater than the specified <code>QuestionInterpretation</code>
	 */
	public int compareTo(QuestionInterpretation qi) {
		return length - qi.getLength();
	}
	
	/**
	 * Returns a string representation of the interpretation.
	 * 
	 * @return readable string
	 */
	public String toString() {
		String s = "Property: " + getProperty();
		s += "\nTarget: " + getTarget();
		for (String context : getContext())
			s += "\nContext: " + context;
		
		return s;
	}
}

package info.ephyra.trec;

/**
 * A <code>TRECTarget</code> is a simple data structure consisting of the ID of
 * the target, the target description, possible target types and a list of
 * questions.
 * 
 * @author Nico Schlaefer
 * @version 2006-07-09
 */
public class TRECTarget {
	/** All types of targets. */
	public static final String[] TARGET_TYPES =
		{"PERSON", "ORGANISATION", "EVENT", "THING"};
	
	/** The ID of the target. */
	private String id;
	/** The target description. */
	private String targetDesc;
	/** Condensed target appended to queries for factoid and list questions. */
	private String condensedTarget;
	/** Possible target types. */
	private String[] targetTypes = TARGET_TYPES.clone();
	/** Indicates if the target is a noun phrase. */
	private boolean nounPhrase = true;
	/** The questions on the target. */
	private TRECQuestion[] questions;
	
	/**
	 * Creates a <code>TRECTarget</code> data structure and sets the ID, the
	 * target description and the questions.
	 * 
	 * @param id the ID of the target
	 * @param targetDesc the target description
	 * @param questions the questions on the target
	 */
	public TRECTarget(String id, String targetDesc, TRECQuestion[] questions) {
		this.id = id;
		this.targetDesc = targetDesc;
		this.questions = questions;
	}
	
	/**
	 * Returns the ID of the target.
	 * 
	 * @return ID
	 */
	public String getId() {
		return id;
	}
	
	/**
	 * Returns the target description.
	 * 
	 * @return target description
	 */
	public String getTargetDesc() {
		return targetDesc;
	}
	
	/**
	 * Returns the condensed target.
	 * 
	 * @return condensed target
	 */
	public String getCondensedTarget() {
		return condensedTarget;
	}
	
	/**
	 * Returns the possible target types.
	 * 
	 * @return array of target types
	 */
	public String[] getTargetTypes() {
		return targetTypes;
	}
	
	/**
	 * Checks if the target is a noun phrase.
	 * 
	 * @return <code>true</code> iff the target is a noun phrase.
	 */
	public boolean isNounPhrase() {
		return nounPhrase;
	}
	
	/**
	 * Returns the questions on the target.
	 * 
	 * @return questions
	 */
	public TRECQuestion[] getQuestions() {
		return questions;
	}
	
	/**
	 * Sets the target description.
	 * 
	 * @param targetDesc target description
	 */
	public void setTargetDesc(String targetDesc) {
		this.targetDesc = targetDesc;
	}
	
	/**
	 * Sets the condensed target.
	 * 
	 * @param condensedTarget condensed target
	 */
	public void setCondensedTarget(String condensedTarget) {
		this.condensedTarget = condensedTarget;
	}
	
	/**
	 * Sets the possible target types.
	 * 
	 * @param targetTypes possible target types
	 */
	public void setTargetTypes(String[] targetTypes) {
		this.targetTypes = targetTypes;
	}
	
	/**
	 * Marks the target as a noun phrase.
	 * 
	 * @param nounPhrase <code>true</code> or <code>false</code>
	 */
	public void setNounPhrase(boolean nounPhrase) {
		this.nounPhrase = nounPhrase;
	}
}

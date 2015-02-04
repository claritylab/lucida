package info.ephyra.trec;

import info.ephyra.questionanalysis.QuestionInterpretation;

/**
 * A <code>TRECQuestion</code> is a simple data structure consisting of a
 * question ID, a question type, a question string and the answers returned by
 * the QA engine.
 * 
 * @author Nico Schlaefer
 * @version 2006-06-11
 */
public class TRECQuestion {
	/** The ID of the question. */
	private String id;
	/** The type of the question. */
	private String type;
	/** The question string. */
	private String questionString;
	/** The answers from the QA engine. */
	private TRECAnswer[] answers;
	/** Question interpretation that led to the top answer if available. */
	private QuestionInterpretation interpretation;
	
	/**
	 * Creates a <code>TRECQuestion</code> data structure and sets the fields
	 * for ID, type and question string.
	 * 
	 * @param id the ID of the question
	 * @param type the type of the question
	 * @param questionString the question string
	 */
	public TRECQuestion(String id, String type, String questionString) {
		this.id = id;
		this.type = type;
		this.questionString = questionString;
	}
	
	/**
	 * Sets a modified question string, e.g. after coreference resolution.
	 * 
	 * @param questionString modified question string
	 */
	public void setQuestionString(String questionString) {
		this.questionString = questionString;
	}
	
	/**
	 * Sets the answers determined by the QA engine.
	 * 
	 * @param answers the answers 
	 */
	public void setAnswers(TRECAnswer[] answers) {
		this.answers = answers;
	}
	
	/**
	 * Sets the question interpretation that led to the top answer.
	 * 
	 * @param interpretation top interpretation
	 */
	public void setInterpretation(QuestionInterpretation interpretation) {
		this.interpretation = interpretation;
	}
	
	/**
	 * Returns the ID of the question.
	 * 
	 * @return ID
	 */
	public String getId() {
		return id;
	}
	
	/**
	 * Returns the type of the question.
	 * 
	 * @return type
	 */
	public String getType() {
		return type;
	}
	
	/**
	 * Returns the question string.
	 * 
	 * @return question string
	 */
	public String getQuestionString() {
		return questionString;
	}
	
	/**
	 * Returns the answers determined by the QA engine.
	 * 
	 * @return answers
	 */
	public TRECAnswer[] getAnswers() {
		return answers;
	}
	
	/**
	 * Returns the question interpretation that led to the top answer if
	 * available.
	 * 
	 * @return top interpretation
	 */
	public QuestionInterpretation getInterpretation() {
		return interpretation;
	}
}

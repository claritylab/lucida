package info.ephyra.trec;

/**
 * <p>A <code>TRECAnswer</code> is a simple data structure consisting of the ID
 * of the answer, an answer string and optionally a supporting document.</p>
 * 
 * <p>This data structure is used both for answers returned by the QA engine and
 * answers provided by TREC.</p>
 * 
 * @author Nico Schlaefer
 * @version 2006-06-11
 */
public class TRECAnswer {
	/** The ID of the answer. */
	private String id;
	/** The answer string. */
	private String answerString;
	/** The supporting document. */
	private String supportDoc;
	
	/**
	 * Creates a <code>TRECAnswer</code> data structure and sets the ID and the
	 * answer string.
	 * 
	 * @param id the ID of the answer
	 * @param answerString the answer string
	 */
	public TRECAnswer(String id, String answerString) {
		this.id = id;
		this.answerString = answerString;
	}
	
	/**
	 * Creates a <code>TRECAnswer</code> data structure and sets the ID, the
	 * answer string and the supporting document.
	 * 
	 * @param id the ID of the answer
	 * @param answerString the answer string
	 * @param supportDoc the supporting document
	 */
	public TRECAnswer(String id, String answerString, String supportDoc) {
		this.id = id;
		this.answerString = answerString;
		this.supportDoc = supportDoc;
	}
	
	/**
	 * Returns the ID of the answer.
	 * 
	 * @return ID
	 */
	public String getId() {
		return id;
	}
	
	/**
	 * Returns the answer string.
	 * 
	 * @return answer string
	 */
	public String getAnswerString() {
		return answerString;
	}
	
	/**
	 * Returns the supporting document.
	 * 
	 * @return supporting document
	 */
	public String getSupportDoc() {
		return supportDoc;
	}
}

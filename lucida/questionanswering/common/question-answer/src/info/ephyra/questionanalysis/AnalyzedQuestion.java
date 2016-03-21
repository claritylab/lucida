package info.ephyra.questionanalysis;

import info.ephyra.nlp.semantics.Predicate;

import java.io.Serializable;

/**
 * <p>An <code>AnalyzedQuestion</code> is a data structure representing a
 * syntactic and semantic analysis of a question.</p>
 * 
 * <p>This class implements the interface <code>Serializable</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-07-17
 */
public class AnalyzedQuestion implements Serializable {
	/** Version number used during deserialization. */
	private static final long serialVersionUID = 20070717;
	
	/** Question string. */
	private String question;
	/** Normalized question string. */
	private String qn;
	/** Question string with stemmed verbs and nouns. */
	private String stemmed;
	/** Question string with modified verbs. */
	private String verbMod;
	/** Keywords in the question and context. */
	private String[] kws;
	/** Named entities in the question and context. */
	private String[][] nes;
	/** Terms in the question and context. */
	private Term[] terms;
	/** Focus word. */
	private String focus;
	/** Expected answer types. */
	private String[] ats = new String[0];
	/** Interpretations of the question. */
	private QuestionInterpretation[] qis = new QuestionInterpretation[0];
	/** Predicates extracted from the question. */
	private Predicate[] ps = new Predicate[0];
	/** Indicates that this is a factoid question. */
	private boolean isFactoid = true;
	
	public AnalyzedQuestion(String question) {
		setQuestion(question);
	}
	
	public AnalyzedQuestion(String question, String qn, String stemmed,
			String verbMod, String[] kws, String[][] nes, Term[] terms) {
		this(question);
		
		setNormalized(qn);
		setStemmed(stemmed);
		setVerbMod(verbMod);
		setKeywords(kws);
		setNes(nes);
		setTerms(terms);
	}
	
	public AnalyzedQuestion(String question, String qn, String stemmed,
			String verbMod, String[] kws, String[][] nes, Term[] terms,
			String focus, String[] ats, QuestionInterpretation[] qis,
			Predicate[] ps) {
		this(question, qn, stemmed, verbMod, kws, nes, terms);
		
		setFocus(focus);
		setAnswerTypes(ats);
		setInterpretations(qis);
		setPredicates(ps);
	}
	
	public String getQuestion() {
		return question;
	}
	
	public String getNormalized() {
		return qn;
	}
	
	public String getStemmed() {
		return stemmed;
	}
	
	public String getVerbMod() {
		return verbMod;
	}
	
	public String[] getKeywords() {
		return kws;
	}
	
	public String[][] getNes() {
		return nes;
	}
	
	public Term[] getTerms() {
		return terms;
	}
	
	public String getFocus() {
		return focus;
	}
	
	public String[] getAnswerTypes() {
		return ats;
	}
	
	public QuestionInterpretation[] getInterpretations() {
		return qis;
	}
	
	public Predicate[] getPredicates() {
		return ps;
	}
	
	public boolean isFactoid() {
		return isFactoid;
	}
	
	public void setQuestion(String question) {
		this.question = question;
	}
	
	public void setNormalized(String qn) {
		this.qn = qn;
	}
	
	public void setStemmed(String stemmed) {
		this.stemmed = stemmed;
	}
	
	public void setVerbMod(String verbMod) {
		this.verbMod = verbMod;
	}
	
	public void setKeywords(String[] kws) {
		this.kws = kws;
	}
	
	public void setNes(String[][] nes) {
		this.nes = nes;
	}
	
	public void setTerms(Term[] terms) {
		this.terms = terms;
	}
	
	public void setFocus(String focus) {
		this.focus = focus;
	}
	
	public void setAnswerTypes(String[] ats) {
		this.ats = ats;
	}
	
	public void setInterpretations(QuestionInterpretation[] qis) {
		this.qis = qis;
	}
	
	public void setPredicates(Predicate[] ps) {
		this.ps = ps;
	}
	
	public void setFactoid(boolean isFactoid) {
		this.isFactoid = isFactoid;
	}
}

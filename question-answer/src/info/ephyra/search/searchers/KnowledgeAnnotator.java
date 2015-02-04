package info.ephyra.search.searchers;

import info.ephyra.querygeneration.Query;
import info.ephyra.search.Result;
import info.ephyra.search.Search;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>A <code>KnowledgeAnnotator</code> searches a (semi)structured knowledge
 * source. It provides a specialized solution to certain classes of questions,
 * described by a set of question patterns. Only questions that match at least
 * one of the patterns in the field <code>qPatterns</code> are supported by a
 * <code>KnowledgeAnnotator</code>.</p>
 * 
 * <p>It runs as a separate thread, so several queries can be performed in
 * parallel.</p>
 * 
 * <p>This class extends the class <code>Searcher</code> and is abstract.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-09-28
 */
public abstract class KnowledgeAnnotator extends Searcher {
	/** Name of the knowledge annotator. */
	protected String name;
	/**
	 * A question that matches at least one of these patterns can be handled by
	 * the <code>KnowledgeAnnotator</code>.
	 */
	protected ArrayList<Pattern> qPatterns = new ArrayList<Pattern>();
	/**
	 * Strings identifying the relevant content of a question by referring to
	 * the groups in the corresponding question patterns.
	 */
	protected ArrayList<String> qContents = new ArrayList<String>();
	/** Index of the matching pattern. */
	protected int index;
	/** The <code>Matcher</code> that matched the pattern with the question. */
	protected Matcher matcher;
	
	/**
	 * Protected constructor used by the <code>getCopy()</code> method.
	 * 
	 * @param name name of the <code>KnowledgeAnnotator</code>
	 * @param qPatterns question patterns
	 * @param qContents descriptors of the relevant content of a question
	 */
	protected KnowledgeAnnotator(String name, ArrayList<Pattern> qPatterns,
								 ArrayList<String> qContents) {
		this.name = name;
		this.qPatterns = qPatterns;
		this.qContents = qContents;
	}
	
	/**
	 * <p>Creates a <code>KnowledgeAnnotator</code> and reads the question
	 * patterns and descriptors of the relevant content of a question from a
	 * file.</p>
	 * 
	 * <p>The file must have the following format:</p>
	 * 
	 * <p><code>KnowledgeAnnotator::<br>
	 * [name of the knowledge annotator]<br>
	 * <br>
	 * QuestionPatterns:<br>
	 * [regular expression 1]<br>
	 * [relevant content 1]<br>
	 * ...<br>
	 * [regular expression n]<br>
	 * [relevant content n]</code></p>
	 * 
	 * <p>The relevant content of a question is described by a string that may
	 * contain group identifiers of the format <code>[group_no]</code> that are
	 * replaced by the capturing groups that occur in the corresponding question
	 * pattern.</p>
	 * 
	 * @param filename file containing the question patterns and descriptors of
	 * 				   the relevant content of a question
	 */
	public KnowledgeAnnotator(String filename) throws IOException {
		File file = new File(filename);
		BufferedReader in = new BufferedReader(new FileReader(file));
		
		// read name of the knowledge
		in.readLine();
		name = in.readLine();
		in.readLine();
		
		// read answer patterns
		in.readLine();
		while (in.ready()) {
			qPatterns.add(Pattern.compile(in.readLine()));
			qContents.add(in.readLine());
		}
		
		in.close();
	}
	
	/**
	 * Tests whether the knowledge annotator is appropriate for a question by
	 * applying the patterns in the field <code>qPatterns</code>.
	 * 
	 * @param query <code>Query</code> object
	 * @return true, iff the question matches at least one of the patterns in
	 * 		   <code>qPatterns</code>
	 */
	protected boolean matches(Query query) {
		String question = query.getAnalyzedQuestion().getQuestion();
		
		for (int i = 0; i < qPatterns.size(); i++) {
			Matcher m = qPatterns.get(i).matcher(question);
			
			if (m.matches()) {
				this.query = query;	// save the Query object
				index = i;			// save the index of the pattern
				matcher = m;		// save the matcher
				
				return true;
			}
		}
		
		return false;
	}
	
	/**
	 * Extracts the relevant content of a question by resolving the group
	 * identifiers of the format <code>[group_no]</code> in the content string
	 * that corresponds to the matching pattern.
	 * 
	 * @return relevant content of the question
	 */
	protected String getContent() {
		String content = qContents.get(index);
		
		Pattern p = Pattern.compile("\\[(\\d*)\\]");
		Matcher m = p.matcher(content);
		
		// replace all group IDs by the corresponding parts of the question
		while (m.find()) {
			int group = Integer.parseInt(m.group(1));
			
			content = content.replace(m.group(), matcher.group(group));
		}
		
		return content;
	}
	/**
	 * Creates an array of a single <code>Result</code> object form an answer
	 * string and a document ID.
	 * 
	 * @param answer answer string
	 * @param docID document ID
	 * @return array of a single <code>Result</code> object
	 */
	protected Result[] getResult(String answer, String docID) {
		Result[] results = new Result[1];
		
		results[0] = new Result(answer, query, docID);
		// result is always returned by the QA engine
		results[0].setScore(Float.POSITIVE_INFINITY);
		
		return results;
	}
	
	/**
	 * Returns the name of the knowledgeAnnotator.
	 * 
	 * @return name of the knowledge annotator
	 */
	public String getKAName() {
		return name;
	}
	
	/**
	 * <p>Returns a new instance of the <code>KnowledgeAnnotator</code>. A new
	 * instance is created for each query.</p>
	 * 
	 * <p>It does not necessarily return an exact copy of the current
	 * instance.</p>
	 * 
	 * @return new instance of the <code>KnowledgeAnnotator</code>
	 */
	public abstract KnowledgeAnnotator getCopy();
	
	/**
	 * <p>Sets the query and starts the thread if the knowledge annotator is
	 * appropriate for the user question.</p>
	 * 
	 * <p>This method should be used instead of the inherited
	 * <code>start()</code> method without arguments.</p>
	 * 
	 * @param query query object
	 * @return true, iff the knowledge annotator is appropriate and the thread
	 * 		   was started
	 */
	public boolean start(Query query) {
		KnowledgeAnnotator ka = getCopy();
		
		if (ka.matches(query)) {
			// wait until there are less than MAX_PENDING pending queries
			Search.waitForPending();
			
			ka.start();
			
			// one more pending query
			Search.incPending();
			
			return true;
		}
		
		return false;
	}
}

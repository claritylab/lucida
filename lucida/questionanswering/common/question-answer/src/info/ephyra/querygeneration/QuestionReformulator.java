package info.ephyra.querygeneration;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>A <code>QuestionReformulator</code> can be applied to a question to obtain
 * reformulations of the question that are likely to occur in text passages that
 * answer the question.</p>
 * 
 * <p>A question is expected to be of the format described by the
 * <code>pattern</code> field. If the question does not match this pattern,
 * no reformulations are created.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-11-09
 */
public class QuestionReformulator {
	/**
	 * The pattern that identifies questions that can be processed by this
	 * reformulator.
	 */
	private Pattern pattern;
	/**
	 * <code>QuestionReformulation</code> objects that are applied to questions
	 * that match the pattern.
	 */
	private ArrayList<QuestionReformulation> reforms =
		new ArrayList<QuestionReformulation>();
	
	/**
	 * <p>Creates a new <code>QuestionReformulator</code> from a file.</p>
	 * 
	 * <p>The file must have the following format:</p>
	 * 
	 * <p><code>QuestionPattern:<br>
	 * [regular expression]<br>
	 * <br>
	 * QuestionReformulations:<br>
	 * [expr 1]<br>
	 * [score 1]<br>
	 * ...<br>
	 * [expr n]<br>
	 * [score n]</code></p>
	 * 
	 * <p><code>expr</code> is an expression describing a reformulation of the
	 * question. See the documentation of the class
	 * <code>QuestionReformulation</code> for further details on the format of
	 * such an expression. <code>score</code> is the score assigned to the
	 * reformulation and should be the higher the more specialized a
	 * reformulation is.</p>
	 * 
	 * @param filename name of the file containing the reformulation rules
	 * @throws IOException if the reformulation rules could not be read
	 * 					   successfully
	 */
	public QuestionReformulator(String filename) throws IOException {
		File file = new File(filename);
		BufferedReader in = new BufferedReader(new FileReader(file));
		
		// read question pattern
		in.readLine();
		pattern = Pattern.compile(in.readLine(), Pattern.CASE_INSENSITIVE);
		in.readLine();
		
		// read question reformulations
		in.readLine();		
		String expr;
		float score;
		while (in.ready()) {
			expr = in.readLine();
			score = Float.parseFloat(in.readLine());
			reforms.add(new QuestionReformulation(expr, score));
		}
		
		in.close();
	}
	
	/**
	 * Creates reformulations of a question if it is of the format described by
	 * the <code>pattern</code> field and wraps them in <code>Query</code>
	 * objects.
	 * 
	 * @param verbMod question string with modified verbs
	 * @return <code>Query</code objects created from question reformulations or
	 * 		   <code>null</code>
	 */
	public Query[] apply(String verbMod) {
		Matcher matcher = pattern.matcher(verbMod);
		
		if (!matcher.matches()) return null;  // question does not match pattern
		
		ArrayList<Query> queries = new ArrayList<Query>();
		
		String[] queryStrings;
		Query query;
		for (QuestionReformulation reform : reforms) {  // apply reformulators
			queryStrings = reform.get(matcher);
			
			for (String queryString : queryStrings) {
				// create query object and set score
				query = new Query(queryString);
				query.setScore(reform.getScore());
				
				queries.add(query);
			}
		}
		
		return queries.toArray(new Query[queries.size()]);
	}
}

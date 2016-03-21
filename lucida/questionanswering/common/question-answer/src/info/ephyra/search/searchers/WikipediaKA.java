package info.ephyra.search.searchers;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.SentenceExtractor;
import info.ephyra.search.Result;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.regex.Pattern;

/**
 * <p>A <code>KnowledgeAnnotator</code> for the Wikipedia online encyclopedia.
 * It answers a question for a definition by returning a sentence from the
 * corresponding Wikipedia web page.</p>
 * 
 * <p>It runs as a separate thread, so several queries can be performed in
 * parallel.</p>
 * 
 * <p>This class extends the class <code>KnowledgeAnnotator</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-09-28
 */
public class WikipediaKA extends KnowledgeAnnotator {
	/** The URL of the Wikipedia search page. */
	private static final String URL =
		"http://en.wikipedia.org/wiki/Special:Search?search=";
	
	/**
	 * Protected constructor used by the <code>getCopy()</code> method.
	 * 
	 * @param name name of the <code>KnowledgeAnnotator</code>
	 * @param qPatterns question patterns
	 * @param qContents descriptors of the relevant content of a question
	 */
	protected WikipediaKA(String name, ArrayList<Pattern> qPatterns,
						  ArrayList<String> qContents) {
		super(name, qPatterns, qContents);
	}
	
	/**
	 * Creates a <code>WikipediaKA</code> and calls the constructor of the
	 * superclass that reads the question patterns from a file.
	 * 
	 * @param filename file containing the question patterns
	 */
	public WikipediaKA(String filename) throws IOException {
		super(filename);
	}
	
	/**
	 * Queries Wikipedia for a definition and returns an array containing a
	 * single <code>Result</code> object or an empty array, if the search
	 * failed.
	 * 
	 * @return array containing a single <code>Result</code> or an empty array
	 */
	protected Result[] doSearch() {
		try {
			// compose URL for the search
			
			String content = getContent();
			String param = content.replace(" ", "+");
			URL search = new URL(URL + param);
		
			// retrieve document and extract answer sentence
			
			BufferedReader in;
			String line, sentence;
			
			in = new BufferedReader(new InputStreamReader(search.openStream(),
									Charset.forName("utf-8")));
			
			while (in.ready()) {
				line = in.readLine();
				
				// line should contain the term
				if (line.matches("(?i).*" + content + ".*")) {
					// extract first sentence
					sentence = SentenceExtractor.getSentencesFromHtml(line)[0];
					
					// sentence is really a definition of the term
					if (sentence.matches("(?i)(an? |the )?" + content +
										 ".*\\."))
						// create result from sentence
						return getResult(sentence, search.toString());
				}
			}
			
			in.close();
		}
		catch (Exception e) {
			MsgPrinter.printSearchError(e);  // print search error message
		}
		
		return new Result[0];  // search failed
	}
	
	/**
	 * Returns a new instance of <code>WikipediaKA</code>. A new instance is
	 * created for each query.
	 * 
	 * @return new instance of <code>WikipediaKA</code>
	 */
	public KnowledgeAnnotator getCopy() {
		KnowledgeAnnotator ka = new WikipediaKA(name, qPatterns, qContents);
		
		return ka;
	}
}

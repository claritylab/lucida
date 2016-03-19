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
import java.util.Hashtable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * <p>A <code>KnowledgeAnnotator</code> for the CIA World Factbook. It answers a
 * question about a country by extracting the information from the web page for
 * that country.</p>
 * 
 * <p>It runs as a separate thread, so several queries can be performed in
 * parallel.</p>
 * 
 * <p>This class extends the class <code>KnowledgeAnnotator</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-09-29
 */
public class WorldFactbookKA extends KnowledgeAnnotator {
	/** The URL of the CIA World Factbook. */
	private static final String URL =
		"https://www.cia.gov/library/publications/the-world-factbook/";
	/** Country names and corresponding web pages. */
	private Hashtable<String, String> countries =
		new Hashtable<String, String>();
	
	/**
	 * Protected constructor used by the <code>getCopy()</code> method.
	 * 
	 * @param name name of the <code>KnowledgeAnnotator</code>
	 * @param qPatterns question patterns
	 * @param qContents descriptors of the relevant content of a question
	 */
	protected WorldFactbookKA(String name, ArrayList<Pattern> qPatterns,
							  ArrayList<String> qContents) {
		super(name, qPatterns, qContents);
	}
	
	/**
	 * <p>Creates a <code>WorldFactbookKA</code> and calls the constructor of
	 * the superclass that reads the question patterns from a file.</p>
	 * 
	 * <p>Furthermore, a list of the available countries and the URLs of the
	 * corresponding web pages are retrieved from the Factbook.</p>
	 * 
	 * @param filename file containing the question patterns
	 */
	public WorldFactbookKA(String filename) throws IOException {
		super(filename);
		
		try {
			URL factbook = new URL(URL);  // URL of the main page
			
			BufferedReader in;
			String line;
			
			in = new BufferedReader(new InputStreamReader(factbook.openStream(),
									Charset.forName("iso-8859-1")));

			Pattern p = Pattern.compile(".*<option\\s*value=\"(.*)\"\\s*>(.*)" +
										"</option>.*");
			Matcher m;
			while (in.ready()) {
				line = in.readLine();
				
				m = p.matcher(line);
				if (m.matches())  // (country, url) pair found
					countries.put(m.group(2).toLowerCase(), m.group(1));
			}
			
			in.close();
		}
		catch (Exception e) {
			MsgPrinter.printSearchError(e);  // print search error message
		}
	}
	
	/**
	 * Searches the World Factbook for country details and returns an array
	 * containing a single <code>Result</code> object or an empty array, if the
	 * search failed.
	 * 
	 * @return array containing a single <code>Result</code> or an empty array
	 */
	protected Result[] doSearch() {
		try {
			// get country name and demanded information
			
			String[] content = getContent().split("#");
			String info = content[0];
			String country = content[1];
			
			// get URL of country web page
			
			String countryPage = countries.get(country.toLowerCase());
			if (countryPage == null) return new Result[0];
			URL page = new URL(URL + countryPage);
		
			// retrieve document
			
			BufferedReader in;
			String html = "";
			
			in = new BufferedReader(new InputStreamReader(page.openStream(),
									Charset.forName("iso-8859-1")));
			
			while (in.ready()) {
				html += in.readLine() + " ";
			}
			
			in.close();
			
			// extract information
			Pattern p = Pattern.compile("(?i).*" + info + ":</div>\\s*</td>" +
										"\\s*<td .*?>(.*?)</td>.*");
			Matcher m = p.matcher(html);
			if (m.matches()) {
				// extract sentence
				String sentence = SentenceExtractor.getSentencesFromHtml(m.group(1))[0];
				
				// create result from that sentence
				return getResult(sentence, page.toString());
			}
		}
		catch (Exception e) {
			MsgPrinter.printSearchError(e);  // print search error message
		}
		
		return new Result[0];  // search failed
	}
	
	/**
	 * Returns a new instance of <code>WorldFactbookKA</code>. A new instance is
	 * created for each query.
	 * 
	 * @return new instance of <code>WorldFactbookKA</code>
	 */
	public KnowledgeAnnotator getCopy() {
		WorldFactbookKA ka = new WorldFactbookKA(name, qPatterns, qContents);
		ka.countries = countries;  // also copy list of countries
		
		return ka;
	}
}

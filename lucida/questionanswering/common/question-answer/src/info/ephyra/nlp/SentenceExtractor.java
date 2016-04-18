package info.ephyra.nlp;

import info.ephyra.util.HTMLConverter;

/**
 * Extracts sentences and text fragments from an HTML document.
 * 
 * @author Nico Schlaefer
 * @version 2005-09-12
 */
public class SentenceExtractor {
	/**
	 * Regular expression that describes non-structuring tags, i.e. tags that
	 * appear within a sentence and that are not sentence delimiters. All other
	 * tags are assumed to be sentence delimiters.
	 */
	private static final String NON_STRUC_TAGS = "(?i)" +
		"<b( .*?)?>|</b>|<i( .*?)?>|</i>|<u( .*?)?>|</u>|<sup( .*?)?>|</sup>" +
		"|<sub( .*?)?>|</sub>|<tt( .*?)?>|</tt>|<font( .*?)?>|</font>" +
		"|<small( .*?)?>|</small>|<big( .*?)?>|</big>|<a( .*?)?>|</a>" +
		"|<br>|<nobr>";
	
	/**
	 * Extracts sentences from an HTML document
	 * 
	 * @param html the HTML document
	 * @return sentences extracted from the document
	 */
	public static String[] getSentencesFromHtml(String html) {
		// handle special characters
		html = HTMLConverter.replaceSpecialCharacters(html);
		
		// drop non-structuring tags
		html = html.replaceAll(NON_STRUC_TAGS, "");
		// replace all structuring tags by the sentence delimiter tag <delim>
		html = html.replaceAll("<.*?>", "<delim>");
		
		// insert the <delim> tag between all sentences
		html = html.replaceAll("\\. ", "\\.<delim>");
		html = html.replaceAll("! ", "!<delim>");
		html = html.replaceAll("\\? ", "\\?<delim>");

		// replace all sequences of whitespace characters by single blanks
		html = html.replaceAll("\\s+", " ");
		// remove multiple <delim> tags
		html = html.replaceAll(" ?<delim>( |<delim>)*", "<delim>");
		// remove whitespaces and <delim> tags at beginning/end of the document
		html = html.replaceAll("\\A( |<delim>)|( |<delim>)\\z", "");
		
		// split the document into sentences
		String[] sentences = html.split("<delim>");
		
		return sentences;
	}
}

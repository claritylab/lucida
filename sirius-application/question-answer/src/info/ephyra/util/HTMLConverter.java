package info.ephyra.util;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.net.URLConnection;

import org.htmlparser.Parser;
import org.htmlparser.beans.StringBean;
import org.htmlparser.util.ParserException;

/**
 * The <code>HTMLConverter</code> can be used to convert an HTML document to
 * plain text.
 * 
 * @author Nico Schlaefer
 * @version 2007-06-19
 */
public class HTMLConverter {
	/** Timeout for HTTP connections in milliseconds. */
	private static final int TIMEOUT = 120000;  // 2 min
	
	/**
	 * Checks if the given string is a URL.
	 * 
	 * @param s a string
	 * @return <code>true</code> iff the string is a URL
	 */
	public static boolean isUrl(String s) {
		try {
			new URL(s);
		} catch (MalformedURLException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Handles special characters in HTML documents by replacing sequences of
	 * the form <code>&...;</code> by the corresponding characters.
	 * 
	 * @param html html document
	 * @return transformed html document
	 */
	public static synchronized String replaceSpecialCharacters(String html) {
		html = html.replaceAll("&#09;", " ");
		html = html.replaceAll("&#10;", " ");
		html = html.replaceAll("&#32;", " ");
		html = html.replaceAll("&#33;", "!");
		html = html.replaceAll("(?i)(&#34;|&quot;)", "\"");
		html = html.replaceAll("&#35;", "#");
		html = html.replaceAll("&#36;", "$");
		html = html.replaceAll("&#37;", "%");
		html = html.replaceAll("(?i)(&#38;|&amp;)", "&");
		html = html.replaceAll("&#39;", "'");
		html = html.replaceAll("&#40;", "(");
		html = html.replaceAll("&#41;", ")");
		html = html.replaceAll("&#42;", "*");
		html = html.replaceAll("&#43;", "+");
		html = html.replaceAll("&#44;", ",");
		html = html.replaceAll("&#45;", "-");
		html = html.replaceAll("&#46;", ".");
		html = html.replaceAll("(?i)(&#47;|&frasl;)", "/");
		html = html.replaceAll("&#48;", "0");
		html = html.replaceAll("&#49;", "1");
		html = html.replaceAll("&#50;", "2");
		html = html.replaceAll("&#51;", "3");
		html = html.replaceAll("&#52;", "4");
		html = html.replaceAll("&#53;", "5");
		html = html.replaceAll("&#54;", "6");
		html = html.replaceAll("&#55;", "7");
		html = html.replaceAll("&#56;", "8");
		html = html.replaceAll("&#57;", "9");
		html = html.replaceAll("&#58;", ":");
		html = html.replaceAll("&#59;", ";");
		html = html.replaceAll("(?i)(&#60;|&lt;)", "<");
		html = html.replaceAll("&#61;", "=");
		html = html.replaceAll("(?i)(&#62;|&gt;)", ">");
		html = html.replaceAll("&#63;", "?");
		html = html.replaceAll("&#64;", "@");
		html = html.replaceAll("&#65;", "A");
		html = html.replaceAll("&#66;", "B");
		html = html.replaceAll("&#67;", "C");
		html = html.replaceAll("&#68;", "D");
		html = html.replaceAll("&#69;", "E");
		html = html.replaceAll("&#70;", "F");
		html = html.replaceAll("&#71;", "G");
		html = html.replaceAll("&#72;", "H");
		html = html.replaceAll("&#73;", "I");
		html = html.replaceAll("&#74;", "J");
		html = html.replaceAll("&#75;", "K");
		html = html.replaceAll("&#76;", "L");
		html = html.replaceAll("&#77;", "M");
		html = html.replaceAll("&#78;", "N");
		html = html.replaceAll("&#79;", "O");
		html = html.replaceAll("&#80;", "P");
		html = html.replaceAll("&#81;", "Q");
		html = html.replaceAll("&#82;", "R");
		html = html.replaceAll("&#83;", "S");
		html = html.replaceAll("&#84;", "T");
		html = html.replaceAll("&#85;", "U");
		html = html.replaceAll("&#86;", "V");
		html = html.replaceAll("&#87;", "W");
		html = html.replaceAll("&#88;", "X");
		html = html.replaceAll("&#89;", "Y");
		html = html.replaceAll("&#90;", "Z");
		html = html.replaceAll("&#91;", "[");
		html = html.replaceAll("&#92;", "\\");
		html = html.replaceAll("&#93;", "]");
		html = html.replaceAll("&#94;", "^");
		html = html.replaceAll("&#95;", "_");
		html = html.replaceAll("&#96;", "`");
		html = html.replaceAll("&#97;", "a");
		html = html.replaceAll("&#98;", "b");
		html = html.replaceAll("&#99;", "c");
		html = html.replaceAll("&#100;", "d");
		html = html.replaceAll("&#101;", "e");
		html = html.replaceAll("&#102;", "f");
		html = html.replaceAll("&#103;", "g");
		html = html.replaceAll("&#104;", "h");
		html = html.replaceAll("&#105;", "i");
		html = html.replaceAll("&#106;", "j");
		html = html.replaceAll("&#107;", "k");
		html = html.replaceAll("&#108;", "l");
		html = html.replaceAll("&#109;", "m");
		html = html.replaceAll("&#110;", "n");
		html = html.replaceAll("&#111;", "o");
		html = html.replaceAll("&#112;", "p");
		html = html.replaceAll("&#113;", "q");
		html = html.replaceAll("&#114;", "r");
		html = html.replaceAll("&#115;", "s");
		html = html.replaceAll("&#116;", "t");
		html = html.replaceAll("&#117;", "u");
		html = html.replaceAll("&#118;", "v");
		html = html.replaceAll("&#119;", "w");
		html = html.replaceAll("&#120;", "x");
		html = html.replaceAll("&#121;", "y");
		html = html.replaceAll("&#122;", "z");
		html = html.replaceAll("&#123;", "{");
		html = html.replaceAll("&#124;", "|");
		html = html.replaceAll("&#125;", "}");
		html = html.replaceAll("&#126;", "~");
		html = html.replaceAll("(?i)(&#150;|&ndash;)", "–");
		html = html.replaceAll("(?i)(&#151;|&mdash;)", "—");
		html = html.replaceAll("(?i)(&#160;|&nbsp;)", " ");
		html = html.replaceAll("(?i)(&#161;|&iexcl;)", "¡");
		html = html.replaceAll("(?i)(&#162;|&cent;)", "¢");
		html = html.replaceAll("(?i)(&#163;|&pound;)", "£");
		html = html.replaceAll("(?i)(&#164;|&curren;)", "¤");
		html = html.replaceAll("(?i)(&#165;|&yen;)", "¥");
		html = html.replaceAll("(?i)(&#166;|&brvbar;|&brkbar;)", "¦");
		html = html.replaceAll("(?i)(&#167;|&sect;)", "§");
		html = html.replaceAll("(?i)(&#168;|&uml;|&die;)", "¨");
		html = html.replaceAll("(?i)(&#169;|&copy;)", "©");
		html = html.replaceAll("(?i)(&#170;|&ordf;)", "ª");
		html = html.replaceAll("(?i)(&#171;|&laquo;)", "«");
		html = html.replaceAll("(?i)(&#172;|&not;)", "¬");
		html = html.replaceAll("(?i)(&#173;|&shy;)", "");
		html = html.replaceAll("(?i)(&#174;|&reg;)", "®");
		html = html.replaceAll("(?i)(&#175;|&macr;|&hibar;)", "¯");
		// TODO complete this list
		// (http://www.webmonkey.com/reference/Special_Characters/)
		
		html = html.replaceAll("&#?+\\w*+;", "");  // drop invalid codes
		
		return html;
	}
	
	/**
	 * Converts a snippet with HTML tags and special characters into plain text.
	 * 
	 * @param snippet HTML snippet
	 * @return plain text
	 */
	public static synchronized String htmlsnippet2text(String snippet) {
		// drop HTML tags
		snippet = snippet.replaceAll("<[^>]*+>", "");
		
		// handle special characters
		snippet = replaceSpecialCharacters(snippet);
		
		// replace sequences of whitespaces by single blanks and trim
		snippet = snippet.replaceAll("\\s++", " ").trim();
		
		return snippet;
	}
	
	/**
	 * Converts an HTML document into plain text.
	 * 
	 * @param html HTML document
	 * @return plain text or <code>null</code> if the conversion failed
	 */
	public static synchronized String html2text(String html) {
		// convert HTML document
		StringBean sb = new StringBean();
		sb.setLinks(false);  // no links
		sb.setReplaceNonBreakingSpaces (true); // replace non-breaking spaces
	    sb.setCollapse(true);  // replace sequences of whitespaces
		Parser parser = new Parser();
		try {
			parser.setInputHTML(html);
			parser.visitAllNodesWith(sb);
		} catch (ParserException e) {
			return null;
		}
		String docText = sb.getStrings();
		
		if (docText == null) docText = "";  // no content
		
		return docText;
	}
	
	/**
	 * Reads an HTML document from a file and converts it into plain text.
	 * 
	 * @param filename name of file containing HTML documents
	 * @return plain text or <code>null</code> if the reading or conversion failed
	 */
	public static synchronized String file2text(String filename) {
		// read from file and convert HTML document
		StringBean sb = new StringBean();
		sb.setLinks(false);  // no links
		sb.setReplaceNonBreakingSpaces (true); // replace non-breaking spaces
	    sb.setCollapse(true);  // replace sequences of whitespaces
		Parser parser = new Parser();
		try {
			parser.setResource(filename);
			parser.visitAllNodesWith(sb);
		} catch (ParserException e) {
			return null;
		}
		String docText = sb.getStrings();
		
		return docText;
	}
	
	/**
	 * Fetches an HTML document from a URL and converts it into plain text.
	 * 
	 * @param url URL of HTML document
	 * @return plain text or <code>null</code> if the fetching or conversion failed
	 */
	public static synchronized String url2text(String url) throws SocketTimeoutException {
		// connect to URL
		URLConnection conn = null;
		try {
			conn = (new URL(url)).openConnection();
			if (!(conn instanceof HttpURLConnection)) return null;  // only allow HTTP connections
		} catch (IOException e) {
			return null;
		}
		conn.setRequestProperty("User-agent","Mozilla/4.0");  // pretend to be a browser
		conn.setConnectTimeout(TIMEOUT);
		conn.setReadTimeout(TIMEOUT);
		
		// fetch URL and convert HTML document
		StringBean sb = new StringBean();
		sb.setLinks(false);  // no links
		sb.setReplaceNonBreakingSpaces(true); // replace non-breaking spaces
	    sb.setCollapse(true);  // replace sequences of whitespaces
		sb.setConnection(conn);
		String docText = sb.getStrings();
		
		return docText;
	}
//	// simple conversion using standard API components
//	public static String url2text(String url) {
//		EditorKit kit = new HTMLEditorKit();
//		Document doc = kit.createDefaultDocument();
//		doc.putProperty("IgnoreCharsetDirective", Boolean.TRUE);  // Document does not handle charsets properly
//		
//		try {
//			// create reader on HTML content
//			URLConnection conn = (HttpURLConnection) (new URL(url)).openConnection();
//			conn.setRequestProperty("User-agent","Mozilla/4.0");  // pretend to be a browser
//			BufferedReader br = new BufferedReader(new InputStreamReader(conn.getInputStream(), "ISO-8859-1"));//, "UTF-8"));
//			
//			// parse HTML content
//			kit.read(br, doc, 0);
//			
//			return doc.getText(0, doc.getLength());
//		} catch (Exception e) {
//			// print HTTP error message
//			MsgPrinter.printHttpError(e.toString());
//			return null;
//		}
//	}
}

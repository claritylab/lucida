package info.ephyra.util;

/**
 * The <code>RegexConverter</code> can be used to tranform a string into a
 * regular expression and to build a query string from a regular expression.
 * 
 * @author Nico Schlaefer
 * @version 2006-04-12
 */
public class RegexConverter {
	/**
	 * Transform a string into a regular expression that matches the string.
	 * This is done by replacing all metacharacters 'C' by '\\C'.
	 * 
	 * @param expr a string
	 * @return a regular expression that matches the string
	 */
	public static String strToRegex(String expr) {
		expr = expr.replace("\\", "\\\\").replace("|", "\\|");
		expr = expr.replace("*", "\\*").replace("+", "\\+");
		expr = expr.replace("?", "\\?").replace(".", "\\.");
		expr = expr.replace("^", "\\^").replace("$", "\\$");
		expr = expr.replace("(", "\\(").replace(")", "\\)");
		expr = expr.replace("{", "\\{").replace("}", "\\}");
		expr = expr.replace("[", "\\[").replace("]", "\\]");
		
		return expr;
	}
	
	/**
	 * Transforms a string into a regular expression that describes a regular
	 * expression that matches the string by applying the method
	 * <code>strToRegex()</code> twice.
	 * 
	 * @param expr a string
	 * @return a regular expression that describes a regular expression that
	 * 		   matches the string
	 */
	public static String strToRegex2(String expr){
		return strToRegex(strToRegex(expr));
	}
	
	/**
	 * Transforms a string into a regular expressions and adds word boundaries
	 * if the first/last character is a word character.
	 * 
	 * @param expr a string
	 * @return a regular expression with word boundaries
	 */
	public static String strToRegexWithBounds(String expr) {
		if (expr.length() == 0) return expr;
		
		expr = strToRegex(expr);
		
		if (expr.substring(0,1).matches("\\w"))
			expr = "\\b" + expr;
		if (expr.substring(expr.length() - 1, expr.length()).matches("\\w"))
			expr += "\\b";
		
		return expr;
	}
	
	/**
	 * Returns a query string that is derived from the regular expression. When
	 * learning patterns, this string should be part of the query string to
	 * ensure that both the TARGET and the seeked PROPERTY occur in the passages
	 * that are obtained from the search engine.
	 */
	// TODO improve this
	public static String regexToQueryStr(String regex) {
		// drop '\' before metacharacters
		//expr = expr.replace("\\(", "(").replace("\\)", ")");
		regex = regex.replace("\\{", "{").replace("\\}", "}");
		regex = regex.replace("\\[", "[").replace("\\]", "]");
		regex = regex.replace("\\^", "^").replace("\\$", "$");
		regex = regex.replace("\\.", ".").replace("\\|", "|");
		regex = regex.replace("\\*", "*").replace("\\+", "+");
		//expr = expr.replace("\\?", "?");
		
		// replace "\s+" by single blank
		regex = regex.replace("\\s+", " ");
		
		// drop substrings that contain '.' or '\'
		regex = regex.replaceAll("[^\\(\\)\\s\\|]*\\.\\\\[^\\(\\)\\s\\|]*", "");
		
		// replace "(?:" by simple '('
		regex = regex.replace("(?:", "(");
		
		// replace ")?" by "|)"
		regex = regex.replace(")?", "|)");
		
		return regex;
	}
}

package info.ephyra.nlp;

import info.ephyra.util.RegexConverter;
import info.ephyra.util.StringUtils;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import opennlp.tools.coref.LinkerMode;
import opennlp.tools.coref.mention.DefaultParse;
import opennlp.tools.coref.mention.Mention;
import opennlp.tools.lang.english.PosTagger;
import opennlp.tools.lang.english.SentenceDetector;
import opennlp.tools.lang.english.Tokenizer;
import opennlp.tools.lang.english.TreebankChunker;
import opennlp.tools.lang.english.TreebankLinker;
import opennlp.tools.lang.english.TreebankParser;
import opennlp.tools.parser.Parse;
import opennlp.tools.parser.ParserME;
import opennlp.tools.postag.POSDictionary;

/**
 * <p>This class provides a common interface to the
 * <a href="http://opennlp.sourceforge.net/">OpenNLP</a> toolkit.</p>
 * 
 * <p>It supports the following natural language processing tools:
 * <ul>
 * <li>Sentence detection</li>
 * <li>Tokenization/untokenization</li>
 * <li>Part of speech (POS) tagging</li>
 * <li>Chunking</li>
 * <li>Full parsing</li>
 * <li>Coreference resolution</li>
 * </ul>
 * </p>
 * 
 * @author Nico Schlaefer
 * @version 2006-05-20
 */
public class OpenNLP {
	/** Pattern for abundant blanks. More specific rules come first. T.b.c. */
	private static final Pattern ABUNDANT_BLANKS = Pattern.compile("(" +
		"\\d (st|nd|rd)\\b"			+ "|" +  // 1 st -> 1st
		"[A-Z] \\$"					+ "|" +  // US $ -> US$
		"\\d , \\d\\d\\d\\D"		+ "|" +  // 1 , 000 -> 1,000
		"\\d (\\.|:) \\d"			+ "|" +  // 1 . 99 -> 1.99
		"\\B(\\$|€|¢|£|¥|¤) \\d"	+ "|" +  // $ 100 -> $100
		"\\d (\\$|€|¢|£|¥|¤)"		+ "|" +  // 100 $ -> 100$
		" (-|/) "					+ "|" +  // one - third -> one-third
		"(\\(|\\[|\\{) "			+ "|" +  // ( ... ) -> (... )
		" (\\.|,|:|\\)|\\]|\\})"	+ ")");  // Prof . -> Prof.
	
	/** Sentence detector from the OpenNLP project. */
	private static SentenceDetector sentenceDetector;
	/** Tokenizer from the OpenNLP project. */
	private static Tokenizer tokenizer;
	/** Part of speech tagger from the OpenNLP project. */
	private static PosTagger tagger;
	/** Chunker from the OpenNLP project. */
	private static TreebankChunker chunker;
	/** Full parser from the OpenNLP project. */
	private static ParserME parser;
	/** Linker from the OpenNLP project. */
	private static TreebankLinker linker;
	
	/**
	 * Creates the sentence detector from a model file.
	 * 
	 * @param model model file
	 * @return true, iff the sentence detector was created successfully
	 */
	public static boolean createSentenceDetector(String model) {
		try {
			sentenceDetector = new SentenceDetector(model);
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Creates the tokenizer from a model file.
	 * 
	 * @param model model file
	 * @return true, iff the tokenizer was created successfully
	 */
	public static boolean createTokenizer(String model) {
		try {
			tokenizer = new Tokenizer(model);
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Creates the part of speech tagger from a model file and a case sensitive
	 * tag dictionary.
	 * 
	 * @param model model file
	 * @param tagdict case sensitive tag dictionary
	 * @return true, iff the POS tagger was created successfully
	 */
	public static boolean createPosTagger(String model, String tagdict) {
		try {
			// create POS tagger, use case sensitive tag dictionary
			tagger = new PosTagger(model, new POSDictionary(tagdict, true));
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Creates the chunker from a model file.
	 * 
	 * @param model model file
	 * @return true, iff the chunker was created successfully
	 */
	public static boolean createChunker(String model) {
		try {
			chunker = new TreebankChunker(model);
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Creates the parser from a directory containing models.
	 * 
	 * @param dir model directory
	 * @return true, iff the parser was created successfully
	 */
	public static boolean createParser(String dir) {
		try {
			// create parser, use default beamSize and advancePercentage
			parser = TreebankParser.getParser(dir);
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}

	/**
	 * Creates the linker from a directory containing models.
	 * 
	 * @param dir model directory
	 * @return true, iff the linker was created successfully
	 */
	public static boolean createLinker(String dir) {
		try {
			// create linker that works on unannotated text (TEST mode)
		    linker = new TreebankLinker(dir, LinkerMode.TEST);
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Splits a text into sentences.
	 * 
	 * @param text sequence of sentences
	 * @return array of sentences in the text or <code>null</code>, if the
	 * 		   sentence detector is not initialized
	 */
	public static String[] sentDetect(String text) {
		return (sentenceDetector != null)
			? sentenceDetector.sentDetect(text)
			: null;
	}
	
	/**
	 * A model-based tokenizer used to prepare a sentence for POS tagging.
	 * 
	 * @param text text to tokenize
	 * @return array of tokens or <code>null</code>, if the tokenizer is not
	 * 		   initialized
	 */
	public static String[] tokenize(String text) {
		return (tokenizer != null) ? tokenizer.tokenize(text) : null;
	}
	
	/**
	 * Applies the model-based tokenizer and concatenates the tokens with
	 * spaces.
	 * 
	 * @param text text to tokenize
	 * @return string of space-delimited tokens or <code>null</code>, if the
	 * 		   tokenizer is not initialized
	 */
	public static String tokenizeWithSpaces(String text) {
		String[] tokens = tokenize(text);
		return (tokens != null) ? StringUtils.concatWithSpaces(tokens) : null;
	}
	
	/**
	 * <p>Untokenizes a text by removing abundant blanks.</p>
	 * 
	 * <p>Note that it is not guaranteed that this method exactly reverts the
	 * effect of <code>tokenize()</code>.</p>
	 * 
	 * @param text text to untokenize
	 * @return text without abundant blanks
	 */
	public static String untokenize(String text) {
		Matcher m = ABUNDANT_BLANKS.matcher(text);
		while (m.find()) {
			String noBlank = "";
			for (String token : m.group(0).split(" ")) noBlank += token;
			text = text.replace(m.group(0), noBlank);
		}
		return text;
	}
	
	/**
	 * <p>Untokenizes a text by mapping it to a string that contains the
	 * original text as a subsequence.</p>
	 * 
	 * <p>Note that it is not guaranteed that this method exactly reverts the
	 * effect of <code>tokenize()</code>.</p>
	 * 
	 * @param text text to untokenize
	 * @param original string that contains the original text as a subsequence
	 * @return subsequence of the original string or the input text, iff there
	 * 		   is no such subsequence
	 */
	public static String untokenize(String text, String original) {
		// try with boundary matchers
		String regex = RegexConverter.strToRegexWithBounds(text);
		regex = regex.replace(" ", "\\s*+");
		Matcher m = Pattern.compile(regex).matcher(original);
		if (m.find()) return m.group(0);
		
		// try without boundary matchers
		regex = RegexConverter.strToRegex(text);
		regex = regex.replace(" ", "\\s*+");
		m = Pattern.compile(regex).matcher(original);
		if (m.find()) return m.group(0);
		
		// untokenization failed
		return text;
	}
	
	/**
	 * Assigns POS tags to a sentence of space-delimited tokens.
	 * 
	 * @param sentence sentence to be annotated with POS tags
	 * @return tagged sentence or <code>null</code>, if the tagger is not
	 * 		   initialized
	 */
	public static String tagPos(String sentence) {
		return (tagger != null) ? tagger.tag(sentence) : null;
	}
	
	/**
	 * Assigns POS tags to an array of tokens that form a sentence.
	 * 
	 * @param sentence array of tokens to be annotated with POS tags
	 * @return array of POS tags or <code>null</code>, if the tagger is not
	 * 		   initialized
	 */
	public static String[] tagPos(String[] sentence) {
		return (tagger != null) ? tagger.tag(sentence) : null;
	}
	
	/**
	 * Assigns chunk tags to an array of tokens and POS tags.
	 * 
	 * @param tokens array of tokens
	 * @param pos array of corresponding POS tags
	 * @return array of chunk tags or <code>null</code>, if the chunker is not
	 * 		   initialized
	 */
	public static String[] tagChunks(String[] tokens, String[] pos) {
		return (chunker != null) ? chunker.chunk(tokens, pos) : null;
	}
	
	/**
	 * Peforms a full parsing on a sentence of space-delimited tokens.
	 * 
	 * @param sentence the sentence
	 * @return parse of the sentence or <code>null</code>, if the parser is not
	 * 		   initialized or the sentence is empty
	 */
	public static Parse parse(String sentence) {
		return (parser != null && sentence.length() > 0)
			// only get first parse (that is most likely to be correct)
			? TreebankParser.parseLine(sentence, parser, 1)[0]
			: null;
	}
	
	/**
	 * Identifies coreferences in an array of full parses of sentences.
	 * 
	 * @param parses array of full parses of sentences
	 */
	public static void link(Parse[] parses) {
		int sentenceNumber = 0;
		List<Mention> document = new ArrayList<Mention>();
		
		for (Parse parse : parses) {
			DefaultParse dp = new DefaultParse(parse, sentenceNumber);
			Mention[] extents =	linker.getMentionFinder().getMentions(dp);
			
			//construct new parses for mentions which do not have constituents
			for (int i = 0; i < extents.length; i++)
				if (extents[i].getParse() == null) {
					Parse snp = new Parse(parse.getText(), extents[i].getSpan(),
										  "NML", 1.0);
					parse.insert(snp);
					extents[i].setParse(new DefaultParse(snp,sentenceNumber));
				}
			
			document.addAll(Arrays.asList(extents));
			sentenceNumber++;
	    }
		
		if (document.size() > 0) {
//			Mention[] ms = document.toArray(new Mention[document.size()]);
//			DiscourseEntity[] entities = linker.getEntities(ms);
//			TODO return results in an appropriate data structure
		}
	}
	
	private static HashSet<String> unJoinablePrepositions = new HashSet<String>(); 
	static {
		unJoinablePrepositions.add("that");
		unJoinablePrepositions.add("than");
		unJoinablePrepositions.add("which");
		unJoinablePrepositions.add("whose");
		unJoinablePrepositions.add("if");
		unJoinablePrepositions.add("such");
		unJoinablePrepositions.add("whether");
		unJoinablePrepositions.add("when");
		unJoinablePrepositions.add("where");
		unJoinablePrepositions.add("who");
	}
	
	public static String[] joinNounPhrases(String[] tokens, String[] chunkTags) {
		if (chunkTags.length < 2) return chunkTags;
		
		String[] newChunkTags = new String[chunkTags.length];
		newChunkTags[0] = chunkTags[0];
		
		for (int t = 1; t < chunkTags.length; t++) {
			if ("B-NP".equals(chunkTags[t]) && ("B-NP".equals(chunkTags[t - 1]) || "I-NP".equals(chunkTags[t - 1]))) {
				newChunkTags[t] = "I-NP";
			} else if ((t != 1) && "B-NP".equals(chunkTags[t]) && "B-PP".equals(chunkTags[t - 1]) && !unJoinablePrepositions.contains(tokens[t-1]) && ("B-NP".equals(chunkTags[t - 2]) || "I-NP".equals(chunkTags[t - 2]))) {
				newChunkTags[t - 1] = "I-NP";
				newChunkTags[t] = "I-NP";
			} else newChunkTags[t] = chunkTags[t];
		}
		
		return newChunkTags;
	}
}

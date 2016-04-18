package info.ephyra.nlp;

import java.io.StringReader;
import java.util.ArrayList;
import java.util.List;

import edu.stanford.nlp.ling.HasWord;
import edu.stanford.nlp.ling.Sentence;
import edu.stanford.nlp.ling.Word;
import edu.stanford.nlp.tagger.maxent.MaxentTagger;

/**
 * This class provides an interface to the 
 * <a href="http://nlp.stanford.edu/software/tagger.shtml">Stanford part of speech tagger</a>.
 * 
 * @author Manas Pathak
 * @version 2008-02-01
 */
public class StanfordPosTagger  {
	/**
	 * Initializes the POS Tagger
	 * 
	 * @param model model file
	 * @return true, iff the POS tagger was initialized successfully
	 */
	public static boolean init(String model) {
		try {
			new MaxentTagger(model);
		} catch (Exception e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Splits the sentence into individual tokens.
	 * 
	 * @param sentence Input sentence
	 * @return Array of tokens
	 */
	public static String[] tokenize(String sentence) {
		List t = MaxentTagger.tokenizeText(new StringReader(sentence));
		
		List<String> tokens = new ArrayList<String>();
		
		for (int j = 0; j < t.size(); j++) {
			Sentence s1 = (Sentence) t.get(j);
			
			for (int i = 0; i < s1.length(); i++) {
				HasWord w = s1.getHasWord(i);
				tokens.add(w.word());
			}
		}
		
		return (String[]) tokens.toArray(new String[tokens.size()]);
	}
	
	/**
	 * Tags the tokens with part of speech
	 * 
	 * @param tokens Array of token strings
	 * @return Part of speech tags
	 */
	public static String[] tagPos(String[] tokens) {
		Sentence untagged = createSentence(tokens);
		Sentence tagged = MaxentTagger.tagSentence(untagged);
		
		String[] pos = new String[tagged.size()];
		for (int i = 0; i < tagged.size(); i++) {
			HasWord w = (HasWord) tagged.get(i);
			String[] s = w.toString().split("/");
			if (s.length > 1)
				pos[i] = s[s.length - 1];
			else
				pos[i] = "";
		}
		
		return pos;
	}
	
	/**
	 * Combines the tokens into a <code>Sentence</code> 
	 * 
	 * @param tokens
	 * @return <code>Sentence</code> made of the tokens
	 */
	@SuppressWarnings("unchecked")
	private static Sentence createSentence(String[] tokens) {
		ArrayList<HasWord> wordList = new ArrayList<HasWord>();
		
		for (String s : tokens) {
			HasWord w = new Word(s);
			wordList.add(w);
		}
		
		Sentence sentence = new Sentence();
		sentence.setWords(wordList);
		
		return sentence;
	}
	
	/**
	 * Tags a String with part of speech
	 * 
	 * @param text Input String
	 * @return String tagged with part of speech in "word/tag" notation
	 */
	public static String tagPos(String text) {
		String[] tokens = tokenize(text);
		String[] pos = tagPos(tokens);
		
		String output = "";
		
		for (int i = 0; i < pos.length; i++) {
			output += tokens[i] + "/" + pos[i] + " ";
		}
		
		return output.trim();
	}
	
	public static void main(String[] args) throws Exception {
		String s1 = "\"Everything but the kitchen sink\" is an English phrase used to denote wildly exaggerated inclusion. It is used in phrases such as He brought everything but the kitchen sink. See for example this humorous bug report that claims that the web browser Mozilla has everything but a kitchen sink.";
		
		init("res/nlp/postagger/stanford/train-wsj-0-18.holder");
		
		System.out.println(tagPos(s1));
		
		String[] tokens = tokenize(s1);
		String[] pos = tagPos(tokens);
		for (int i = 0; i < pos.length; i++) {
			System.out.println(tokens[i] + " <- " + pos[i]);
		}
	}
}

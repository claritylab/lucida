package info.ephyra.nlp.indices;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;

/**
 * <p>A dictionary of irregular verbs in English. For each verb, the infinitive,
 * simple past and past participle forms are given.</p>
 * 
 * <p>The dictionary is case-insensitive, so it does not matter whether a verb
 * that is looked up is in upper or lower case.</p>
 * 
 * @author Nico Schlaefer
 * @version 2005-09-26
 */
public class IrregularVerbs {
	/** The infinitive forms of the irregular verbs.*/
	private static ArrayList<String> inf = new ArrayList<String>();
	/** The simple past forms of the irregular verbs.*/
	private static ArrayList<String> sp = new ArrayList<String>();
	/** The past participle forms of the irregular verbs.*/
	private static ArrayList<String> pp = new ArrayList<String>();
	
	/**
	 * Determines whether a word is in a list of words separated by "/". Does
	 * not distinguish between lower and upper case.
	 * 
	 * @param word word to test
	 * @param words words separated by "/"
	 * @return true, iff the word is in the list
	 */
	private static boolean matches(String word, String words) {
		String[] list = words.split("/");
		
		for (String elem : list)
			if (elem.toLowerCase().equals(word.toLowerCase())) return true;
		return false;
	}
	
	/**
	 * Returns the index of a verb or <code>-1</code>, if the verb is not in the
	 * dictionary.
	 * 
	 * @param verb verb to check
	 * @return index or <code>-1</code>
	 */
	private static int getIndex(String verb) {
		for (int i = 0; i < inf.size(); i++)
			if (matches(verb, inf.get(i)) ||
				matches(verb, sp.get(i)) ||
				matches(verb, pp.get(i)))
				return i;
		
		return -1;
	}
	
	/**
	 * <p>Reads the irregular verbs from a text file of the following
	 * format:</p>
	 * 
	 * <p><code>
	 * infinitive_of_verb_1 simple_past_of_verb_1 past_participle_of_verb_1<br>
	 * ...<br>
	 * infinitive_of_verb_n simple_past_of_verb_n past_participle_of_verb_n
	 * </code></p>
	 * 
	 * <p>Equivalent forms of the same verb should be separated by "/",
	 * e.g.:</p>
	 * <p><code>
	 * infinitive simple_past_1/simple_past_2 past_participle_1/past_participle_2
	 * <code></p>
	 * 
	 * @param filename name and path of the textfile
	 * @return true, iff the verbs were loaded successfully
	 */
	public static boolean loadVerbs(String filename) {
		File file = new File(filename);
		
		try {
			BufferedReader in = new BufferedReader(new FileReader(file));
			
			// read (infinitive, simple past, past participle) triples
			String[] verb;
			while (in.ready()) {
				verb = in.readLine().split(" ");
				
				inf.add(verb[0]);
				sp.add(verb[1]);
				pp.add(verb[2]);
			}
			
			in.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Looks up a verb in the dictionary. Returns
	 * <code>INF</code>/<code>SP</code>/<code>PP</code>, if the verb is in its
	 * infinitive/simple past/past participle form or <code>null</code>, if the
	 * verb is not in the dictionary.
	 * 
	 * @param verb verb to look up
	 * @return <code>INF</code>, <code>SP</code>, <code>PP</code> or
	 * 		   <code>null</code>
	 */
	public static String lookup(String verb) {
		for (String elem : inf) if (matches(verb, elem)) return "INF";
		for (String elem : sp) if (matches(verb, elem)) return "SP";
		for (String elem : pp) if (matches(verb, elem)) return "PP";
		
		return null;
	}
	
	/**
	 * Returns the infinitive forms of a verb or <code>null</code>, if the verb
	 * is regular.
	 * 
	 * @param verb verb in infinitive, simple past or past participle
	 * @return infinitive forms of the verb or <code>null</code>
	 */
	public static String[] getInfinitive(String verb) {
		int index = getIndex(verb);
		
		if (index == -1) return null;
		else return inf.get(index).split("/");
	}
	
	/**
	 * Returns the simple past forms of a verb or <code>null</code>, if the verb
	 * is regular.
	 * 
	 * @param verb verb in infinitive, simple past or past participle
	 * @return simple past forms of the verb or <code>null</code>
	 */
	public static String[] getSimplePast(String verb) {
		int index = getIndex(verb);
		
		if (index == -1) return null;
		else return sp.get(index).split("/");
	}
	
	/**
	 * Returns the past participle forms of a verb or <code>null</code>, if the
	 * verb is regular.
	 * 
	 * @param verb verb in infinitive, simple past or past participle
	 * @return past participle forms of the verb or <code>null</code>
	 */
	public static String[] getPastParticiple(String verb) {
		int index = getIndex(verb);
		
		if (index == -1) return null;
		else return pp.get(index).split("/");
	}
}

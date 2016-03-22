package info.ephyra.nlp.indices;

import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.NETagger;
import info.ephyra.util.FileUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Hashtable;

/**
 * <p>Counts the frequencies of words in an arbitrary text corpus and represents
 * them in a dictionary.</p>
 * 
 * <p>Internally, a hash table is used to store the index, which allows access
 * to the index in constant time.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-01-23
 */
public class WordFrequencies {
	/** Maximum number of words to be parsed (0 = no limit). */
	private static final int MAX_WORDS = 0;
	/** Whether words are converted to lower case. */
	private static final boolean LOWER_CASE = true;
	/** Minimum frequency of a word to remain in the index. */
	private static final int MIN_FREQUENCY = 2;
	/** Whether words are saved in the order of their frequencies. */
	private static final boolean SORT_BY_FREQUENCY = true;
	
	/** Total number of words that have been parsed. */
	private static int total;
	/** Number of distinct words in the index. */
	private static int distinct;
	/** <code>Hashtable</code> used to store (word, frequency) pairs. */
	private static Hashtable<String, Integer> index;
	
	/**
	 * Creates an index of word frequencies from an arbitrary text file.
	 * 
	 * @param filename name of the text file to parse
	 * @return true, iff the index was created successfully
	 */
	public static boolean createIndexFromFile(String filename) {
		total = 0;
		distinct = 0;
		index = new Hashtable<String, Integer>(10000);  // initial size 10,000
		
		return updateIndexFromFile(filename);
	}

	/**
	 * Updates the index with the words in an arbitrary text file.
	 * 
	 * @param filename name of the text file to parse
	 * @return true, iff the index was updated successfully
	 */
	public static boolean updateIndexFromFile(String filename) {
		MsgPrinter.printStatusMsg(filename);
		
		File file = new File(filename);
		try {
			BufferedReader in = new BufferedReader(new FileReader(file));
			
			String line;
			String[] words;
			int frequency;
			
			while (in.ready()) {
				line = in.readLine();  // read file line-by-line
				words = NETagger.tokenize(line);
				
				// update index for each word
				for (String word : words) {
					// maximum number of words reached?
					if (MAX_WORDS > 0 && total >= MAX_WORDS) return true;
					
					// convert to lower case
					if (LOWER_CASE) word = word.toLowerCase();
					
					if (index.containsKey(word))
						frequency = index.get(word).intValue();
					else {
						frequency = 0;
						distinct++;  // update number of distinct words
					}
					
					index.put(word, new Integer(++frequency));
					total++;  // update total number of words
				}
			}
			
			in.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Creates an index of word frequencies from a folder containing text files.
	 * 
	 * @param dirname name of the folder to parse
	 * @return true, iff the index was created successfully
	 */
	public static boolean createIndexFromDir(String dirname) {
		total = 0;
		distinct = 0;
		index = new Hashtable<String, Integer>(10000);  // initial size 10,000
		
		return updateIndexFromDir(dirname);
	}
	
	/**
	 * Updates the index by adding the words contained in the files in the given
	 * folder.
	 * 
	 * @param dir name of the folder to parse
	 * @return true, iff the index was updated successfully
	 */
	public static boolean updateIndexFromDir(String dir) {
		File[] files = FileUtils.getFiles(dir);
		
		// update index for each file
		for (File file : files) {
			// maximum number of words reached?
			if (MAX_WORDS > 0 && total >= MAX_WORDS) return true;
			
			if (!updateIndexFromFile(file.getPath())) return false;
		}
		
		return true;
	}
	
	/**
	 * Drops rare words from the index.
	 */
	public static void dropRareWords() {
		Hashtable<String, Integer> newIndex = new Hashtable<String, Integer>();
		
		for (String word : index.keySet()) {
			int frequency = index.get(word);
			
			if (frequency < MIN_FREQUENCY) distinct--;
			else newIndex.put(word, frequency);
		}
		
		index = newIndex;
	}
	
	/**
	 * Sorts the words in the index by their frequencies in descending order.
	 * 
	 * @return words sorted by their frequencies
	 */
	public static String[] getSortedWords() {
		String[] sorted = index.keySet().toArray(new String[index.size()]);
		
		Arrays.sort(sorted, new Comparator<String>() {
				public int compare(String s1, String s2) {
					return lookup(s2) - lookup(s1);
				}
			});
		
		return sorted;
	}
	
	/**
	 * Saves index of word frequencies to an ouput file.
	 * 
	 * @param filename name of the output file to write to
	 * @return true, iff the index was saved successfully
	 */
	public static boolean saveIndex(String filename) {
		if (index == null) return false;  // no index loaded
		
		File file = new File(filename);
		
		try {
			PrintWriter out = new PrintWriter(new FileOutputStream(file));
			
			out.println(total);  // write total number of words
			out.println(distinct);  // write number of distinct words
			
			// write (word, frequency) pairs ...
			if (SORT_BY_FREQUENCY) {
				// ... in the order of their frequencies
				String[] sorted = getSortedWords();
				for (String word : sorted) {
					out.println(word);
					out.println(lookup(word));
				}
			} else {
				// ... in an arbitrary order
				for (String word : index.keySet()) {
					out.println(word);
					out.println(lookup(word));
				}
			}
			
			out.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Loads an index of word frequencies from an input file.
	 * 
	 * @param filename name of the input file containing the index
	 * @return true, iff the index was loaded successfully
	 */
	public static boolean loadIndex(String filename) {
		File file = new File(filename);
		
		try {
			BufferedReader in = new BufferedReader(new FileReader(file));
			
			// read total number of words
			total = Integer.parseInt(in.readLine());
			// read number of distinct words
			distinct = Integer.parseInt(in.readLine());
			
			// create hash table that will have load factor of 0.5
			index = new Hashtable<String, Integer>(2 * distinct);
			
			String word;
			int frequency;
			
			// read (word, frequency) pairs
			for (int i = 0; i < distinct; i++) {
				word = in.readLine();
				frequency = Integer.parseInt(in.readLine());
				
				index.put(word, new Integer(frequency));
			}
			
			in.close();
		} catch (IOException e) {
			return false;
		}
		
		return true;
	}
	
	/**
	 * Returns the total number of words that have been parsed.
	 * 
	 * @return total number of words
	 */
	public static int getTotal() {
		return total;
	}
	
	/**
	 * Returns the number of distinct words in the index.
	 * 
	 * @return total number of distinct words
	 */
	public static int getDistinct() {
		return distinct;
	}
	
	/**
	 * Looks up a word in the index and returns its frequency. If the word is
	 * not in the index, the frequency is 0.
	 * 
	 * @param word word to look up
	 * @return frequency of the word
	 */
	public static int lookup(String word) {
		if (index == null) return 0;  // no index loaded
		
		// convert to lower case;
		if (LOWER_CASE) word = word.toLowerCase();
		
		if (index.containsKey(word))
			return (index.get(word)).intValue();
		else
			return 0;
	}
	
	/**
	 * Looks up a word in the index and returns its relative frequency. If the
	 * word is not in the index, the relative frequency is 0.
	 * 
	 * @param word word to look up
	 * @return relative frequency of the word
	 */
	public static double lookupRel(String word) {
		double frequency = lookup(word);
		
		if (total > 0) return frequency / total; else return 0; 
	}
	
	/**
	 * Entry point. Creates the index from the text files in a given folder,
	 * drops rare words and saves the index.
	 * 
	 * @param args argument 1: folder containing text files
	 * 			   argument 2: output file
	 */
	public static void main(String[] args) {
		if (args.length < 2) {
			MsgPrinter.printUsage("java WordFrequencies corpus_folder " +
								  "output_file");
			System.exit(1);
		}
		
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.printStatusMsg("Building index of word frequencies...");
		
		createIndexFromDir(args[0]);
		dropRareWords();
		saveIndex(args[1]);
		
		MsgPrinter.printStatusMsg("...completed.");
	}
}

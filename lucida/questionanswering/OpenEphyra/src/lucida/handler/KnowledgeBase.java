package lucida.handler;

// Open Ephyra packages
import info.ephyra.OpenEphyra;
import info.ephyra.search.Result;
import info.ephyra.io.MsgPrinter;

// Java packages
import java.util.List;
import java.util.ArrayList;
import java.util.Set;
import java.util.HashSet;
import java.util.Map;
import java.util.HashMap;
import java.io.File;
import java.io.FileWriter;
import java.net.URL;

//Jsoup and Indri
import org.jsoup.Jsoup;
import lemurproject.indri.IndexEnvironment;

// Interface definition
import lucida.thrift.*;

/** Class representing knowledge base.
 *  Four types of knowledge are accepted:
 *  1. Paths to Indri repositories that already exist,
 *     e.g. "path/to/Wiki_Indri_index";
 *  2. Plain texts, e.g. "The speed of light is 301,000 km/s";
 *  3. Paths to files that can be parsed by Indri,
 *     e.g. "path/to/file1.txt", "path/to/file2.pdf"
 *  4. Valid urls, e.g. "https://en.wikipedia.org/wiki/Aloe".
 */
public class KnowledgeBase {
	/**
	 * Stores knowledge bases that have not been committed to Indri repository yet. 
	 */
	public static Map<String, KnowledgeBase> active_kbs = new HashMap<String, KnowledgeBase>();
	
	/**
	 * Returns the knowledge base of the user. Creates one if it does not exist.
	 * @param LUCID ID of Lucida user
	 */
	public static KnowledgeBase getKnowledgeBase(String LUCID) {
    	KnowledgeBase kb = null;
    	if (KnowledgeBase.active_kbs.containsKey(LUCID)) {
    		kb = KnowledgeBase.active_kbs.get(LUCID);
    	} else {
    		kb = new KnowledgeBase(LUCID);
    		KnowledgeBase.active_kbs.put(LUCID, kb);
    	}
		// Create the default Indri directory if it does not exist.
		if (!new File(kb.Indri_repo).exists()) {
			QAServiceHandler.print("Creating directory " + kb.Indri_repo);
			new File(kb.Indri_repo).mkdirs();
			IndexEnvironment env = new IndexEnvironment();
			try {
				env.setStemmer("krovetz");
				env.create(kb.Indri_repo);
				env.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
    	return kb;
	}
	
	/**
	 * Path to the default Indri repository.
	 */
	private final String Indri_repo;
	
	/**
	 * Paths to the Indri repositories provided by the user
	 * through learn().
	 */
	private Set<String> custom_Indri_repos; 
	
	/**
	 * Texts provided by the user through learn().
	 * Upon infer(), they are written into individual files,
	 * added to the default Indri repository, 
	 * and cleared from memory.
	 * Thus, validity is not checked until infer().
	 */
	private List<String> texts;
	
	/**
	 * File paths provided by the user through learn().
	 * Upon infer(), they are added to the default Indri repository, 
	 * and cleared from memory.
	 * Thus, validity is not checked until infer().
	 */
	private List<String> files;
	
	/**
	 * Urls provided by the user through learn().
	 * Upon infer(), they are parsed into individual files,
	 * added to the default Indri repository, 
	 * and cleared from memory.
	 * Thus, validity is not checked until infer().
	 */
	private List<String> urls;
	
	/**
	 * Constructs an empty knowledge base.
	 * @param LUCID ID of Lucida user
	 */
	public KnowledgeBase(String LUCID) {
		// Append the time stamp to the default Indri repository path.
		Indri_repo = System.getenv("LUCIDAROOT")
				+ "/questionanswering/OpenEphyra/db/" + LUCID;
		// Initialize other instance variables.
		custom_Indri_repos = new HashSet<String>();
		texts = new ArrayList<String>();
		files = new ArrayList<String>();
		urls = new ArrayList<String>();
	}
	
	/**
	 * Adds knowledge to this knowledge base.
	 * @param knowledge knowledge input from the user
	 */
	public void addKnowledge(QuerySpec knowledge) {
		for (QueryInput i : knowledge.content) {
			switch (i.type.toLowerCase()) {
			case "text":
				for (String text : i.data) {
					texts.add(text);
				}
				break;
			case "file":
				for (String file : i.data) {
					files.add(file);
				}
				break;
			case "url":
				for (String url : i.data) {
					urls.add(url);
				}
				break;
			case "indri":
				for (String indri : i.data) {
					// Duplicates are allowed.
					// "If this set already contains the element,
					// the call leaves the set unchanged and returns false."
					custom_Indri_repos.add(indri);
				}
				break;
			default:
				// Do nothing.
				// Unrecognized types are ignored.
				break;
			}
		}
	}
	
	/*
	 * Adds texts, files, and urls to the default Indri repository.
	 */
	public void commitKnowledge() {
		IndexEnvironment env = new IndexEnvironment();
		try {
			env.setStemmer("krovetz");
			env.open(Indri_repo);
			commitTexts(env);
			commitFiles(env);
			commitUrls(env);
			env.close();
			MsgPrinter.printStatusMsg(env.documentsIndexed() + " documents indexed.");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * Adds texts to the default Indri repository.
	 * @param env Indri index environment
	 */
	private void commitTexts(IndexEnvironment env) {
		for (int i = 0; i < texts.size(); ++i) {
			try {
				// Write the text to a temporary file.
				String file_path = Indri_repo + "/text_" + i + ".txt";
				File file = new File(file_path);
				FileWriter fileWriter = new FileWriter(file);
				fileWriter.write(texts.get(i));
				fileWriter.flush();
				fileWriter.close();
				// Add the file to the Indri index environment.
				env.addFile(file_path);
				// Delete the temporary file.
				file.delete();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		// Clear texts.
		texts.clear();
	}
	
	/**
	 * Adds files to the default Indri repository.
	 * @param env Indri index environment
	 */
	private void commitFiles(IndexEnvironment env) {
		for (int i = 0; i < files.size(); ++i) {
			try {
				// Add the file to the Indri index environment.
				env.addFile(files.get(i));
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		// Clear files.
		files.clear();
	}
	
	/**
	 * Parses and adds urls to the default Indri repository.
	 * @param env Indri index environment
	 */
	private void commitUrls(IndexEnvironment env) {
		for (int i = 0; i < urls.size(); ++i) {
			try {
				// Set the time limit (connection and read timeout, in milliseconds) to
				// the maximum value an int can hold, i.e. 2^31-1.
				String parsed_text = Jsoup.parse(new URL(urls.get(i)),
						Integer.MAX_VALUE).text();
				// Write the text to a temporary file.
				String file_path = Indri_repo + "/url_" + i + ".txt";
				File file = new File(file_path);
				FileWriter file_writer = new FileWriter(file);
				file_writer.write(parsed_text);
				file_writer.flush();
				file_writer.close();
				// Add the file to the Indri index environment.
				env.addFile(file_path);
				// Delete the temporary file.
				file.delete();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		// Clear urls.
		urls.clear();
	}
	
	/**
	 * @return a list of all Indri index directories separated by ";"
	 */
	public String getIndriIndex() {
		StringBuilder sb = new StringBuilder();
		sb.append(Indri_repo);
		for (String i : custom_Indri_repos) {
			sb.append("; " + i);
		}
		return sb.toString();
	}
	
	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("******************** Summary of the knowledge base: ********************\n");
		sb.append("Indri repository: " + Indri_repo + "\n");
		sb.append("Custom Indri repositories: ");
		for (String i : custom_Indri_repos) {
			sb.append(i + " ");
		}
		sb.append("\n");
		sb.append("Recently added texts: ");
		for (String i : texts) {
			sb.append(i + " ");
		}
		sb.append("\n");
		sb.append("Recently added files: ");
		for (String i : files) {
			sb.append(i + " ");
		}
		sb.append("\n");
		sb.append("Recently added urls: ");
		for (String i : urls) {
			sb.append(i + " ");
		}
		sb.append("\n");
		sb.append("Note: Recently added items will be added to the default Indri repository\n");
		sb.append("      upon the next call of infer().\n");
		sb.append("******************** End of Summary ********************");
		return sb.toString();
	}
}

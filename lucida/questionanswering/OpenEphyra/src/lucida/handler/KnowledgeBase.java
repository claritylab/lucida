package lucida.handler;

// Open Ephyra packages
import info.ephyra.io.MsgPrinter;

// Java packages
import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.io.File;
import java.net.URL;

//Jsoup, MongoDB, and Indri.
import org.jsoup.Jsoup;
import org.bson.Document;
import com.mongodb.MongoClient; 
import com.mongodb.client.MongoCollection; 
import com.mongodb.client.MongoDatabase;
import static com.mongodb.client.model.Filters.*;
import static com.mongodb.client.model.Updates.*;
import lemurproject.indri.IndexEnvironment;
// Interface definition
import lucida.thrift.*;

/** Class representing knowledge base.
 *  Four types of knowledge are accepted:
 *  1. Plain texts, e.g. "The speed of light is 301,000 km/s";
 *  2. Valid urls, e.g. "https://en.wikipedia.org/wiki/Aloe".
 */
public class KnowledgeBase {
	/**
	 * Stores knowledge bases that have not been committed to Indri repository yet. 
	 */
	public static Map<String, KnowledgeBase> active_kbs
	= new ConcurrentHashMap<String, KnowledgeBase>();

	/**
	 * Stores the mapping from MongoDB document ID to Indri document ID
	 * for all users.
	 */
	private static MongoCollection<Document> collection;

	/**
	 * Initializes MongoDB collection for OpenEphyra.
	 */
	static {
		try {
			String mongo_addr = "localhost";
			if (System.getenv("MONGO_PORT_27017_TCP_ADDR") != null) {
				mongo_addr = System.getenv("MONGO_PORT_27017_TCP_ADDR");
			}
			@SuppressWarnings("resource")
			MongoClient mongoClient = new MongoClient(mongo_addr, 27017);
			MongoDatabase db = mongoClient.getDatabase("lucida");
			collection = db.getCollection("openephyra");
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * Returns the knowledge base of the user. Creates one if it does not exist.
	 * @param LUCID ID of Lucida user
	 * @throws Exception 
	 */
	public static KnowledgeBase getKnowledgeBase(String LUCID) throws Exception {
		KnowledgeBase kb = null;
		if (KnowledgeBase.active_kbs.containsKey(LUCID)) {
			kb = KnowledgeBase.active_kbs.get(LUCID);
		} else {
			kb = new KnowledgeBase(LUCID);
			KnowledgeBase.active_kbs.put(LUCID, kb);
		}
		// Create the default Indri directory if it does not exist.
		if (!new File(kb.Indri_path).exists()) {
			MsgPrinter.printStatusMsg("Creating directory " + kb.Indri_path);
			new File(kb.Indri_path).mkdirs();
			IndexEnvironment env = new IndexEnvironment();
			env.setStemmer("krovetz");
			env.create(kb.Indri_path);
			env.close();
		}
		return kb;
	}

	/**
	 * ID of the Lucida user.
	 */
	private final String LUCID;

	/**
	 * Path to the default Indri repository.
	 */
	private final String Indri_path;

	/**
	 * Constructs an empty knowledge base.
	 * @param LUCID ID of Lucida user
	 */
	private KnowledgeBase(String LUCID) {
		this.LUCID = LUCID;
		Indri_path = KnowledgeBase.class.getProtectionDomain().
		getCodeSource().getLocation().getFile() + "../db/" + LUCID;
		// Insert the document for id mappings if it does not exist.
		if (collection.find(eq("LUCID", LUCID)).first() == null) {
			collection.insertOne(new Document(new Document("LUCID", LUCID)));
		}
	}

	/**
	 * Adds knowledge to this knowledge base.
	 * @param knowledge knowledge input from the user
	 * @throws Exception 
	 */
	public synchronized void addKnowledge(QuerySpec knowledge) throws Exception {
		IndexEnvironment env = new IndexEnvironment();
		env.setStemmer("krovetz");
		env.open(Indri_path);
		for (QueryInput q : knowledge.content) {
			switch (q.type.toLowerCase()) {
			case "text":
				for (int i = 0; i < q.data.size(); ++i) {
					commitText(env, q.data.get(i), q.tags.get(i));
				}
				break;
			case "url":
				for (int i = 0; i < q.data.size(); ++i) {
					commitUrl(env, q.data.get(i), q.tags.get(i));
				}
				break;
			case "unlearn":
				for (int i = 0; i < q.tags.size(); ++i) {
					deleteDoc(env, q.tags.get(i));
				}
				break;
			default:
				throw new RuntimeException("Unrecognized QueryInput type: " + q.type);
			}
		}
		env.close();
		MsgPrinter.printStatusMsg(env.documentsIndexed() + " documents indexed.");
	}

	/**
	 * Adds text to the Indri repository.
	 * @param env Indri index environment
	 * @param text plain text
	 * @param doc_id document id
	 * @throws Exception 
	 */
	private void commitText(IndexEnvironment env, String text, String doc_id) throws Exception {
		// Add the text to the Indri index environment.
		int indri_id = env.addString(text, "text", new HashMap<String, String>());
		MsgPrinter.printStatusMsg("Add (doc_id, indri_id): (" + doc_id + ", "
		+ indri_id + ")");
		collection.updateOne(eq("LUCID", LUCID), set(doc_id, indri_id));
	}

	/**
	 * Parses and adds url to the Indri repository.
	 * @param env Indri index environment
	 * @param url url
	 * @param doc_id document id
	 * @throws Exception 
	 */
	private void commitUrl(IndexEnvironment env, String url, String doc_id) throws Exception {
		// Set the time limit (connection and read timeout, in milliseconds) to
		// the maximum value an int can hold, i.e. 2^31-1.
		String parsed_text = Jsoup.parse(new URL(url),
				Integer.MAX_VALUE).text();
		// Add the text to the Indri index environment.
		int indri_id = env.addString(parsed_text, "text", new HashMap<String, String>());
		MsgPrinter.printStatusMsg("parsed_text.length(): " + parsed_text.length());
		MsgPrinter.printStatusMsg("Add (doc_id, indri_id): (" + doc_id + ", "
		+ indri_id + ")");
		collection.updateOne(eq("LUCID", LUCID), set(doc_id, indri_id));
	}

	/**
	 * Deletes a document from the Indri repository.
	 * @param env Indri index environment
	 * @param doc_id document id
	 * @throws Exception 
	 */
	private void deleteDoc(IndexEnvironment env, String doc_id) throws Exception {
		Integer indri_id = collection.find(eq("LUCID", LUCID)).first().getInteger(doc_id);
		MsgPrinter.printStatusMsg("Delete (doc_id, indri_id): (" + doc_id + ", "
		+ indri_id + ")");
		if (indri_id == null) {
			throw new RuntimeException("Couldn't delete document that does not exist");
		}
		env.deleteDocument(indri_id);
	}

	/**
	 * @return a list of all Indri index directories separated by ";"
	 */
	public String getIndriIndex() {
		StringBuilder sb = new StringBuilder();
		sb.append(Indri_path);
		return sb.toString();
	}
}

package lucida.handler;

// Java packages
import java.util.Map;
import java.util.HashMap;

/** Class representing a Lucida user.
 */
public class User {
	/**
	 * A map from LUCID to the knowledge bases.
	 * that the user chooses to use by calling learn().  
	 */
	private static Map<String, KnowledgeBase> user_to_database = 
			new HashMap<String, KnowledgeBase>();
	
	/**
	 * Adds a new lucida user.
	 * @param LUCID ID of Lucida user
	 */
	public static void addUser(String LUCID) {
		// Check if the ID is already in user_to_database.
		if (user_to_database.containsKey(LUCID)) {
			throw new IllegalArgumentException();
		}
		// Add the user.
		user_to_database.put(LUCID, new KnowledgeBase(LUCID));
	}
	
	/**
	 * Returns the knowledge base of the user identified by LUCID.
	 * @param LUCID ID of Lucida user
	 */
	public static KnowledgeBase getUserKB(String LUCID) {
		// Check if the ID does not exist in user_to_database.
		if (!user_to_database.containsKey(LUCID)) {
			throw new IllegalArgumentException();
		}
		// Return the knowledge base.
		return user_to_database.get(LUCID);
	}
	
	
	
	
	
}


package info.ephyra.nlp.semantics.ontologies;

import info.ephyra.util.HashDictionary;

import java.util.Hashtable;
import java.util.List;
import java.util.Map;

import net.didion.jwnl.data.POS;
import edu.cmu.lti.javelin.qa.JavelinOntologyAdapter;

/**
 * <p>An ontology for a specific domain created from resource files.</p>
 * 
 * <p>This class extends the class <code>JavelinOntologyAdapter</code> and
 * implements the interface <code>Ontology</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-02-11
 */
public class DomainOntology extends JavelinOntologyAdapter implements Ontology {
	/** Dictionary of words in the ontology. */
	private HashDictionary dict;
	
	/**
	 * Creates the ontology for a specific domain from input files.
	 * 
	 * @param ontologyFile words and relations between them
	 * @param alternationsFile alternations of roles associated with relations
	 * @param costsFile weights for the relations
	 */
    public DomainOntology(String ontologyFile, String alternationsFile,
    		String costsFile) {
    	super(ontologyFile, alternationsFile, costsFile);
    	
    	// create dictionary of the words in the ontology
    	dict = new HashDictionary();
    	for (String relation : relationMaps.keySet()) {
    		Map<String, List<String>> relationMap = relationMaps.get(relation);
    		for (String word : relationMaps.get(relation).keySet()) {
    			dict.add(word);
    			for (String related : relationMap.get(word))
    				dict.add(related);
    		}
    	}
    }
	
	/**
	 * Looks up a word.
	 * 
	 * @param word the word to look up
	 * @return <code>true</code> iff the word was found
	 */
	public boolean contains(String word) {
		return dict.contains(word);
	}
	
	/**
	 * Expands an event by looking up related events.
	 * 
	 * @param event an event
	 * @return related events and their weights
	 */
	public Map<String, Double> expandEvent(String event) {
		Map<String, Double> expansions = new Hashtable<String, Double>();
		
        expansions.put(event, 1d);
        expansions = expandMapTransitive(expansions, "is-a");
        expansions = expandMapTransitive(expansions, "implied-by");
        expansions = expandMapTransitive(expansions, "subtype");
        expansions.remove(event);
		
		return expansions;
	}
	
	/**
	 * Expands an entity by looking up related entities.
	 * 
	 * @param entity an entity
	 * @return related entities and their weights
	 */
	public Map<String, Double> expandEntity(String entity) {
		Map<String, Double> expansions = new Hashtable<String, Double>();
		
        expansions.put(entity, 1d);
        expansions = expandMapTransitiveLeaves(expansions, "subtype");
        expansions.remove(entity);
        
		return expansions;
	}
	
	/**
	 * Expands a modifier by looking up related modifiers.
	 * 
	 * @param modifier a modifier
	 * @param pos its part of speech: <code>POS.ADJECTIVE</code> or
	 *            <code>POS.ADVERB</code>
	 * @return related modifiers and their weights
	 */
	public Map<String, Double> expandModifier(String modifier, POS pos) {
		Map<String, Double> expansions = new Hashtable<String, Double>();
		
        // domain ontologies are currently not used to expand modifiers
        
		return expansions;
	}
}

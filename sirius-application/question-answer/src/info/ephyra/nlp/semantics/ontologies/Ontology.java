package info.ephyra.nlp.semantics.ontologies;

import info.ephyra.util.Dictionary;

import java.util.Map;

import net.didion.jwnl.data.POS;

/**
 * <p>An <code>Ontology</code> comprises entities and relations between
 * them.</p>
 * 
 * <p>This interface extends the interface <code>Dictionary</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2007-02-11
 */
public interface Ontology extends Dictionary {
	/**
	 * Expands an event by looking up related events.
	 * 
	 * @param event an event
	 * @return related events and their weights
	 */
	public abstract Map<String, Double> expandEvent(String event);
	
	/**
	 * Expands an entity by looking up related entities.
	 * 
	 * @param entity an entity
	 * @return related entities and their weights
	 */
	public abstract Map<String, Double> expandEntity(String entity);
	
	/**
	 * Expands a modifier by looking up related modifiers.
	 * 
	 * @param modifier a modifier
	 * @param pos its part of speech: <code>POS.ADJECTIVE</code> or
	 *            <code>POS.ADVERB</code>
	 * @return related modifiers and their weights
	 */
	public abstract Map<String, Double> expandModifier(String modifier,
			POS pos);
}

package info.ephyra.trec;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.indices.FunctionWords;

import java.util.HashSet;

/**
 * A <code>TRECNugget</code> is a simple data structure for a nugget to be
 * covered by the results returned for an 'other' question.
 * 
 * @author Guido Sautter
 * @version 2008-02-10
 */
public class TRECNugget {
	public final String targetID;
	public final String questionID;
	public final String nuggetID;
	public final String nuggetType;
	public final String nugget;
	public final int size;
	
	/**
	 * @param	targetID	the targetID of the TREC target the OTHER question belongs to
	 * @param	questionID	the ID of the OTHER question
	 * @param	nuggetID	the ID of the nugget
	 * @param	nuggetType	the type of the nugget (okay or vital)
	 * @param	nugget		the nugget's text
	 */
	public TRECNugget(String targetID, String questionID, String nuggetID, String nuggetType, String nugget) {
		this.targetID = targetID;
		this.questionID = questionID;
		this.nuggetID = nuggetID;
		this.nuggetType = nuggetType;
		this.nugget = nugget;
		
		String[] nTokens = NETagger.tokenize(nugget);
		HashSet<String> nSet = new HashSet<String>();
		for (String n : nTokens)
			if (!FunctionWords.lookup(n) && (n.length() > 1))
				nSet.add(SnowballStemmer.stem(n).toLowerCase());
		
		this.size = nSet.size();
	}

	/** @see java.lang.Object#equals(java.lang.Object)
	 */
	public boolean equals(Object o) {
		if ((o == null) || !(o instanceof TRECNugget)) return false;
		TRECNugget nug = ((TRECNugget) o);
		return (this.targetID.equals(nug.targetID) && this.nuggetID.equals(nug.nuggetID));
	}
}

package edu.cmu.sphinx.decoder.search;

import java.util.HashMap;
import java.util.Map;
import java.util.logging.Logger;

import edu.cmu.sphinx.linguist.HMMSearchState;
import edu.cmu.sphinx.linguist.SearchState;
import edu.cmu.sphinx.linguist.WordSearchState;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;

public class SkewPruningSearchManager extends
		WordPruningBreadthFirstSearchManager {
	
	private Logger logger;
	boolean pruneHMM = false;
	int skew = 2;

	private Map<SearchState, Token> skewMap;

	@Override
    protected boolean allowExpansion(Token t) {
		if (pruneHMM)
			return skewPruneHMM(t);
		return skewPruneWord(t);
	}

    /*
     * (non-Javadoc)

     *
     * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
     */
     @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
    	 logger = ps.getLogger();
     }
   
	@Override
    protected void localStart() {
		skewMap = new HashMap<SearchState, Token>();
	}

	/**
	 * Frame skew based on HMM states
	 * 
	 * @param t
	 *            the token to test
	 * @return <code>true</code> if the token should be expanded
	 */
	private boolean skewPruneHMM(Token t) {
		boolean keep = true;
		SearchState ss = t.getSearchState();
		if (ss instanceof HMMSearchState) {
			if (!t.isEmitting()) {
				// HMMSearchState hss = (HMMSearchState) ss;
				Token lastToken = skewMap.get(ss);
				if (lastToken != null) {
					int lastFrame = lastToken.getFrameNumber();
					if (t.getFrameNumber() - lastFrame > skew
							|| t.getScore() > lastToken.getScore()) {
						keep = true;
					} else {
						logger.fine("Dropped " + t + " in favor of " + lastToken);
						keep = false;
					}

				} else {
					keep = true;
				}

				if (keep) {
					skewMap.put(ss, t);
				}
			}
		}
		return keep;
	}

	/**
	 * Frame skew based on word states
	 * 
	 * @param t
	 *            the token to test
	 * @return <code>true</code> if the token should be expanded
	 */
	private boolean skewPruneWord(Token t) {
		boolean keep = true;
		SearchState ss = t.getSearchState();
		if (ss instanceof WordSearchState) {
			Token lastToken = skewMap.get(ss);
			if (lastToken != null) {
				int lastFrame = lastToken.getFrameNumber();
				if (t.getFrameNumber() - lastFrame > skew
						|| t.getScore() > lastToken.getScore()) {
					keep = true;
				} else {
					logger.fine("Dropped " + t + " in favor of " + lastToken);
					keep = false;
				}

			} else {
				keep = true;
			}

			if (keep) {
				skewMap.put(ss, t);
			}
		}
		return keep;
	}

}

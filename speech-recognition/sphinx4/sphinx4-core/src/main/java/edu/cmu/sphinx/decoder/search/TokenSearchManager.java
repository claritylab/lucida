package edu.cmu.sphinx.decoder.search;

import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Boolean;

abstract public class TokenSearchManager implements SearchManager {

    /** The property that specifies whether to build a word lattice. */
    @S4Boolean(defaultValue = true)
    public final static String PROP_BUILD_WORD_LATTICE = "buildWordLattice";

    /**
     * The property that controls whether or not we keep all tokens. If this is
     * set to false, only word tokens are retained, otherwise all tokens are
     * retained.
     */
    @S4Boolean(defaultValue = false)
    public final static String PROP_KEEP_ALL_TOKENS = "keepAllTokens";

    protected boolean buildWordLattice;
    protected boolean keepAllTokens;

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util
     * .props.PropertySheet)
     */
    public void newProperties(PropertySheet ps) throws PropertyException {       
        buildWordLattice = ps.getBoolean(PROP_BUILD_WORD_LATTICE);
        keepAllTokens = ps.getBoolean(PROP_KEEP_ALL_TOKENS);
    }

    /**
     * Find the token to use as a predecessor in resultList given a candidate
     * predecessor. There are three cases here:
     * 
     * <ul>
     * <li>We want to store everything in resultList. In that case
     * {@link #keepAllTokens} is set to true and we just store everything that
     * was built before.
     * <li>We are only interested in sequence of words. In this case we just
     * keep word tokens and ignore everything else. In this case timing and
     * scoring information is lost since we keep scores in emitting tokens.
     * <li>We want to keep words but we want to keep scores to build a lattice
     * from the result list later and {@link #buildWordLattice} is set to true.
     * In this case we want to insert intermediate token to store the score and
     * this token will be used during lattice path collapse to get score on
     * edge. See {@link edu.cmu.sphinx.result.Lattice} for details of resultList
     * compression.
     * </ul>
     * 
     * @param token
     *            the token of interest
     * @return the immediate successor word token
     */
    protected Token getResultListPredecessor(Token token) {

        if (keepAllTokens) {
            return token;
        }

        if(!buildWordLattice) {
            if (token.isWord())
                return token;
            else
                return token.getPredecessor();
        }

        float logAcousticScore = 0.0f;
        float logLanguageScore = 0.0f;
        float logInsertionScore = 0.0f;

        while (token != null && !token.isWord()) {
            logAcousticScore += token.getAcousticScore();
            logLanguageScore += token.getLanguageScore();
            logInsertionScore += token.getInsertionScore();
            token = token.getPredecessor();
        }

        return new Token(token, token.getScore(), logInsertionScore, logAcousticScore, logLanguageScore);
    }

}

package edu.cmu.sphinx.decoder.scorer;

import edu.cmu.sphinx.decoder.search.SimpleBreadthFirstSearchManager;
import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

import java.util.List;
import java.util.logging.Logger;

/**
 * Normalizes a set of Tokens against the best scoring Token of a background model.
 *
 * @author Holger Brandl
 */
public class BackgroundModelNormalizer implements ScoreNormalizer {

    /**
     * The active list provider used to determined the best token for normalization. If this reference is not defined no
     * normalization will be applied.
     */
    @S4Component(type = SimpleBreadthFirstSearchManager.class, mandatory = false)
    public static final String ACTIVE_LIST_PROVIDER = "activeListProvider";
    private SimpleBreadthFirstSearchManager activeListProvider;

    private Logger logger;

    public BackgroundModelNormalizer() {       
    }

    public void newProperties(PropertySheet ps) throws PropertyException {
        this.activeListProvider = (SimpleBreadthFirstSearchManager) ps.getComponent(ACTIVE_LIST_PROVIDER);
        this.logger = ps.getLogger();

        logger.warning("no active list set.");
    }

    /**
     * @param activeListProvider The active list provider used to determined the best token for normalization. If this reference is not defined no
     * normalization will be applied.
     */
    public BackgroundModelNormalizer(SimpleBreadthFirstSearchManager activeListProvider) {
        this.activeListProvider = activeListProvider;
        this.logger = Logger.getLogger(getClass().getName());

        logger.warning("no active list set.");
    }
    
    public Scoreable normalize(List<? extends Scoreable> scoreableList, Scoreable bestToken) {
        if (activeListProvider == null) {
            return bestToken;
        }

        Token normToken = activeListProvider.getActiveList().getBestToken();

        assert bestToken.getFrameNumber() == normToken.getFrameNumber() - 1 : "frame numbers should be equal for a meaningful normalization";

        float normScore = normToken.getScore();

        for (Scoreable scoreable : scoreableList) {
            if (scoreable instanceof Token) {
                scoreable.normalizeScore(normScore);
            }
        }

        return bestToken;
    }
}

package edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi;

import java.util.Arrays;

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.FloatData;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.ScoreCachingSenone;
import edu.cmu.sphinx.util.LogMath;

/**
 * Gaussian Mixture Model with diagonal covariances.
 *
 * @see DiagGmm class in Kaldi.
 */
@SuppressWarnings("serial")
public class DiagGmm extends ScoreCachingSenone {

    private int id;
    private float[] gconsts;
    private float[] invVars;
    private float[] meansInvVars;

    /**
     * Constructs new mixture model.
     *
     * @param   id     identifier of this GMM as defined in the model
     * @param   parser text format parser
     */
    public DiagGmm(int id, KaldiTextParser parser) {
        this.id = id;

        parser.expectToken("<DiagGMM>");
        parser.expectToken("<GCONSTS>");
        gconsts = parser.getFloatArray();

        parser.expectToken("<WEIGHTS>");
        // Do not use weights as they are in gconsts.
        parser.getFloatArray();

        parser.expectToken("<MEANS_INVVARS>");
        meansInvVars = parser.getFloatArray();

        parser.expectToken("<INV_VARS>");
        invVars = parser.getFloatArray();
        parser.expectToken("</DiagGMM>");
    }

    /**
     * Convenient method if 32-bit ID is required.
     *
     * Kaldi model uses 32-bit integer to store GMM id while Senone contract
     * imposes long type. This method is present to avaoid type cast when
     * working in the Kaldi domain.
     */
    public int getId() {
        return id;
    }

    @Override
    public float calculateScore(Data data) {
        float logTotal = LogMath.LOG_ZERO;
        LogMath logMath = LogMath.getInstance();
        for (Float mixtureScore : calculateComponentScore(data))
            logTotal = logMath.addAsLinear(logTotal, mixtureScore);

        return logTotal;
    }

    public float[] calculateComponentScore(Data data) {
        float[] features = FloatData.toFloatData(data).getValues();
        int dim = meansInvVars.length / gconsts.length;

        if (features.length != dim) {
            String fmt = "feature vector must be of length %d, got %d";
            String msg = String.format(fmt, dim, features.length);
            throw new IllegalArgumentException(msg);
        }

        float[] likelihoods = Arrays.copyOf(gconsts, gconsts.length);
        for (int i = 0; i < likelihoods.length; ++i) {
            for (int j = 0; j < features.length; ++j) {
                int k = i * features.length + j;
                likelihoods[i] += meansInvVars[k] * features[j];
                likelihoods[i] -= .5f * invVars[k] * features[j] * features[j];
            }

            likelihoods[i] = LogMath.getInstance().lnToLog(likelihoods[i]);
        }

        return likelihoods;
    }

    public long getID() {
        return id;
    }

    public void dump(String msg) {
        System.out.format("%s DiagGmm: ID %d\n", msg, id);
    }
}

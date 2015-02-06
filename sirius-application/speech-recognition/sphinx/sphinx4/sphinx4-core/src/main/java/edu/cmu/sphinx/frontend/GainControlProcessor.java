package edu.cmu.sphinx.frontend;

import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Double;

/**
 * Allows to modify the gain of an audio-signal.  If the gainFactor is 1 the signal passes this
 * <code>DataProcessor</code> unchanged.
 *
 * @author Holger Brandl
 */
public class GainControlProcessor extends BaseDataProcessor {

    @S4Double(defaultValue = 1.0)
    public static final String GAIN_FACTOR = "gainFactor";

    private double gainFactor;

    public GainControlProcessor(double gainFactor) {
        initLogger();
        this.gainFactor = gainFactor;
    }

    public GainControlProcessor() {        
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        gainFactor = ps.getDouble(GAIN_FACTOR);
    }


    @Override
    public Data getData() throws DataProcessingException {
        Data data = getPredecessor().getData();

        if (data instanceof FloatData) {
            float[] values = ((FloatData) data).getValues();
            if (gainFactor != 1.0) {
                // apply the gain-factor
                for (int i = 0; i < values.length; i++) {
                    values[i] *= gainFactor;

                }
            }

        } else if (data instanceof DoubleData) {
            double[] values = ((DoubleData) data).getValues();
            if (gainFactor != 1.0) {
                // apply the gain-factor
                for (int i = 0; i < values.length; i++) {
                    values[i] *= gainFactor;

                }
            }
        }

        return data;
    }


    public double getGainFactor() {
        return gainFactor;
    }


    public void setGainFactor(double gainFactor) {
        this.gainFactor = gainFactor;
    }
}

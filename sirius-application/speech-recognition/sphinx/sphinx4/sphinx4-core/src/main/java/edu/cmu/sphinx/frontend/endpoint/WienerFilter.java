/*
 * Copyright 2010 PC-NG Inc.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.frontend.endpoint;


import edu.cmu.sphinx.frontend.BaseDataProcessor;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DataProcessingException;
import edu.cmu.sphinx.frontend.DoubleData;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

/**
 * The noise Wiener filter. Parameters are taken from the article
 * "An Effective Subband OSF-Based VAD With Noise Reduction 
 * for Robust Speech Recognition" by Ramirez et all. IEEE
 * Transactions on Speech And Audio Processing, Vol 13, No 6, 2005
 * <br/>
 * Subband VAD is not implemented yet, default endpointer is used.
 * The frontend configuration with filtering should look like:
 *  <br/>
 *  <br/>
 *  &lt;item&gt;audioFileDataSource &lt;/item&gt;<br/>
 *  &lt;item&gt;dataBlocker &lt;/item&gt;<br/>
 *  &lt;item&gt;preemphasizer &lt;/item&gt;<br/>
 *  &lt;item&gt;windower &lt;/item&gt;<br/>
 *  &lt;item&gt;fft &lt;/item&gt;<br/>
 *  &lt;item&gt;wiener &lt;/item&gt;<br/>
 *  &lt;item&gt;speechClassifier &lt;/item&gt;<br/>
 *  &lt;item&gt;speechMarker &lt;/item&gt;<br/>
 *  &lt;item&gt;nonSpeechDataFilter &lt;/item&gt;<br/>                 
 *  &lt;item&gt;melFilterBank &lt;/item&gt;<br/>             
 *  &lt;item&gt;dct &lt;/item&gt;<br/>
 *  &lt;item&gt;liveCMN &lt;/item&gt;<br/>
 *  &lt;item&gt;featureExtraction &lt;/item&gt;<br/>
 */
public class WienerFilter extends BaseDataProcessor {

    double[] prevNoise;
    double[] prevSignal;
    double[] prevInput;

    double lambda = 0.99;
    double gamma = 0.98;
    double etaMin = 1e-2;
    protected AbstractVoiceActivityDetector classifier;

    /** The name of the transform matrix file */
    @S4Component(type = AbstractVoiceActivityDetector.class)
    public final static String PROP_CLASSIFIER = "classifier";

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util
     * .props.PropertySheet)
     */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        classifier = (AbstractVoiceActivityDetector) ps.getComponent(PROP_CLASSIFIER);
    }

    @Override
    public Data getData() throws DataProcessingException {
        Data inputData = getPredecessor().getData();

        /* signal, reset smoother */
        if (!(inputData instanceof DoubleData)) {
            prevNoise = null;
            prevSignal = null;
            prevInput = null;
            return inputData;
        }
        DoubleData inputDoubleData = (DoubleData) inputData;
        double[] input = inputDoubleData.getValues();
        int length = input.length;

        /* no previous data, just return input */
        if (prevNoise == null) {
            prevNoise = new double[length];
            prevSignal = new double[length];
            prevInput = new double[length];
            return inputData;
        }

        double[] smoothedInput = smooth(input);
        double[] noise = estimateNoise(smoothedInput);
        double[] signal = filter(input, smoothedInput, noise);

        System.arraycopy(noise, 0, prevNoise, 0, length);
        System.arraycopy(signal, 0, prevSignal, 0, length);
        System.arraycopy(input, 0, prevInput, 0, length);

        DoubleData outputData = new DoubleData(signal, inputDoubleData
                .getSampleRate(),
                inputDoubleData.getFirstSampleNumber());

        return outputData;
    }

    private double[] filter(double[] input, double[] smoothedInput,
            double[] noise) {
        int length = input.length;
        double[] signal = new double[length];

        for (int i = 0; i < length; i++) {
            double max = Math.max(smoothedInput[i] - noise[i], 0);
            double s = gamma * prevSignal[i] + (1 - gamma) * max;
            double eta = Math.max(s / noise[i], etaMin);
            signal[i] = eta / (1 + eta) * input[i];
        }
        return signal;
    }

    private double[] estimateNoise(double[] smoothedInput) {
        int length = smoothedInput.length;
        double[] noise = new double[length];

        for (int i = 0; i < length; i++) {
            if (classifier.isSpeech()) {
                noise[i] = prevNoise[i];
            } else {
                noise[i] = lambda * prevNoise[i] + (1 - lambda)
                        * smoothedInput[i];
            }
        }
        return noise;
    }

    private double[] smooth(double[] input) {
        int length = input.length;
        double[] smoothedInput = new double[length];

        for (int i = 1; i < length - 1; i++) {
            smoothedInput[i] = (input[i] + input[i - 1] + input[i + 1] + prevInput[i]) / 4;
        }
        smoothedInput[0] = (input[0] + input[1] + prevInput[0]) / 3;
        smoothedInput[length - 1] = (input[length - 1] + input[length - 2] + prevInput[length - 1]) / 3;

        return smoothedInput;
    }
}

/*
 * Copyright 2013 Carnegie Mellon University. All Rights Reserved. Use is
 * subject to license terms. See the file "license.terms" for information on
 * usage and redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
package edu.cmu.sphinx.frontend;

import static java.lang.Double.parseDouble;
import static java.lang.Integer.parseInt;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import edu.cmu.sphinx.frontend.denoise.Denoise;
import edu.cmu.sphinx.frontend.frequencywarp.MelFrequencyFilterBank;
import edu.cmu.sphinx.frontend.frequencywarp.MelFrequencyFilterBank2;
import edu.cmu.sphinx.frontend.transform.*;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.KaldiLoader;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.Loader;
import edu.cmu.sphinx.util.props.*;


/**
 * Cepstrum is an auto-configurable DataProcessor which is used to compute a
 * specific cepstrum (for a target acoustic model) given the spectrum. The
 * Cepstrum is computed using a pipeline of front end components which are
 * selected, customized or ignored depending on the feat.params file which
 * characterizes the target acoustic model for which this cepstrum is computed.
 * A typical legacy MFCC Cepstrum will use a MelFrequencyFilterBank, followed
 * by a DiscreteCosineTransform. A typical denoised MFCC Cepstrum will use a
 * MelFrequencyFilterBank, followed by a Denoise component, followed by a
 * DiscreteCosineTransform2, followed by a Lifter component. The
 * MelFrequencyFilterBank parameters (numberFilters, minimumFrequency and
 * maximumFrequency) are auto-configured based on the values found in
 * feat.params.
 * 
 * @author Horia Cucu
 */
public class AutoCepstrum extends BaseDataProcessor {

    /**
     * The property specifying the acoustic model for which this cepstrum will
     * be configured. For this acoustic model (AM) it is mandatory to specify a
     * location in the configuration file. The Cepstrum will be configured
     * based on the feat.params file that will be found in the specified AM
     * location.
     */
    @S4Component(type = Loader.class)
    public final static String PROP_LOADER = "loader";
    protected Loader loader;

    /**
     * The filter bank which will be used for creating the cepstrum. The filter
     * bank is always inserted in the pipeline and its minimum frequency,
     * maximum frequency and number of filters are configured based on the
     * "lowerf", "upperf" and "nfilt" values in the feat.params file of the
     * target acoustic model.
     */
    protected BaseDataProcessor filterBank;

    /**
     * The denoise component which could be used for creating the cepstrum. The
     * denoise component is inserted in the pipeline only if
     * "-remove_noise yes" is specified in the feat.params file of the target
     * acoustic model.
     */
    protected Denoise denoise;

    /**
     * The property specifying the DCT which will be used for creating the
     * cepstrum. If "-transform legacy" is specified in the feat.params file of
     * the target acoustic model or if the "-transform" parameter does not
     * appear in this file at all, the legacy DCT component is inserted in the
     * pipeline. If "-transform dct" is specified in the feat.params file of
     * the target acoustic model, then the current DCT component is inserted in
     * the pipeline.
     */
    protected DiscreteCosineTransform dct;

    /**
     * The lifter component which could be used for creating the cepstrum. The
     * lifter component is inserted in the pipeline only if
     * "-lifter <lifterValue>" is specified in the feat.params file of the
     * target acoustic model.
     */
    protected Lifter lifter;

    /**
     * The list of <code>DataProcessor</code>s which were auto-configured for
     * this Cepstrum component.
     */
    protected List<DataProcessor> selectedDataProcessors;

    public AutoCepstrum(Loader loader) throws IOException {
        initLogger();
        this.loader = loader;
        loader.load();
        initDataProcessors();
    }

    public AutoCepstrum() {
    }

    /*
     * (non-Javadoc)
     * @see
     * edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util
     * .props.PropertySheet)
     */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        loader = (Loader) ps.getComponent(PROP_LOADER);
        try {
            loader.load();
        } catch (IOException e) {
            throw new PropertyException(e);
        }
        initDataProcessors();
    }

    private void initDataProcessors() {
        try {
            Properties featParams = loader.getProperties();
            selectedDataProcessors = new ArrayList<DataProcessor>();

            double lowFreq = parseDouble(featParams.getProperty("-lowerf"));
            double hiFreq = parseDouble(featParams.getProperty("-upperf"));
            int numFilter = parseInt(featParams.getProperty("-nfilt"));

            // TODO: should not be there, but for now me must preserve
            // backward compatibility with the legacy code.
            if (loader instanceof KaldiLoader)
                filterBank = new MelFrequencyFilterBank2(lowFreq,
                                                         hiFreq,
                                                         numFilter);
            else
                filterBank = new MelFrequencyFilterBank(lowFreq,
                                                        hiFreq,
                                                        numFilter);

            selectedDataProcessors.add(filterBank);

            if ((featParams.get("-remove_noise") != null)
                    && (featParams.get("-remove_noise").equals("yes"))) {
                denoise = new Denoise(Denoise.class.getField("LAMBDA_POWER")
                                              .getAnnotation(S4Double.class)
                                              .defaultValue(),
                                      Denoise.class.getField("LAMBDA_A")
                                              .getAnnotation(S4Double.class)
                                              .defaultValue(),
                                      Denoise.class.getField("LAMBDA_B")
                                              .getAnnotation(S4Double.class)
                                              .defaultValue(),
                                      Denoise.class.getField("LAMBDA_T")
                                              .getAnnotation(S4Double.class)
                                              .defaultValue(),
                                      Denoise.class.getField("MU_T")
                                              .getAnnotation(S4Double.class)
                                              .defaultValue(),
                                      Denoise.class
                                              .getField("EXCITATION_THRESHOLD")
                                              .getAnnotation(S4Double.class)
                                              .defaultValue(),
                                      Denoise.class.getField("MAX_GAIN")
                                              .getAnnotation(S4Double.class)
                                              .defaultValue(),
                                      Denoise.class.getField("SMOOTH_WINDOW")
                                              .getAnnotation(S4Integer.class)
                                              .defaultValue());
                // denoise.newProperties();
                denoise.setPredecessor(selectedDataProcessors
                        .get(selectedDataProcessors.size() - 1));
                selectedDataProcessors.add(denoise);
            }

            if ((featParams.get("-transform") != null)
                    && (featParams.get("-transform").equals("dct"))) {
                dct = new DiscreteCosineTransform2(
                                                   numFilter,
                                                   DiscreteCosineTransform.class
                                                           .getField("PROP_CEPSTRUM_LENGTH")
                                                           .getAnnotation(S4Integer.class)
                                                           .defaultValue());
            } else if ((featParams.get("-transform") != null)
                    && (featParams.get("-transform").equals("kaldi")))
            {
                dct = new KaldiDiscreteCosineTransform(
                                                       numFilter,
                                                       DiscreteCosineTransform.class
                                                               .getField("PROP_CEPSTRUM_LENGTH")
                                                               .getAnnotation(S4Integer.class)
                                                               .defaultValue());
            } else {
                dct = new DiscreteCosineTransform(numFilter,
                                                  DiscreteCosineTransform.class
                                                          .getField("PROP_CEPSTRUM_LENGTH")
                                                          .getAnnotation(S4Integer.class)
                                                          .defaultValue());
            }
            dct.setPredecessor(selectedDataProcessors
                    .get(selectedDataProcessors.size() - 1));
            selectedDataProcessors.add(dct);

            if (featParams.get("-lifter") != null) {
                lifter = new Lifter(Integer.parseInt((String) featParams
                        .get("-lifter")));
                lifter.setPredecessor(selectedDataProcessors
                        .get(selectedDataProcessors.size() - 1));
                selectedDataProcessors.add(lifter);
            }
            logger.info("Cepstrum component auto-configured as follows: "
                    + toString());
        } catch (NoSuchFieldException exc) {
            throw new RuntimeException(exc);
        }
    }

    /*
     * (non-Javadoc)
     * @see
     * edu.cmu.sphinx.frontend.DataProcessor#initialize(edu.cmu.sphinx.frontend
     * .CommonConfig)
     */
    @Override
    public void initialize() {
        super.initialize();

        for (DataProcessor dataProcessor : selectedDataProcessors)
            dataProcessor.initialize();
    }

    /**
     * Returns the processed Data output, basically calls
     * <code>getData()</code> on the last processor.
     * 
     * @return a Data object that has been processed by the cepstrum
     * @throws DataProcessingException if a data processor error occurs
     */
    @Override
    public Data getData() throws DataProcessingException {
        DataProcessor dp;
        dp = selectedDataProcessors.get(selectedDataProcessors.size() - 1);
        return dp.getData();
    }

    /**
     * Sets the predecessor for this DataProcessor. The predecessor is actually
     * the spectrum builder.
     * 
     * @param predecessor the predecessor of this DataProcessor
     */
    @Override
    public void setPredecessor(DataProcessor predecessor) {
        filterBank.setPredecessor(predecessor);
    }

    /**
     * Returns a description of this Cepstrum component in the format:
     * <cepstrum name> {<DataProcessor1>, <DataProcessor2> ...
     * <DataProcessorN>}
     * 
     * @return a description of this Cepstrum
     */
    @Override
    public String toString() {
        StringBuilder description = new StringBuilder(super.toString())
                .append(" {");
        for (DataProcessor dp : selectedDataProcessors)
            description.append(dp).append(", ");
        description.setLength(description.length() - 2);
        return description.append('}').toString();
    }

}

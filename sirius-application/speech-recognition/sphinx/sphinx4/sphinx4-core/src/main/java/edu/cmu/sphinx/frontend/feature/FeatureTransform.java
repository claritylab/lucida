/*
 * Copyright 1999-2010 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * Portions Copyright 2008 PC-NG Inc.
 * 
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.frontend.feature;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.*;
import edu.cmu.sphinx.util.props.*;

/**
 * Implements a linear feature transformation transformation.
 *
 * It might be a dimension reduction or just a decorrelation transform. This
 * component requires a special model trained with LDA/MLLT transform.
 */
public class FeatureTransform extends BaseDataProcessor {

    /**
     * The name of the transform matrix file.
     */
    @S4Component(type = Loader.class)
    public final static String PROP_LOADER = "loader";

    float[][] transform;
    protected Loader loader;

    int rows;
    int values;

    public FeatureTransform(Loader loader) {
        initLogger();
        init(loader);
    }

    public FeatureTransform() {
    }

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
        init((Loader) ps.getComponent(PROP_LOADER));
    }

    private void init(Loader loader) {
        this.loader = loader;

        try {
            loader.load();
        } catch (Exception e) {
            e.printStackTrace();
        }

        transform = loader.getTransformMatrix();
    }

    /**
     * Returns the next Data object being processed by this LDA, or if it is a
     * Signal, it is returned without modification.
     * 
     * @return the next available Data object, returns null if no Data object is
     *         available
     * @throws DataProcessingException
     *             if there is a processing error
     * @see Data
     */
    @Override
    public Data getData() throws DataProcessingException {
        Data data = getPredecessor().getData();

        if (null == transform || null == data || !(data instanceof FloatData))
            return data;

        FloatData floatData = (FloatData) data; 
        float[] features = floatData.getValues();

        if (features.length > transform[0].length + 1)
            throw new IllegalArgumentException("dimenstion mismatch");

        float[] result = new float[transform.length];
        getTimer().start();

        for (int i = 0; i < transform.length; ++i) {
            for (int j = 0; j < features.length; ++j)
                result[i] += transform[i][j] * features[j];
        }

        if (features.length > transform[0].length) {
            for (int i = 0; i < transform.length; ++i)
                result[i] += transform[i][features.length];
        }

        getTimer().stop();

        return new FloatData(result,
                             floatData.getSampleRate(),
                             floatData.getFirstSampleNumber());
    }
}

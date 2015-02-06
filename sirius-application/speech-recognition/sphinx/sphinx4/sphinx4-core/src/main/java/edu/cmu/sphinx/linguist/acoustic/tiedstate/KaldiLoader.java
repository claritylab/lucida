/*
 * Copyright 2013 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 */

package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import edu.cmu.sphinx.linguist.acoustic.HMMPosition;
import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi.KaldiGmmPool;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi.KaldiTextParser;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi.TransitionModel;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;
import edu.cmu.sphinx.util.props.S4String;


public class KaldiLoader implements Loader {

    @S4Component(type = UnitManager.class)
    public final static String PROP_UNIT_MANAGER = "unitManager";
    private String location;

    @S4String(mandatory = true)
    public final static String PROP_LOCATION = "location";
    private UnitManager unitManager;

    private Pool<Senone> senonePool;
    private HMMManager hmmManager;
    private Properties modelProperties;
    private Map<String, Unit> contextIndependentUnits;
    private float[][] transform;

    /**
     * Constructs empty object.
     *
     * Does nothing but is required for instantiation from the context object.
     */
    public KaldiLoader() {
    }

    public KaldiLoader(String location, UnitManager unitManager) {
        init(location, unitManager);
    }

    public void init(String location, UnitManager unitManager) {
        this.location = location;
        this.unitManager = unitManager;
    }

    public void newProperties(PropertySheet ps) throws PropertyException {
        init(ps.getString(PROP_LOCATION),
             (UnitManager) ps.getComponent(PROP_UNIT_MANAGER));
    }

    /**
     * Loads the acoustic model.
     *
     * @throws IOException if an error occurs while loading the model
     */
    public void load() throws IOException {
        KaldiTextParser parser = new KaldiTextParser(location);
        TransitionModel transitionModel = new TransitionModel(parser);
        senonePool = new KaldiGmmPool(parser);

        File file = new File(location, "phones.txt");
        InputStream stream = new URL(file.getPath()).openStream();
        Reader reader = new InputStreamReader(stream);
        BufferedReader br = new BufferedReader(reader);
        Map<String, Integer> symbolTable = new HashMap<String, Integer>();
        String line;

        while (null != (line = br.readLine())) {
            String[] fields = line.split(" ");
            if (Character.isLetter(fields[0].charAt(0)))
                symbolTable.put(fields[0], Integer.parseInt(fields[1]));
        }

        contextIndependentUnits = new HashMap<String, Unit>();
        hmmManager = new LazyHmmManager(parser, transitionModel,
                                        senonePool, symbolTable);

        for (String phone : symbolTable.keySet()) {
            Unit unit = unitManager.getUnit(phone, "SIL".equals(phone));
            contextIndependentUnits.put(unit.getName(), unit);
            // Ensure monophone HMMs are created.
            hmmManager.get(HMMPosition.UNDEFINED, unit);
        }

        loadTransform();
        loadProperties();
    }

    private void loadTransform() throws IOException {
        URL transformUrl = new URL(new File(location, "final.mat").getPath());
        Reader reader = new InputStreamReader(transformUrl.openStream());
        BufferedReader br = new BufferedReader(reader);
        List<Float> values = new ArrayList<Float>();
        int numRows = 0;
        int numCols = 0;
        String line;

        while (null != (line = br.readLine())) {
            int colCount = 0;

            for (String word : line.split("\\s+")) {
                if (word.isEmpty() || "[".equals(word) || "]".equals(word))
                    continue;

                values.add(Float.parseFloat(word));
                ++colCount;
            }

            if (colCount > 0)
                ++numRows;

            numCols = colCount;
        }

        transform = new float[numRows][numCols];
        Iterator<Float> valueIterator = values.iterator();

        for (int i = 0; i < numRows; ++i) {
            for (int j = 0; j < numCols; ++j)
                transform[i][j] = valueIterator.next();
        }
    }

    private void loadProperties() throws IOException {
        File file = new File(location, "feat.params");
        InputStream stream = new URL(file.getPath()).openStream();
        Reader reader = new InputStreamReader(stream);
        BufferedReader br = new BufferedReader(reader);
        modelProperties = new Properties();
        String line;

        while ((line = br.readLine()) != null) {
            String[] tokens = line.split(" ");
            modelProperties.put(tokens[0], tokens[1]);
        }
    }

    /**
     * Gets the senone pool for this loader.
     *
     * @return the pool
     */
    public Pool<Senone> getSenonePool() {
        return senonePool;
    }

    /**
     * Returns the HMM Manager for this loader.
     *
     * @return the HMM Manager
     */
    public HMMManager getHMMManager() {
        return hmmManager;
    }

    /**
     * Returns the map of context indepent units.
     *
     * The map can be accessed by unit name.
     *
     * @return the map of context independent units
     */
    public Map<String, Unit> getContextIndependentUnits() {
        return contextIndependentUnits;
    }

    /**
     * Returns the size of the left context for context dependent units.
     *
     * @return the left context size
     */
    public int getLeftContextSize() {
        return 1;
    }

    /**
     * Returns the size of the right context for context dependent units.
     *
     * @return the right context size
     */
    public int getRightContextSize() {
        return 1;
    }

    /**
     * Returns the model properties
     */
    public Properties getProperties() {
        return modelProperties;
    }

    /**
     * Logs information about this loader
     */
    public void logInfo() {
    }

    /**
     * Not implemented.
     */
    public Pool<float[]> getMeansPool() {
        return null;
    }

    /**
     * Not implemented.
     */
    public Pool<float[][]> getMeansTransformationMatrixPool() {
        return null;
    }

    /**
     * Not implemented.
     */
    public Pool<float[]> getMeansTransformationVectorPool() {
        return null;
    }

    /**
     * Not implemented.
     */
    public Pool<float[]> getVariancePool() {
        return null;
    }

    /**
     * Not implemented.
     */
    public Pool<float[][]> getVarianceTransformationMatrixPool() {
        return null;
    }

    /**
     * Not implemented.
     */
    public Pool<float[]> getVarianceTransformationVectorPool() {
        return null;
    }

    /**
     * Not implemented.
     */
    public Pool<float[]> getMixtureWeightPool() {
        return null;
    }

    /**
     * Not implemented.
     */
    public Pool<float[][]> getTransitionMatrixPool() {
        return null;
    }

    /**
     * Not implemented.
     */
    public float[][] getTransformMatrix() {
        return transform;
    }
}

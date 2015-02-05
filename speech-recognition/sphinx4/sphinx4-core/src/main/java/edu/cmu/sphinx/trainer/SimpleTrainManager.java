/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.trainer;

import edu.cmu.sphinx.linguist.acoustic.AcousticModel;
import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.trainer.TrainerAcousticModel;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.trainer.TrainerScore;
import edu.cmu.sphinx.util.Utilities;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;
import edu.cmu.sphinx.util.props.S4Boolean;
import edu.cmu.sphinx.util.props.S4ComponentList;

import java.io.IOException;
import java.util.List;


/** This is a dummy implementation of a TrainManager. */
public class SimpleTrainManager implements TrainManager {

	@S4Component(type = ControlFile.class)
	public static final String CONTROL_FILE = "control";
    private ControlFile controlFile;

    private boolean dumpMemoryInfo;

    @S4Component(type = Learner.class)
    public static final String LEARNER = "learner";
    private Learner learner;
    
    @S4Component(type = Learner.class)
    public static final String INIT_LEARNER = "initLearner";
    private Learner initLearner;

    @S4Component(type = UnitManager.class)
    public static final String UNIT_MANAGER = "unitManager";
    private UnitManager unitManager;

    @S4ComponentList(type = TrainerAcousticModel.class)
    public static final String AM_COLLECTION = "models";
    private List<? extends TrainerAcousticModel> acousticModels;

    /**
     * The property for the boolean property that controls whether or not the recognizer will display detailed
     * memory information while it is running. The default value is <code>true</code>.
     */
    @S4Boolean(defaultValue = false)
    public final static String DUMP_MEMORY_INFO = "dumpMemoryInfo";

    private int maxIteration;
    private float minimumImprovement;


    public void newProperties(PropertySheet ps) throws PropertyException {
        dumpMemoryInfo = ps.getBoolean(DUMP_MEMORY_INFO);
        learner = (Learner) ps.getComponent(LEARNER);
        controlFile = (ControlFile) ps.getComponent(CONTROL_FILE);
        initLearner = (Learner) ps.getComponent(INIT_LEARNER);
        minimumImprovement = ps.getFloat(PROP_MINIMUM_IMPROVEMENT);
        maxIteration = ps.getInt(PROP_MAXIMUM_ITERATION);        
        acousticModels = ps.getComponentList(AM_COLLECTION, TrainerAcousticModel.class);
        unitManager = (UnitManager)ps.getComponent(UNIT_MANAGER);
    }


    /** Do the train. */
    public void train() {
        assert controlFile != null;
        for (controlFile.startUtteranceIterator();
             controlFile.hasMoreUtterances();) {
            Utterance utterance = controlFile.nextUtterance();
            System.out.println(utterance);
            for (utterance.startTranscriptIterator();
                 utterance.hasMoreTranscripts();) {
                System.out.println(utterance.nextTranscript());
            }
        }
    }


    /**
     * Copy the model.
     * <p/>
     * This method copies to model set, possibly to a new location and new format. This is useful if one wants to
     * convert from binary to ascii and vice versa, or from a directory structure to a JAR file. If only one model is
     * used, then name can be null.
     *
     * @param context this TrainManager's context
     * @throws IOException if an error occurs while loading the data
     */
    public void copyModels(String context) throws IOException {
        loadModels(context);
        saveModels(context);
    }


    /**
     * Saves the acoustic models.
     *
     * @param context the context of this TrainManager
     * @throws IOException if an error occurs while loading the data
     */
    public void saveModels(String context) throws IOException {
        if (1 == acousticModels.size()) {
            acousticModels.get(0).save(null);
        } else {
            for (AcousticModel model : acousticModels) {
                if (model instanceof TrainerAcousticModel) {
                    TrainerAcousticModel tmodel =
                            (TrainerAcousticModel) model;
                    tmodel.save(model.getName());
                }
            }
        }
    }


    /**
     * Loads the acoustic models.
     *
     * @param context the context of this TrainManager
     */
    private void loadModels(String context) throws IOException {
        dumpMemoryInfo("TrainManager start");

        for (TrainerAcousticModel model : acousticModels) {
            model.load();
        }
        dumpMemoryInfo("acoustic model");

    }


    /**
     * Initializes the acoustic models.
     *
     * @param context the context of this TrainManager
     */
    public void initializeModels(String context) throws IOException {
        TrainerScore score[];

        dumpMemoryInfo("TrainManager start");

        for (TrainerAcousticModel model : acousticModels) {

            for (controlFile.startUtteranceIterator();
                 controlFile.hasMoreUtterances();) {
                Utterance utterance = controlFile.nextUtterance();
                initLearner.setUtterance(utterance);
                while ((score = initLearner.getScore()) != null) {
                    assert score.length == 1;
                    model.accumulate(0, score);
                }
            }

            // normalize() has a return value, but we can ignore it here.
            model.normalize();
        }
        dumpMemoryInfo("acoustic model");
    }


    /**
     * Trains context independent models. If the initialization stage was skipped, it loads models from files,
     * automatically.
     *
     * @param context the context of this train manager.
     * @throws IOException
     */
    public void trainContextIndependentModels(String context)
            throws IOException {
        UtteranceGraph uttGraph;
        TrainerScore[] score;
        TrainerScore[] nextScore;

        // If initialization was performed, then learner should not be
        // null. Otherwise, we need to load the models.
        if (learner == null) {
            loadModels(context);
        }


        dumpMemoryInfo("TrainManager start");

        for (TrainerAcousticModel model : acousticModels) {
            float logLikelihood;
            float lastLogLikelihood = Float.MAX_VALUE;
            float relativeImprovement = 100.0f;           
            for (int iteration = 0;
                 (iteration < maxIteration) &&
                     (relativeImprovement > minimumImprovement);
                 iteration++) {
                System.out.println("Iteration: " + iteration);
                model.resetBuffers();
                for (controlFile.startUtteranceIterator();
                     controlFile.hasMoreUtterances();) {
                    Utterance utterance = controlFile.nextUtterance();
                    uttGraph =
                        new UtteranceHMMGraph(context, utterance, model, unitManager);
                    learner.setUtterance(utterance);
                    learner.setGraph(uttGraph);
                    nextScore = null;
                    while ((score = learner.getScore()) != null) {
                        for (int i = 0; i < score.length; i++) {
                            if (i > 0) {
                                model.accumulate(i, score, nextScore);
                            } else {
                                model.accumulate(i, score);
                            }
                        }
                        nextScore = score;
                    }
                    model.updateLogLikelihood();
                }
                logLikelihood = model.normalize();
                System.out.println("Loglikelihood: " + logLikelihood);
                saveModels(context);
                if (iteration > 0) {
                    if (lastLogLikelihood != 0) {
                        relativeImprovement =
                            (logLikelihood - lastLogLikelihood) /
                                lastLogLikelihood * 100.0f;
                    } else if (lastLogLikelihood == logLikelihood) {
                        relativeImprovement = 0;
                    }
                    System.out.println("Finished iteration: " + iteration +
                        " - Improvement: " +
                        relativeImprovement);
                }
                lastLogLikelihood = logLikelihood;
            }
        }
    }


    /**
     * Conditional dumps out memory information
     *
     * @param what an additional info string
     */
    private void dumpMemoryInfo(String what) {
        if (dumpMemoryInfo) {
            Utilities.dumpMemoryInfo(what);
        }
    }
}

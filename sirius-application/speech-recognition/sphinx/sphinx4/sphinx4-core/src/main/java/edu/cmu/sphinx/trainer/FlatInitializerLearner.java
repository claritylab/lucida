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

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.util.StreamCepstrumSource;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.trainer.TrainerAcousticModel;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.trainer.TrainerScore;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

/** Provides mechanisms for computing statistics given a set of states and input data. */
public class FlatInitializerLearner implements Learner {

    @S4Component(type = FrontEnd.class)
    public static final String FRONT_END = "frontend";
    private FrontEnd frontEnd;

    @S4Component(type = StreamCepstrumSource.class)
    public static final String DATA_SOURCE = "source";
    private StreamCepstrumSource dataSource;

    private Data curFeature;


    public void newProperties(PropertySheet ps) throws PropertyException {
        dataSource = (StreamCepstrumSource) ps.getComponent(DATA_SOURCE);

        frontEnd = (FrontEnd) ps.getComponent(FRONT_END);
        frontEnd.setDataSource(dataSource);
    }


    /**
     * Sets the learner to use a utterance.
     *
     * @param utterance the utterance
     * @throws IOException
     */
    public void setUtterance(Utterance utterance) throws IOException {
        String file = utterance.toString();
        InputStream is = new FileInputStream(file);
        
        dataSource.setInputStream(is, false);
    }


    /**
     * Returns a single frame of speech.
     *
     * @return a feature frame
     * @throws IOException
     */
    private boolean getFeature() {
        try {
            curFeature = frontEnd.getData();

            if (curFeature == null) {
                return false;
            }

            if (curFeature instanceof DataStartSignal) {
                curFeature = frontEnd.getData();
                if (curFeature == null) {
                    return false;
                }
            }

            if (curFeature instanceof DataEndSignal) {
                return false;
            }

            if (curFeature instanceof Signal) {
                throw new Error("Can't score non-content feature");
            }

        } catch (DataProcessingException dpe) {
            System.out.println("DataProcessingException " + dpe);
            dpe.printStackTrace();
            return false;
        }

        return true;
    }


    /** Starts the Learner. */
    public void start() {
    }


    /** Stops the Learner. */
    public void stop() {
    }


    /**
     * Initializes computation for current utterance and utterance graph.
     *
     * @param utterance the current utterance
     * @param graph     the current utterance graph
     * @throws IOException
     */
    public void initializeComputation(Utterance utterance,
                                      UtteranceGraph graph) throws IOException {
        setUtterance(utterance);
        setGraph(graph);
    }


    /**
     * Implements the setGraph method. Since the flat initializer does not need a graph, this method produces an error.
     *
     * @param graph the graph
     */
    public void setGraph(UtteranceGraph graph) {
        throw new Error("Flat initializer does not use a graph!");
    }


    /**
     * Gets the TrainerScore for the next frame
     *
     * @return the TrainerScore
     */
    public TrainerScore[] getScore() {
        // If getFeature() is true, curFeature contains a valid
        // Feature. If not, a problem or EOF was encountered.
        if (getFeature()) {
            // Since it's flat initialization, the probability is
            // neutral, and the senone means "all senones".
            TrainerScore[] score = new TrainerScore[1];
            score[0] = new TrainerScore(curFeature, 0.0f,
                    TrainerAcousticModel.ALL_MODELS);
            return score;
        } else {
            return null;
        }
    }
}

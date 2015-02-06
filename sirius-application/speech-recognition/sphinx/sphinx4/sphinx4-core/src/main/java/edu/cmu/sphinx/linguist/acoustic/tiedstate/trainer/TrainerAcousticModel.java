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

package edu.cmu.sphinx.linguist.acoustic.tiedstate.trainer;

import edu.cmu.sphinx.linguist.acoustic.tiedstate.Loader;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.Saver;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.TiedStateAcousticModel;
import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.util.props.*;

import java.io.FileNotFoundException;
import java.io.IOException;

/** Represents the generic interface to the Acoustic Model for sphinx4 */
public class TrainerAcousticModel extends TiedStateAcousticModel {

	@S4Component(type = Saver.class)
	public static final String SAVER = "saver";
	private Saver saver;

	@S4Double(defaultValue = 0.0001f)
	public final static String PROP_VARIANCE_FLOOR = "varianceFloor";

	/** Mixture component score floor. */
	@S4Double(defaultValue = 0.0)
	public final static String PROP_MC_FLOOR = "MixtureComponentScoreFloor";

	/** Mixture weight floor. */
	@S4Double(defaultValue = 1e-7f)
	public final static String PROP_MW_FLOOR = "mixtureWeightFloor";

	/**
	 * The save format for the acoustic model data. Current supported formats
	 * are:
	 */
	@S4String(defaultValue = "sphinx3.binary")
	public final static String PROP_FORMAT_SAVE = "saveFormat";
	public String saveFormat;

	/** Flag indicating all models should be operated on. */
	public final static int ALL_MODELS = -1;

	/** The pool manager */
	private HMMPoolManager hmmPoolManager;

	public TrainerAcousticModel(Loader loader, UnitManager unitManager,
			boolean useComposites, Saver saver, String saveFormat) throws IOException {
		super(loader, unitManager, useComposites);

		this.saver = saver;
		this.hmmPoolManager = new HMMPoolManager(loader);
		this.saveFormat = saveFormat;

		logInfo();
	}

	public TrainerAcousticModel() {
	}

	@Override
    public void newProperties(PropertySheet ps) throws PropertyException {
		super.newProperties(ps);

		saver = (Saver) ps.getComponent(SAVER);
		try {
			hmmPoolManager = new HMMPoolManager(loader);
		} catch (IOException e) {
			throw new PropertyException(e);
		}
		saveFormat = ps.getString(PROP_FORMAT_SAVE);

		logInfo();
	}

	/**
	 * Saves the acoustic model with a given name and format
	 * 
	 * @param name
	 *            the name of the acoustic model
	 * @throws IOException
	 *             if the model could not be loaded
	 * @throws FileNotFoundException
	 *             if the model does not exist
	 */
	public void save(String name) throws IOException {
		saver.save(name, true);
		logger.info("saved models with " + saver);
	}

	/**
	 * Loads the acoustic models. This has to be explicitly requested in this
	 * class.
	 * 
	 * @throws IOException
	 *             if the model could not be loaded
	 * @throws FileNotFoundException
	 *             if the model does not exist
	 */
	public void load() throws IOException, FileNotFoundException {
		// super.load();
		logInfo();
		hmmPoolManager = new HMMPoolManager(loader);
	}

	/** Resets the buffers. */
	public void resetBuffers() {
		// Resets the buffers and associated variables.
		hmmPoolManager.resetBuffers();
	}

	/**
	 * Accumulate the current TrainerScore into the buffers.
	 * 
	 * @param index
	 *            the current index into the TrainerScore vector
	 * @param trainerScore
	 *            the TrainerScore in the current frame
	 * @param nextTrainerScore
	 *            the TrainerScore in the next frame
	 */
	public void accumulate(int index, TrainerScore[] trainerScore,
			TrainerScore[] nextTrainerScore) {
		hmmPoolManager.accumulate(index, trainerScore, nextTrainerScore);
	}

	/**
	 * Accumulate the current TrainerScore into the buffers.
	 * 
	 * @param index
	 *            the current index into the TrainerScore vector
	 * @param trainerScore
	 *            the TrainerScore
	 */
	public void accumulate(int index, TrainerScore[] trainerScore) {
		hmmPoolManager.accumulate(index, trainerScore);
	}

	/**
	 * Update the log likelihood. This should be called at the end of each
	 * utterance.
	 */
	public void updateLogLikelihood() {
		hmmPoolManager.updateLogLikelihood();
	}

	/**
	 * Normalize the buffers and update the models.
	 * 
	 * @return the log likelihood for the whole training set
	 */
	public float normalize() {
		float logLikelihood = hmmPoolManager.normalize();
		hmmPoolManager.update();
		return logLikelihood;
	}

}

package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.util.props.Configurable;

import java.io.File;

/**
 * An interface which is describes the functionality which is required to handle new file signals fired by the
 * audio-data sources.
 *
 * @author Holger Brandl
 */

public interface AudioFileProcessListener extends Configurable {

    /**
     * This method is invoked whenever a new file is started to become processed by an audio file data source.
     *
     * @param audioFile The name of the new audio file.
     */
    public void audioFileProcStarted(File audioFile);


    /**
     * This method is invoked whenever a file processing has finished within a audio file data source.
     *
     * @param audioFile The name of the processed audio file.
     */
    public void audioFileProcFinished(File audioFile);
}

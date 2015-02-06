/*
 * Copyright 1999-2002 Carnegie Mellon University.
 * Portions Copyright 2002 Sun Microsystems, Inc.
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */

package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.util.props.*;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

/**
 * An AudioFileDataSource generates a stream of audio data from a given audio file. All required information concerning
 * the audio format are read directly from the file . One would need to call {@link #setAudioFile(java.io.File,String)}
 * to set the input file, and call {@link #getData} to obtain the Data frames.
 * <p/>
 * Using JavaSound as backend this class is able to handle all sound files supported by JavaSound. Beside the built-in
 * support for .wav, .au and .aiff. Using plugins (cf.  http://www.jsresources.org/ ) it can be extended to support
 * .ogg, .mp3, .speex and others.
 *
 * @author Holger Brandl
 */

public class AudioFileDataSource extends BaseDataProcessor {

    /** The property for the number of bytes to read from the InputStream each time. */
    @S4Integer(defaultValue = 3200)
    public static final String PROP_BYTES_PER_READ = "bytesPerRead";

    @S4ComponentList(type = Configurable.class)
    public static final String AUDIO_FILE_LISTENERS = "audioFileListners";
    protected final List<AudioFileProcessListener> fileListeners = new ArrayList<AudioFileProcessListener>();


    protected InputStream dataStream;
    protected int sampleRate;
    protected int bytesPerRead;
    protected int bytesPerValue;
    private long totalValuesRead;
    protected boolean bigEndian;
    protected boolean signedData;
    private boolean streamEndReached;
    private boolean utteranceEndSent;
    private boolean utteranceStarted;

    private File curAudioFile;

    public AudioFileDataSource(int bytesPerRead, List<AudioFileProcessListener> listeners) {
	initLogger();
        create(bytesPerRead,listeners);
    }

    public AudioFileDataSource() {
    }
    
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        logger = ps.getLogger();
        create(ps.getInt(PROP_BYTES_PER_READ), ps.getComponentList(AUDIO_FILE_LISTENERS, AudioFileProcessListener.class));
    }

    private void create( int bytesPerRead, List<AudioFileProcessListener> listeners ) {
        this.bytesPerRead = bytesPerRead;

        if( listeners != null ) {
            // attach all pool-listeners
            for (AudioFileProcessListener configurable : listeners) {
                addNewFileListener(configurable);
            }
        }

        initialize();
    }


    @Override
    public void initialize() {
        super.initialize();

        // reset all stream tags
        streamEndReached = false;
        utteranceEndSent = false;
        utteranceStarted = false;

        if (bytesPerRead % 2 == 1) {
            bytesPerRead++;
        }
    }


    /**
     * Sets the audio file from which the data-stream will be generated of.
     *
     * @param audioFile  The location of the audio file to use
     * @param streamName The name of the InputStream. if <code>null</code> the complete path of the audio file will be
     *                   uses as stream name.
     */
    public void setAudioFile(File audioFile, String streamName) {
        try {
            setAudioFile(audioFile.toURI().toURL(), streamName);
        } catch (MalformedURLException e) {
            e.printStackTrace();
        }
    }


    /**
     * Sets the audio file from which the data-stream will be generated of.
     *
     * @param audioFileURL The location of the audio file to use
     * @param streamName   The name of the InputStream. if <code>null</code> the complete path of the audio file will be
     *                     uses as stream name.
     */
    public void setAudioFile(URL audioFileURL, String streamName) {
        // first close the last stream if there's such a one
        if (dataStream != null) {
            try {
                dataStream.close();
            } catch (IOException e) {
                e.printStackTrace();
            }

            dataStream = null;
        }

        assert audioFileURL != null;
        if (streamName != null)
            streamName = audioFileURL.getPath();

        AudioInputStream audioStream = null;
        try {
            audioStream = AudioSystem.getAudioInputStream(audioFileURL);
        } catch (UnsupportedAudioFileException e) {
            System.err.println("Audio file format not supported: " + e);
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

        curAudioFile = new File(audioFileURL.getFile());
        for (AudioFileProcessListener fileListener : fileListeners)
            fileListener.audioFileProcStarted(curAudioFile);

        setInputStream(audioStream, streamName);
    }


    /**
     * Sets the InputStream from which this StreamDataSource reads.
     *
     * @param inputStream the InputStream from which audio data comes
     * @param streamName  the name of the InputStream
     */
    public void setInputStream(AudioInputStream inputStream, String streamName) {
        dataStream = inputStream;
        streamEndReached = false;
        utteranceEndSent = false;
        utteranceStarted = false;

        AudioFormat format = inputStream.getFormat();
        sampleRate = (int) format.getSampleRate();
        bigEndian = format.isBigEndian();

        String s = format.toString();
        logger.finer("input format is " + s);

        if (format.getSampleSizeInBits() % 8 != 0)
            throw new Error("StreamDataSource: bits per sample must be a multiple of 8.");
        bytesPerValue = format.getSampleSizeInBits() / 8;

        // test whether all files in the stream have the same format

        AudioFormat.Encoding encoding = format.getEncoding();
        if (encoding.equals(AudioFormat.Encoding.PCM_SIGNED))
            signedData = true;
        else if (encoding.equals(AudioFormat.Encoding.PCM_UNSIGNED))
            signedData = false;
        else
            throw new RuntimeException("used file encoding is not supported");

        totalValuesRead = 0;
    }


    /**
     * Reads and returns the next Data from the InputStream of StreamDataSource, return null if no data is read and end
     * of file is reached.
     *
     * @return the next Data or <code>null</code> if none is available
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     *          if there is a data processing error
     */
    @Override
    public Data getData() throws DataProcessingException {
        getTimer().start();
        Data output = null;
        if (streamEndReached) {
            if (!utteranceEndSent) {
                // since 'firstSampleNumber' starts at 0, the last
                // sample number should be 'totalValuesRead - 1'
                output = createDataEndSignal();
                utteranceEndSent = true;
            }
        } else {
            if (!utteranceStarted) {
                utteranceStarted = true;
                output = new DataStartSignal(sampleRate);
            } else {
                if (dataStream != null) {
                    output = readNextFrame();
                    if (output == null) {
                        if (!utteranceEndSent) {
                            output = createDataEndSignal();
                            utteranceEndSent = true;
                        }
                    }
                }
            }
        }
        getTimer().stop();
        return output;
    }


    private DataEndSignal createDataEndSignal() {
        if (!(this instanceof ConcatAudioFileDataSource))
            for (AudioFileProcessListener fileListener : fileListeners)
                fileListener.audioFileProcFinished(curAudioFile);

        return new DataEndSignal(getDuration());
    }


    /**
     * Returns the next Data from the input stream, or null if there is none available
     *
     * @return a Data or null
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     */
    private Data readNextFrame() throws DataProcessingException {
        // read one frame's worth of bytes
        int read;
        int totalRead = 0;
        final int bytesToRead = bytesPerRead;
        byte[] samplesBuffer = new byte[bytesPerRead];
        long firstSample = totalValuesRead;
        try {
            do {
                read = dataStream.read(samplesBuffer, totalRead, bytesToRead
                        - totalRead);
                if (read > 0) {
                    totalRead += read;
                }
            } while (read != -1 && totalRead < bytesToRead);
            if (totalRead <= 0) {
                closeDataStream();
                return null;
            }
            // shrink incomplete frames
            totalValuesRead += (totalRead / bytesPerValue);
            if (totalRead < bytesToRead) {
                totalRead = (totalRead % 2 == 0)
                        ? totalRead + 2
                        : totalRead + 3;
                byte[] shrinkedBuffer = new byte[totalRead];
                System
                        .arraycopy(samplesBuffer, 0, shrinkedBuffer, 0,
                                totalRead);
                samplesBuffer = shrinkedBuffer;
                closeDataStream();
            }
        } catch (IOException ioe) {
            throw new DataProcessingException("Error reading data", ioe);
        }
        // turn it into an Data object
        double[] doubleData;
        if (bigEndian) {
            doubleData = DataUtil.bytesToValues(samplesBuffer, 0, totalRead, bytesPerValue, signedData);
        } else {
            doubleData = DataUtil.littleEndianBytesToValues(samplesBuffer, 0, totalRead, bytesPerValue, signedData);
        }

        return new DoubleData(doubleData, sampleRate, firstSample);
    }


    private void closeDataStream() throws IOException {
        streamEndReached = true;
        if (dataStream != null) {
            dataStream.close();
        }
    }


    /**
     * Returns the duration of the current data stream in milliseconds.
     *
     * @return the duration of the current data stream in milliseconds
     */
    private long getDuration() {
        return (long) (((double) totalValuesRead / (double) sampleRate) * 1000.0);
    }


    public int getSampleRate() {
        return sampleRate;
    }


    public boolean isBigEndian() {
        return bigEndian;
    }


    /** Adds a new listener for new file events.
     * @param l*/
    public void addNewFileListener(AudioFileProcessListener l) {
        if (l == null)
            return;

        fileListeners.add(l);
    }


    /** Removes a listener for new file events.
     * @param l*/
    public void removeNewFileListener(AudioFileProcessListener l) {
        if (l == null)
            return;

        fileListeners.remove(l);
    }
}


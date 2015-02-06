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
package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Boolean;
import edu.cmu.sphinx.util.props.S4Integer;

import java.io.IOException;
import java.io.InputStream;

/**
 * A StreamDataSource converts data from an InputStream into Data objects. One 
 * would call {@link #setInputStream(InputStream,String) setInputStream} to set 
 * the input stream, and call {@link #getData} to obtain the Data object. The 
 * InputStream can be an arbitrary stream, for example a data from the network
 * or from a pipe.
 *
 * StreamDataSource is not aware about incoming data format and assumes
 * that incoming data matches StreamDataSource configuration. By default it's configured
 * to read 16 kHz little-endian 16-bit signed raw data. If data has wrong format 
 * the result of the recognition is undefined. Also note that the sample rate of the 
 * data must match the sample required by the the acoustic model. If your
 * model decodes 16 kHz files you can't recognize 8kHz data using it.
 *
 * You can use AudioFileDataSource instead to read the file headers and
 * to convert incoming data to the required format automatically.
 */
public class StreamDataSource extends BaseDataProcessor {

    /** The property for the sample rate. */
    @S4Integer(defaultValue = 16000)
    public static final String PROP_SAMPLE_RATE = "sampleRate";

    /** The property for the number of bytes to read from the InputStream each time. */
    @S4Integer(defaultValue = 3200)
    public static final String PROP_BYTES_PER_READ = "bytesPerRead";

    /** The property for the number of bits per value. */
    @S4Integer(defaultValue = 16)
    public static final String PROP_BITS_PER_SAMPLE = "bitsPerSample";

    /** The property specifying whether the input data is big-endian. */
    @S4Boolean(defaultValue = false)
    public static final String PROP_BIG_ENDIAN_DATA = "bigEndianData";

    /** The property specifying whether the input data is signed. */
    @S4Boolean(defaultValue = true)
    public static final String PROP_SIGNED_DATA = "signedData";

    private InputStream dataStream;
    protected int sampleRate;
    private int bytesPerRead;
    private int bytesPerValue;
    private long totalValuesRead;
    private boolean bigEndian;
    private boolean signedData;
    private boolean streamEndReached;
    private boolean utteranceEndSent;
    private boolean utteranceStarted;
    protected int bitsPerSample;

    public StreamDataSource(int sampleRate, int bytesPerRead, int bitsPerSample, boolean bigEndian, boolean signedData ) {
        initLogger();
        init(sampleRate,bytesPerRead,bitsPerSample,bigEndian,signedData );
    }

    public StreamDataSource() {

    }
    
    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        init(
             ps.getInt(PROP_SAMPLE_RATE),
             ps.getInt(PROP_BYTES_PER_READ),
             ps.getInt(PROP_BITS_PER_SAMPLE),
             ps.getBoolean(PROP_BIG_ENDIAN_DATA),
             ps.getBoolean(PROP_SIGNED_DATA));
    }

    private void init(int sampleRate, int bytesPerRead, int bitsPerSample, boolean bigEndian, boolean signedData ) {
        this.sampleRate = sampleRate;
        this.bytesPerRead = bytesPerRead;
        this.bitsPerSample = bitsPerSample;

        if (this.bitsPerSample % 8 != 0)
            throw new IllegalArgumentException(
                    "bits per sample must be a multiple of 8");
        
        this.bytesPerValue = bitsPerSample / 8;
        this.bigEndian = bigEndian;
        this.signedData = signedData;
        this.bytesPerRead += bytesPerRead % 2;
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.frontend.DataProcessor#initialize(edu.cmu.sphinx.frontend.CommonConfig)
    */
    @Override
    public void initialize() {
        super.initialize();
    }


    /**
     * Sets the InputStream from which this StreamDataSource reads.
     *
     * @param inputStream the InputStream from which audio data comes
     */
    public void setInputStream(InputStream inputStream) {
        dataStream = inputStream;
        streamEndReached = false;
        utteranceEndSent = false;
        utteranceStarted = false;
        totalValuesRead = 0;
    }


    /**
     * Reads and returns the next Data from the InputStream of StreamDataSource, return null if no data is read and end
     * of file is reached.
     *
     * @return the next Data or <code>null</code> if none is available
     * @throws DataProcessingException if there is a data processing error
     */
    @Override
    public Data getData() throws DataProcessingException {
        getTimer().start();
        Data output = null;
        if (streamEndReached) {
            if (!utteranceEndSent) {
                // since 'firstSampleNumber' starts at 0, the last
                // sample number should be 'totalValuesRead - 1'
                output = new DataEndSignal(getDuration());
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
                            output = new DataEndSignal(getDuration());
                            utteranceEndSent = true;
                        }
                    }
                } else {
                    logger.warning("Input stream is not set");
                    if (!utteranceEndSent) {
                        output = new DataEndSignal(getDuration());
                        utteranceEndSent = true;
                    }
                }
            }
        }
        getTimer().stop();
        return output;
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
            doubleData = DataUtil.bytesToValues(samplesBuffer, 0, totalRead,
                    bytesPerValue, signedData);
        } else {
            doubleData = DataUtil.littleEndianBytesToValues(samplesBuffer, 0,
                    totalRead, bytesPerValue, signedData);
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
}

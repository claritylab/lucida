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
 * adapted by Christophe Cerisara june 2007
 *
 */


package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.util.Utilities;
import edu.cmu.sphinx.util.props.*;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;


/**
 * Produces Mel-cepstrum data from an InputStream. To set the input stream with cepstral data, use the {@link
 * #setInputStream(InputStream) setInputStream} method, and then call {@link #getData} to obtain the Data
 * objects that have cepstrum data in it.
 */
public class StreamHTKCepstrum extends BaseDataProcessor {

    /** The property specifying whether the input is in binary. */
    @S4Boolean(defaultValue = true)
    public final static String PROP_BINARY = "binary";

    /** The property specifying whether the input is big endian. */
    @S4Boolean(defaultValue = true)
    public final static String PROP_BIGENDIAN = "bigEndian";
    
    /** The property  name for frame size in milliseconds. */
    @S4Double(defaultValue = 25.625)
    public static final String PROP_FRAME_SIZE_MS = "frameSizeInMs";

    /** The property  name for frame shift in milliseconds, which has a default value of 10F. */
    @S4Double(defaultValue = 10.0)
    public static final String PROP_FRAME_SHIFT_MS = "frameShiftInMs";
    
    /** The property  specifying the length of the cepstrum data. */
    @S4Integer(defaultValue = 13)
    public static final String PROP_CEPSTRUM_LENGTH = "cepstrumLength";
    
    /** The property that defines the sample rate */
    @S4Integer(defaultValue = 16000)
    public static final String PROP_SAMPLE_RATE = "sampleRate";
 
    private DataInputStream binaryStream; // for binary files
    private int numPoints;
    private int curPoint;
    private int cepstrumLength;
    private int frameShift;
    private int frameSize;
    private int sampleRate;
    private long firstSampleNumber;
    private boolean bigEndian;

    public StreamHTKCepstrum( float frameShiftMs, float frameSizeMs, boolean bigEndian, int sampleRate ) {
	initLogger();
        this.bigEndian = bigEndian;
        this.sampleRate = sampleRate;
        this.frameShift = DataUtil.getSamplesPerWindow(sampleRate, frameShiftMs);
        this.frameSize = DataUtil.getSamplesPerShift(sampleRate, frameSizeMs);
    }

    public StreamHTKCepstrum( ) {
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        float frameShiftMs = ps.getFloat(PROP_FRAME_SHIFT_MS
        );
        float frameSizeMs = ps.getFloat(PROP_FRAME_SIZE_MS
        );
        bigEndian = ps.getBoolean(PROP_BIGENDIAN);
        sampleRate = ps.getInt(PROP_SAMPLE_RATE);
        frameShift = DataUtil.getSamplesPerWindow(sampleRate, frameShiftMs);
        frameSize = DataUtil.getSamplesPerShift(sampleRate, frameSizeMs);
        logger = ps.getLogger();
    }


    /** Constructs a StreamCepstrumSource that reads MelCepstrum data from the given path. */
    @Override
    public void initialize() {
        super.initialize();
        curPoint = -1;
        firstSampleNumber = 0;
        // we don't want any more that endianness be set at the upper level, as BatchReco
        // utilizes an ad-hoc procedure to decide on endianness.. We want it to be fixed in the config file
        //bigEndian = true;
    }


    /**
     * Sets the InputStream to read cepstral data from.
     *
     * @param stream the InputStream to read cepstral data from
     * @throws IOException if an I/O error occurs
     */
    public void setInputStream(InputStream stream)
            throws IOException {
        {
            // TODO: update sampleRate
            binaryStream = new DataInputStream(new BufferedInputStream(stream));
            if (bigEndian) {
            	numPoints = binaryStream.readInt();
                int sampPeriod = binaryStream.readInt();
                short sampSize = binaryStream.readShort();
                short parmKind = binaryStream.readShort();

                cepstrumLength = sampSize / 4;
                numPoints *= cepstrumLength;

                logger.info("Sample period is " + sampPeriod);
                logger.info("Sample size " + sampSize);
                logger.info("Parameter kind " + parmKind);
                logger.info("BigEndian");
            } else {
                numPoints = Utilities.readLittleEndianInt(binaryStream);

                int sampPeriod = Utilities.readLittleEndianInt(binaryStream);
                short sampSize = readLittleEndianShort(binaryStream);
                short parmKind = readLittleEndianShort(binaryStream);

                cepstrumLength = sampSize/4;
                numPoints *= cepstrumLength;

                logger.info("Sample period is " + sampPeriod);
                logger.info("Sample size " + sampSize);
                logger.info("Parameter kind " + parmKind);
                logger.info("LittleEndian");
            }
            System.out.println("Frames: " + numPoints / cepstrumLength);
        }
        curPoint = -1;
        firstSampleNumber = 0;
    }

    public static short readLittleEndianShort(DataInputStream dataStream)
            throws IOException {
        short bits = 0x0000;
        for (int shift = 0; shift < 16; shift += 8) {
            int byteRead = (0x00ff & dataStream.readByte());
            bits |= (byteRead << shift);
        }
        return bits;
    }

    /**
     * Returns the next Data object, which is the mel cepstrum of the input frame. However, it can also be other Data
     * objects like DataStartSignal.
     *
     * @return the next available Data object, returns null if no Data object is available
     * @throws DataProcessingException if a data processing error occurs
     */
    @Override
    public Data getData() throws DataProcessingException {

        Data data;

        if (curPoint == -1) {
            data = new DataStartSignal(sampleRate);
            curPoint++;
        } else if (curPoint == numPoints) {
            if (numPoints > 0) {
                firstSampleNumber =
                        (firstSampleNumber - frameShift + frameSize - 1);
            }
            // send a DataEndSignal
            int numberFrames = curPoint / cepstrumLength;
            int totalSamples = (numberFrames - 1) * frameShift + frameSize;
            long duration = (long)
                    (((double) totalSamples / (double) sampleRate) * 1000.0);

            data = new DataEndSignal(duration);

            try {
                binaryStream.close();
                curPoint++;
            } catch (IOException ioe) {
                throw new DataProcessingException("IOException closing cepstrum stream", ioe);
            }
        } else if (curPoint > numPoints) {
            data = null;
        } else {
            double[] vectorData = new double[cepstrumLength];

            for (int i = 0; i < cepstrumLength; i++) {
                try {
                        if (bigEndian) {
                            vectorData[i] = binaryStream.readFloat();
                        } else {
                            vectorData[i] = Utilities.readLittleEndianFloat(binaryStream);
                        }
                    curPoint++;
                } catch (IOException ioe) {
                    throw new DataProcessingException("IOException reading from cepstrum stream", ioe);
                }
            }

            // System.out.println("Read: " + curPoint);
            data = new DoubleData
                    (vectorData, sampleRate, firstSampleNumber);
            firstSampleNumber += frameShift;
            // System.out.println(data);
        }
        return data;
    }
}

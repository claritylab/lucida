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
import edu.cmu.sphinx.util.ExtendedStreamTokenizer;
import edu.cmu.sphinx.util.Utilities;
import edu.cmu.sphinx.util.props.*;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;


/**
 * Produces Mel-cepstrum data from an InputStream. To set the inputstream with cepstral data, use the {@link
 * #setInputStream(InputStream,boolean) setInputStream} method, and then call {@link #getData} to obtain the Data
 * objects that have cepstra data in it.
 */
public class StreamCepstrumSource extends BaseDataProcessor {

    /** The property specifying whether the input is in binary. */
    @S4Boolean(defaultValue = true)
    public final static String PROP_BINARY = "binary";

    /** The property  name for frame size in milliseconds. */
    @S4Double(defaultValue = 25.625)
    public static final String PROP_FRAME_SIZE_MS = "frameSizeInMs";

    /** The property  name for frame shift in milliseconds, which has a default value of 10F. */
    @S4Double(defaultValue = 10.0)
    public static final String PROP_FRAME_SHIFT_MS = "frameShiftInMs";

    /** The property  specifying the length of the cepstrum data. */
    @S4Integer(defaultValue = 13)
    public static final String PROP_CEPSTRUM_LENGTH = "cepstrumLength";

    /** The property specifying whether the input data is big-endian. */
    @S4Boolean(defaultValue = false)
    public static final String PROP_BIG_ENDIAN_DATA = "bigEndianData";

    /** The property that defines the sample rate */
    @S4Integer(defaultValue = 16000)
    public static final String PROP_SAMPLE_RATE = "sampleRate";

    private boolean binary;
    private ExtendedStreamTokenizer est; // for ASCII files
    private DataInputStream binaryStream; // for binary files
    private int numPoints;
    private int curPoint;
    private int cepstrumLength;
    private int frameShift;
    private int frameSize;
    private int sampleRate;
    private long firstSampleNumber;
    private boolean bigEndian;

    public StreamCepstrumSource( int cepstrumLength, boolean binary, float frameShiftMs, float frameSizeMs, int sampleRate ) {
	initLogger();
        this.cepstrumLength = cepstrumLength;
        this.binary = binary;
        this.sampleRate = sampleRate;
        this.frameShift = DataUtil.getSamplesPerWindow(sampleRate, frameShiftMs);
        this.frameSize = DataUtil.getSamplesPerShift(sampleRate, frameSizeMs);
    }

    public StreamCepstrumSource( ) {
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        cepstrumLength = ps.getInt(PROP_CEPSTRUM_LENGTH);
        binary = ps.getBoolean(PROP_BINARY);
        bigEndian = ps.getBoolean(PROP_BIG_ENDIAN_DATA);
        float frameShiftMs = ps.getFloat(PROP_FRAME_SHIFT_MS);
        float frameSizeMs = ps.getFloat(PROP_FRAME_SIZE_MS);
        sampleRate = ps.getInt(PROP_SAMPLE_RATE);
        frameShift = DataUtil.getSamplesPerWindow(sampleRate, frameShiftMs);
        frameSize = DataUtil.getSamplesPerShift(sampleRate, frameSizeMs);        
    }


    /** Constructs a StreamCepstrumSource that reads MelCepstrum data from the given path. */
    @Override
    public void initialize() {
        super.initialize();
        curPoint = -1;
        firstSampleNumber = 0;
        bigEndian = false;
    }


    /**
     * Sets the InputStream to read cepstral data from.
     *
     * @param is        the InputStream to read cepstral data from
     * @param bigEndian true if the InputStream data is in big-endian, false otherwise
     * @throws IOException if an I/O error occurs
     */
    public void setInputStream(InputStream is, boolean bigEndian)
            throws IOException {
        this.bigEndian = bigEndian;
        if (binary) {
            binaryStream = new DataInputStream(new BufferedInputStream(is));
            if (bigEndian) {
                numPoints = binaryStream.readInt();
                System.out.println("BigEndian");
            } else {
                numPoints = Utilities.readLittleEndianInt(binaryStream);
                System.out.println("LittleEndian");
            }
            System.out.println("Frames: " + numPoints / cepstrumLength);
        } else {
            est = new ExtendedStreamTokenizer(is, false);
            numPoints = est.getInt("num_frames");
            est.expectString("frames");
        }
        curPoint = -1;
        firstSampleNumber = 0;
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
                if (binary) {
                    binaryStream.close();
                } else {
                    est.close();
                }
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
                    if (binary) {
                        if (bigEndian) {
                            vectorData[i] = binaryStream.readFloat();
                        } else {
                            vectorData[i] = Utilities.readLittleEndianFloat(binaryStream);
                        }
                    } else {
                        vectorData[i] = est.getFloat("cepstrum data");
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

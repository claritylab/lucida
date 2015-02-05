/*
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2002-2004 Sun Microsystems, Inc.  
 * Portions Copyright 2002-2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.tools.audio;

import javax.sound.sampled.AudioFormat;
import java.io.IOException;
import java.io.OutputStream;


/** Writes raw audio to a file, handling sample size, endian format, and signed/unsigned data. */
public class RawWriter {

    private final OutputStream outputStream;
    private final int bytesPerSample;
    private boolean signedData;


    /**
     */
    public RawWriter(OutputStream outputStream,
                     AudioFormat audioFormat) {
        AudioFormat.Encoding encoding = audioFormat.getEncoding();
        this.outputStream = outputStream;
        this.bytesPerSample = audioFormat.getSampleSizeInBits() / 8;
        if (encoding == AudioFormat.Encoding.PCM_SIGNED) {
            this.signedData = true;
        } else if (encoding == AudioFormat.Encoding.PCM_UNSIGNED) {
            this.signedData = false;
        } else {
            System.err.println("Unsupported audio encoding: " + encoding);
            System.exit(-1);
        }
    }


    /**
     * Writes the sample to the output stream.
     *
     * @throws java.io.IOException
     */
    public void writeSample(int sample) throws IOException {
        /* First byte contains the byte that carries the sign.
         */
        if (signedData) {
            outputStream.write(sample >> ((bytesPerSample - 1) * 8));
        } else {
            outputStream.write((sample >> ((bytesPerSample - 1) * 8)) & 0xff);
        }

        /* Now just output the rest of the data in little endian form.
         */
        for (int i = bytesPerSample - 2; i >= 0; i--) {
            outputStream.write((sample >> (i * 8)) & 0xff);
        }
    }
}

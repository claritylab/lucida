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

import edu.cmu.sphinx.linguist.acoustic.tiedstate.Pool;
import edu.cmu.sphinx.util.StreamFactory;

import java.io.*;
import java.util.Properties;
import java.util.logging.Level;

/**
 * An acoustic model saver that saves sphinx3 ascii data.
 * <p/>
 * Mixture weights and transition probabilities are saved in linear scale.
 */
class Sphinx4Saver extends Sphinx3Saver {
	
    protected final static String TMAT_FILE_VERSION = "4.0";

    @Override
    protected void saveTransitionMatricesAscii(Pool<float[][]> pool, String path, boolean append)
            throws IOException {

        String location = super.getLocation();
        OutputStream outputStream = StreamFactory.getOutputStream(location, path, append);
        if (outputStream == null) {
            throw new IOException("Error trying to write file " + location + path);
        }
        PrintWriter pw = new PrintWriter(outputStream, true);

        logger.info("Saving transition matrices to: ");
        logger.info(path);
        int numMatrices = pool.size();

        assert numMatrices > 0;
        float[][] tmat = pool.get(0);

        pw.println("tmat " + numMatrices + " X");

        for (int i = 0; i < numMatrices; i++) {
            tmat = pool.get(i);
            int numStates = tmat[0].length;

            pw.println("tmat [" + i + ']');
            pw.println("nstate " + (numStates - 1));
            for (int j = 0; j < numStates; j++) {
                for (int k = 0; k < numStates; k++) {

                    // the last row is just zeros, so we just do
                    // the first (numStates - 1) rows

                    if (j < numStates - 1) {
                        if (sparseForm) {
                            if (k < j) {
                                pw.print("\t");
                            }
                            if (k == j || k == j + 1) {
                                pw.print((float)logMath.logToLinear(tmat[j][k]));
                            }
                        } else {
                            pw.print((float)logMath.logToLinear(tmat[j][k]));
                        }
                        if (numStates - 1 == k) {
                            pw.println();
                        } else {
                            pw.print(" ");
                        }

                    }

                    if (logger.isLoggable(Level.FINE)) {
                        logger.fine("tmat j " + j + " k " + k + " tm " + tmat[j][k]);
                    }
                }
            }
        }
        outputStream.close();
    }

    @Override
    protected void saveTransitionMatricesBinary(Pool<float[][]> pool, String path, boolean append)
            throws IOException {

        logger.info("Saving transition matrices to: ");
        logger.info(path);
        Properties props = new Properties();

        String strCheckSum = super.getCheckSum();
        boolean doCheckSum = super.getDoCheckSum();
        props.setProperty("version", TMAT_FILE_VERSION);
        if (doCheckSum) {
            props.setProperty("chksum0", strCheckSum);
        }

        String location = super.getLocation();

        DataOutputStream dos = writeS3BinaryHeader(location, path, props, append);

        int numMatrices = pool.size();
        assert numMatrices > 0;
        writeInt(dos, numMatrices);

        // Now we count number of states. Since each model can have an
        // arbitrary number of states, we have to visit all tmats, and
        // count the total number of states.
        int numStates = 0;
        int numValues = 0;
        for (int i = 0; i < numMatrices; i++) {
            float[][] tmat = pool.get(i);
            int nStates = tmat[0].length;
            numStates += nStates;
            // Number of elements in each transition matrix is the
            // number of row, i.e. number of emitting states, times
            // number of columns, i.e., number of emitting states plus
            // final non-emitting state.
            numValues += nStates * (nStates - 1);
        }
        writeInt(dos, numValues);

        for (int i = 0; i < numMatrices; i++) {
            float[][] tmat = pool.get(i);
            numStates = tmat[0].length;
            writeInt(dos, numStates);

            // Last row should be all zeroes
            float[] logTmatRow = tmat[numStates - 1];
            float[] tmatRow = new float[logTmatRow.length];

            for (int j = 0; j < numStates; j++) {
                assert tmatRow[j] == 0.0f;
            }

            for (int j = 0; j < numStates - 1; j++) {
                logTmatRow = tmat[j];
                tmatRow = new float[logTmatRow.length];
                logMath.logToLinear(logTmatRow, tmatRow);
                writeFloatArray(dos, tmatRow);
            }
        }
        if (doCheckSum) {
            assert doCheckSum = false : "Checksum not supported";
            // writeInt(dos, checkSum);
        }
        dos.close();
    }
}

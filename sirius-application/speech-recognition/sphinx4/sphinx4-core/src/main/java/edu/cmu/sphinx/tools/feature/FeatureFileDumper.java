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

package edu.cmu.sphinx.tools.feature;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.util.StreamDataSource;
import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.PropertyException;

import java.io.*;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.LinkedList;
import java.util.List;
import java.util.Scanner;
import java.util.logging.Logger;

/**
 * This program takes in an audio file, does frontend signal processing to it,
 * and then dumps the resulting Feature into a separate file. Also it can
 * process a list of files at once.
 * <p/>
 * Available options:
 * <ul>
 * <li>-config configFile - the XML configuration file</li>
 * <li>-name frontendName - the name of the feature extractor inside the
 * configuration file</li>
 * <li>-i audioFile - the name of the audio file</li>
 * <li>-ctl controlFile - the name of the input file for batch processing</li>
 * <li>-o outputFile - the name of the output file or output folder</li>
 * <li>-format binary/ascii - output file format</li>
 * </ul>
 */
public class FeatureFileDumper {

    private FrontEnd frontEnd;
    private StreamDataSource audioSource;
    private List<float[]> allFeatures;
    private int featureLength = -1;

    /** The logger for this class */
    private static final Logger logger = Logger
            .getLogger("edu.cmu.sphinx.tools.feature.FeatureFileDumper");

    /**
     * Constructs a FeatureFileDumper.
     * 
     * @param cm
     *            the configuration manager
     * @param frontEndName
     *            the name for the frontend
     */
    public FeatureFileDumper(ConfigurationManager cm, String frontEndName)
            throws IOException {
        try {
            frontEnd = (FrontEnd) cm.lookup(frontEndName);
            audioSource = (StreamDataSource) cm.lookup("streamDataSource");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Process the file and store the features
     * 
     * @param inputAudioFile
     *            the input audio file
     * @throws FileNotFoundException
     */
    public void processFile(String inputAudioFile) throws FileNotFoundException {
        audioSource .setInputStream(new FileInputStream(inputAudioFile));
        allFeatures = new LinkedList<float[]>();
        getAllFeatures();
        logger.info("Frames: " + allFeatures.size());
    }

    /**
     * Retrieve all Features from the frontend, and cache all those with actual
     * feature data.
     */
    private void getAllFeatures() {
        /*
         * Run through all the data and produce feature.
         */
        try {
            assert (allFeatures != null);
            Data feature = frontEnd.getData();
            while (!(feature instanceof DataEndSignal)) {
                if (feature instanceof DoubleData) {
                    double[] featureData = ((DoubleData) feature).getValues();
                    if (featureLength < 0) {
                        featureLength = featureData.length;
                        logger.info("Feature length: " + featureLength);
                    }
                    float[] convertedData = new float[featureData.length];
                    for (int i = 0; i < featureData.length; i++) {
                        convertedData[i] = (float) featureData[i];
                    }
                    allFeatures.add(convertedData);
                } else if (feature instanceof FloatData) {
                    float[] featureData = ((FloatData) feature).getValues();
                    if (featureLength < 0) {
                        featureLength = featureData.length;
                        logger.info("Feature length: " + featureLength);
                    }
                    allFeatures.add(featureData);
                }
                feature = frontEnd.getData();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Returns the total number of data points that should be written to the
     * output file.
     * 
     * @return the total number of data points that should be written
     */
    private int getNumberDataPoints() {
        return (allFeatures.size() * featureLength);
    }

    /**
     * Dumps the feature to the given binary output.
     * 
     * @param outputFile
     *            the binary output file
     */
    public void dumpBinary(String outputFile) throws IOException {
        DataOutputStream outStream = new DataOutputStream(new FileOutputStream(
                outputFile));
        outStream.writeInt(getNumberDataPoints());

        for (float[] feature : allFeatures) {
            for (float val : feature) {
                outStream.writeFloat(val);
            }
        }

        outStream.close();
    }

    /**
     * Dumps the feature to the given ASCII output file.
     * 
     * @param outputFile
     *            the ASCII output file
     */
    public void dumpAscii(String outputFile) throws IOException {
        PrintStream ps = new PrintStream(new FileOutputStream(outputFile), true);
        ps.print(getNumberDataPoints());
        ps.print(' ');

        for (float[] feature : allFeatures) {
            for (float val : feature) {
                ps.print(val);
                ps.print(' ');
            }
        }

        ps.close();
    }

    /**
     * Main program for this dumper.
     */
    public static void main(String[] argv) {

        String configFile = null;
        String frontEndName = null;
        String inputFile = null;
        String inputCtl = null;
        String outputFile = null;
        String format = "binary";

        for (int i = 0; i < argv.length; i++) {
            if (argv[i].equals("-c")) {
                configFile = argv[++i];
            }
            if (argv[i].equals("-name")) {
                frontEndName = argv[++i];
            }
            if (argv[i].equals("-i")) {
                inputFile = argv[++i];
            }
            if (argv[i].equals("-ctl")) {
                inputCtl = argv[++i];
            }
            if (argv[i].equals("-o")) {
                outputFile = argv[++i];
            }
            if (argv[i].equals("-format")) {
                format = argv[++i];
            }
        }

        if (frontEndName == null || (inputFile == null && inputCtl == null)
                || outputFile == null || format == null) {
            System.out
                    .println("Usage: FeatureFileDumper "
                            + "[ -config configFile ] -name frontendName "
                            + "< -i input File -o outputFile | -ctl inputFile -i inputFolder -o outputFolder >\n"
                            + "Possible frontends are: cepstraFrontEnd, spectraFrontEnd, plpFrontEnd");
            System.exit(1);
        }

        logger.info("Input file: " + inputFile);
        logger.info("Output file: " + outputFile);
        logger.info("Format: " + format);

        try {
            URL url;
            if (configFile != null) {
                url = new File(configFile).toURI().toURL();
            } else {
                url = FeatureFileDumper.class
                        .getResource("frontend.config.xml");
            }
            ConfigurationManager cm = new ConfigurationManager(url);
            
            if(cm.lookup(frontEndName) == null) {
            	throw new RuntimeException("No such frontend: " + frontEndName);
            }
            
            FeatureFileDumper dumper = new FeatureFileDumper(cm, frontEndName);

            if (inputCtl == null)
                dumper.processFile(inputFile, outputFile, format);
            else
                dumper.processCtl(inputCtl, inputFile, outputFile, format);
        } catch (IOException ioe) {
            System.err.println("I/O Error " + ioe);
        } catch (PropertyException p) {
            System.err.println("Bad configuration " + p);
        }
    }

    private void processFile(String inputFile, String outputFile, String format)
            throws MalformedURLException, IOException {
        processFile(inputFile);
        if (format.equals("binary")) {
            dumpBinary(outputFile);
        } else if (format.equals("ascii")) {
            dumpAscii(outputFile);
        } else {
            System.out.println("ERROR: unknown output format: " + format);
        }
    }

    private void processCtl(String inputCtl, String inputFolder,
            String outputFolder, String format) throws MalformedURLException,
            IOException {

        Scanner scanner = new Scanner(new File(inputCtl));
        while (scanner.hasNext()) {
            String fileName = scanner.next();
            String inputFile = inputFolder + "/" + fileName + ".wav";
            String outputFile = outputFolder + "/" + fileName + ".mfc";

            processFile(inputFile);
            if (format.equals("binary")) {
                dumpBinary(outputFile);
            } else if (format.equals("ascii")) {
                dumpAscii(outputFile);
            } else {
                System.out.println("ERROR: unknown output format: " + format);
            }
        }
        scanner.close();
    }
}

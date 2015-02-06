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

package edu.cmu.sphinx.tools.batch;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Scanner;

import edu.cmu.sphinx.frontend.util.StreamDataSource;
import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.util.props.ConfigurationManager;

/**
 * Batch decoder recognizer which can be used inside Sphinxtrain
 * to build lattices and generate decoding results.
 */
public class BatchDecoderRecognizer {

    int ctlOffset = -1;
    int ctlCount = 1000000;
    String config;
    String hmm;
    String ctl;
    String hyp;
    String featDir;
    
    ConfigurationManager manager;
    StreamDataSource source;
    Recognizer recognizer;
    PrintWriter writer;
    
    void parseArgs(String[] argv) {
        for (int i = 0; i < argv.length; i++) {
            if (argv[i].equals("-ctl")) {
                ctl = argv[++i];
            }
            if (argv[i].equals("-config")) {
                config = argv[++i];
            }
            if (argv[i].equals("-hmm")) {
                hmm = argv[++i];
            }
            if (argv[i].equals("-ctloffset")) {
                ctlOffset = Integer.parseInt(argv[++i]);
            }
            if (argv[i].equals("-ctlcount")) {
                ctlCount = Integer.parseInt(argv[++i]);
            }
            if (argv[i].equals("-hyp")) {
                hyp = argv[++i];
            }
            if (argv[i].equals("-feat")) {
                featDir = argv[++i];
            }        
        }        
    }
    
    void recognize() throws IOException {
        
        init();
        writer = new PrintWriter (new File(hyp), "UTF-8");
        Scanner scanner = new Scanner(new File(ctl));
        
        for (int i = 0; i < ctlOffset; i++) {
            if (scanner.hasNext())
                scanner.next();
        }
        
        for (int i = 0; i < ctlCount; i++) {
            if (scanner.hasNext()) {
                String utteranceId = scanner.next();
                String inputFile =  featDir + "/" + utteranceId + ".wav";
                processFile(utteranceId, inputFile);
            }
        }
        writer.close();
        scanner.close();
        recognizer.deallocate();
    }
    
    private void processFile(String utteranceId, String inputFile) throws IOException {
        FileInputStream stream = new FileInputStream(inputFile);
        source.setInputStream(stream);
        Result result = recognizer.recognize();
        writer.println (result.getBestFinalResultNoFiller() + " (" + utteranceId + ")");
    }

    public static void main(String[] argv) throws IOException {
        BatchDecoderRecognizer batchRecognizer = new BatchDecoderRecognizer();
        batchRecognizer.parseArgs(argv);
        batchRecognizer.recognize();
    }

    private void init() throws IOException {
        manager = new ConfigurationManager(config);
        recognizer = (Recognizer)manager.lookup("recognizer");
        source = (StreamDataSource)manager.lookup("streamDataSource");
        
        recognizer.allocate();        
    }
}

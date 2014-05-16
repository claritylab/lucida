/*
 * Copyright 1999-2013 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */

package edu.cmu.sphinx.demo.transcriber;

import java.io.InputStream;

import edu.cmu.sphinx.api.Configuration;
import edu.cmu.sphinx.api.SpeechResult;
import edu.cmu.sphinx.api.StreamSpeechRecognizer;
import edu.cmu.sphinx.result.WordResult;


/**
 * A simple example that shows how to transcribe a continuous audio file that
 * has multiple utterances in it.
 */
public class TranscriberDemo {

    public static void main(String[] args) throws Exception {
        System.out.println("Loading models...");

        Configuration configuration = new Configuration();

        // Load model from the jar
        configuration.setAcousticModelPath("resource:/edu/cmu/sphinx/models/acoustic/wsj");
        
        // You can also load model from folder
        // configuration.setAcousticModelPath("file:en-us");
        
        configuration.setDictionaryPath("resource:/edu/cmu/sphinx/models/acoustic/wsj/dict/cmudict.0.6d");
        configuration.setLanguageModelPath("resource:/edu/cmu/sphinx/models/language/en-us.lm.dmp");

        StreamSpeechRecognizer recognizer = 
            new StreamSpeechRecognizer(configuration);
        InputStream stream = TranscriberDemo.class.getResourceAsStream(
                "/edu/cmu/sphinx/demo/aligner/10001-90210-01803.wav");
        recognizer.startRecognition(stream);

        SpeechResult result;

        while ((result = recognizer.getResult()) != null) {
        
            System.out.format("Hypothesis: %s\n",
                              result.getHypothesis());
                              
            System.out.println("List of recognized words and their times:");
            for (WordResult r : result.getWords()) {
        	System.out.println(r);
            }

            System.out.println("Best 3 hypothesis:");            
            for (String s : result.getNbest(3))
                System.out.println(s);

            System.out.println("Lattice contains " + result.getLattice().getNodes().size() + " nodes");
        }

        recognizer.stopRecognition();
    }
}

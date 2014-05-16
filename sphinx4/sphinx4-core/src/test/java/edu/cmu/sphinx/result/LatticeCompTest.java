/*
 * Copyright 1999-2004 Carnegie Mellon University. Portions Copyright 2004 Sun
 * Microsystems, Inc. Portions Copyright 2004 Mitsubishi Electric Research
 * Laboratories. All Rights Reserved. Use is subject to license terms. See the
 * file "license.terms" for information on usage and redistribution of this
 * file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

package edu.cmu.sphinx.result;

import static edu.cmu.sphinx.util.props.ConfigurationManagerUtils.setProperty;
import static javax.sound.sampled.AudioSystem.getAudioInputStream;
import static org.testng.Assert.assertTrue;

import java.io.IOException;
import java.net.URL;

import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.UnsupportedAudioFileException;

import org.testng.annotations.Test;

import edu.cmu.sphinx.frontend.util.StreamDataSource;
import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.util.props.ConfigurationManager;


/**
 * Compares the lattices generated when the LexTreeLinguist flag
 * 'keepAllTokens' is turned on/off.
 */
public class LatticeCompTest {

    /**
     * Main method for running the LatticeCompTest demo.
     * 
     * @throws IOException
     * @throws UnsupportedAudioFileException
     */
    @Test
    public void testLatticeComp() throws UnsupportedAudioFileException,
            IOException {
        // TODO: make an integration test, too heavy to be a unit test
        URL audioFileURL = getClass().getResource("green.wav");
        URL configURL = getClass().getResource("config.xml");
        URL lm = getClass().getResource("hellongram.trigram.lm");

        ConfigurationManager cm = new ConfigurationManager(configURL);
        setProperty(cm, "trigramModel", "location", lm.toString());

        Recognizer recognizer = cm.lookup("recognizer");
        StreamDataSource dataSource = cm.lookup(StreamDataSource.class);

        AudioInputStream ais = getAudioInputStream(audioFileURL);
        dataSource.setInputStream(ais);

        recognizer.allocate();
        Lattice lattice = new Lattice(recognizer.recognize());

        cm = new ConfigurationManager(configURL);
        setProperty(cm, "keepAllTokens", "true");
        setProperty(cm, "trigramModel", "location", lm.toString());

        recognizer = cm.lookup("recognizer");
        recognizer.allocate();
        dataSource = cm.lookup(StreamDataSource.class);
        dataSource.setInputStream(getAudioInputStream(audioFileURL));
        Lattice allLattice = new Lattice(recognizer.recognize());

        assertTrue(lattice.isEquivalent(allLattice));
    }
}

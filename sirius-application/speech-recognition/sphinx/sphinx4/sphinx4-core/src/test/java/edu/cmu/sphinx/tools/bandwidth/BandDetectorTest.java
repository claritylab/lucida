/*
 * Copyright 1999-2013 Carnegie Mellon University. All Rights Reserved. Use is
 * subject to license terms. See the file "license.terms" for information on
 * usage and redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */

package edu.cmu.sphinx.tools.bandwidth;

import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertTrue;

import java.io.File;
import java.net.URISyntaxException;

import org.testng.annotations.Test;


public class BandDetectorTest {

    @Test
    public void test() throws URISyntaxException {
        BandDetector detector = new BandDetector();
        String path;
        path = new File(getClass().getResource("10001-90210-01803-8khz.wav").toURI()).getPath();
        assertTrue(detector.bandwidth(path));
        path = new File(getClass().getResource("10001-90210-01803.wav").toURI()).getPath();
        assertFalse(detector.bandwidth(path));
    }
}

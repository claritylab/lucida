/**
 * 
 * Copyright 1999-2012 Carnegie Mellon University. Portions Copyright 2002 Sun
 * Microsystems, Inc. Portions Copyright 2002 Mitsubishi Electric Research
 * Laboratories. All Rights Reserved. Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * 
 */

package edu.cmu.sphinx.fst;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.net.URL;

import org.testng.annotations.Test;

import edu.cmu.sphinx.fst.operations.RmEpsilon;
import edu.cmu.sphinx.fst.semiring.ProbabilitySemiring;


/**
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class RmEpsilonTest {

    @Test
    public void testRmEpsilon() throws NumberFormatException, IOException, ClassNotFoundException, URISyntaxException {
        URL url = getClass().getResource("algorithms/rmepsilon/A.fst.txt");
        File parent = new File(url.toURI()).getParentFile();

        String path = new File(parent, "A").getPath();
        Fst fst = Convert.importFst(path, new ProbabilitySemiring());
        path = new File(parent, "fstrmepsilon.fst.ser").getPath();
        Fst fstRmEps = Fst.loadModel(path);

        Fst rmEpsilon = RmEpsilon.get(fst);
        assertThat(fstRmEps, equalTo(rmEpsilon));
    }
}

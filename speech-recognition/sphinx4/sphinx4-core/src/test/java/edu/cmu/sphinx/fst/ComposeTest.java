/**
 * Copyright 1999-2012 Carnegie Mellon University. Portions Copyright 2002 Sun
 * Microsystems, Inc. Portions Copyright 2002 Mitsubishi Electric Research
 * Laboratories. All Rights Reserved. Use is subject to license terms. See the
 * file "license.terms" for information on usage and redistribution of this
 * file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

package edu.cmu.sphinx.fst;

import static edu.cmu.sphinx.fst.Convert.importFst;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.net.URL;

import org.testng.annotations.Test;

import edu.cmu.sphinx.fst.operations.Compose;
import edu.cmu.sphinx.fst.semiring.TropicalSemiring;


/**
 * Compose Testing for Examples provided by M. Mohri,
 * "Weighted Automata Algorithms", Handbook of Weighted Automata,
 * Springer-Verlag, 2009, pp. 213–254.
 * 
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class ComposeTest {

    @Test
    public void testCompose() throws NumberFormatException, IOException, ClassNotFoundException, URISyntaxException {
        String path = "algorithms/compose/fstcompose.fst.ser";
        URL url = getClass().getResource(path);                      
        File parent = new File(url.toURI()).getParentFile();

        path = new File(parent, "A").getPath();
        Fst fstA = importFst(path, new TropicalSemiring());
        path = new File(parent, "B").getPath();
        Fst fstB = importFst(path, new TropicalSemiring());
        path = new File(parent, "fstcompose.fst.ser").getPath();
        Fst composed = Fst.loadModel(path);

        Fst fstComposed = Compose.get(fstA, fstB, new TropicalSemiring());
        assertThat(composed, equalTo(fstComposed));
    }
}

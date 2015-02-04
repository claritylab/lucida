/**
 * Copyright 1999-2012 Carnegie Mellon University. Portions Copyright 2002 Sun
 * Microsystems, Inc. Portions Copyright 2002 Mitsubishi Electric Research
 * Laboratories. All Rights Reserved. Use is subject to license terms. See the
 * file "license.terms" for information on usage and redistribution of this
 * file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

package edu.cmu.sphinx.fst;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.net.URL;

import org.testng.annotations.Test;

import edu.cmu.sphinx.fst.semiring.TropicalSemiring;


/**
 * @author "John Salatas <jsalatas@users.sourceforge.net>"
 */
public class ImportTest  {

    @Test
    public void testConvert() throws NumberFormatException, IOException, ClassNotFoundException, URISyntaxException {
        URL url = getClass().getResource("openfst/basic.fst");
        String dir = new File(url.toURI()).getParent();
        
        String path = new File(dir, "basic").getPath();
        Fst fst1 = Convert.importFst(path, new TropicalSemiring());

        path = new File(dir, "basic.fst.ser").getPath();
        Fst fst2 = Fst.loadModel(path);

        assertThat(fst1, equalTo(fst2));
    }

}

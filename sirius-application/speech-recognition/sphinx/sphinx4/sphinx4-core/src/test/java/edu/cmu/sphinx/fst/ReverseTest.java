/**
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

import edu.cmu.sphinx.fst.operations.Reverse;
import edu.cmu.sphinx.fst.semiring.TropicalSemiring;


/**
 * @author John Salatas <jsalatas@users.sourceforge.net>
 */
public class ReverseTest {

    @Test
    public void testReverse() throws NumberFormatException, IOException, ClassNotFoundException, URISyntaxException {
        String path = "algorithms/reverse/A.fst";
        URL url = getClass().getResource(path);
        File parent = new File(url.toURI()).getParentFile();

        path = new File(parent, "A").getPath();
        Fst fst = Convert.importFst(path, new TropicalSemiring());
        path = new File(parent, "fstreverse.fst.ser").getPath();
        Fst fstB = Fst.loadModel(path);

        Fst fstReversed = Reverse.get(fst);
        assertThat(fstB, equalTo(fstReversed));
    }

}

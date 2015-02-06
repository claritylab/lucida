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

import edu.cmu.sphinx.fst.operations.Determinize;
import edu.cmu.sphinx.fst.semiring.TropicalSemiring;


/**
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class DeterminizeTest {

    @Test
    public void testDeterminize() throws NumberFormatException, IOException, ClassNotFoundException, URISyntaxException {
        String path = "algorithms/determinize/fstdeterminize.fst.ser";
        URL url = getClass().getResource(path);
        File parent = new File(url.toURI()).getParentFile();

        path = new File(parent, "A").getPath();
        Fst fstA = Convert.importFst(path, new TropicalSemiring());
        path = new File(parent, "fstdeterminize.fst.ser").getPath();
        Fst determinized = Fst.loadModel(path);

        Fst fstDeterminized = Determinize.get(fstA);
        assertThat(determinized, equalTo(fstDeterminized));
    }
}

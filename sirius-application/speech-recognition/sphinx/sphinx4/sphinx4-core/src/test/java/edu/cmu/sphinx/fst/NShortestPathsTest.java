/**
 * 
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

import edu.cmu.sphinx.fst.operations.NShortestPaths;
import edu.cmu.sphinx.fst.semiring.TropicalSemiring;

/**
 * @author John Salatas <jsalatas@users.sourceforge.net>
 * 
 */
public class NShortestPathsTest {

    @Test
    public void testNShortestPaths() throws NumberFormatException, IOException, URISyntaxException {
        String path = "algorithms/shortestpath/A.fst";
        URL url = getClass().getResource(path);
        File parent = new File(url.toURI()).getParentFile();
        
        path = new File(parent, "A").getPath();
        Fst fst = importFst(path, new TropicalSemiring());
        path = new File(parent, "nsp").getPath();
        Fst nsp = importFst(path, new TropicalSemiring());

        Fst fstNsp = NShortestPaths.get(fst, 6, true);
        assertThat(nsp, equalTo(fstNsp));
    }
}

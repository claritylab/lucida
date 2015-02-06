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

package edu.cmu.sphinx.linguist.acoustic.tiedstate.test;

import static java.lang.Math.PI;
import static java.lang.Math.exp;
import static java.lang.Math.sqrt;

import org.testng.Assert;
import org.testng.annotations.BeforeSuite;
import org.testng.annotations.Test;

import edu.cmu.sphinx.frontend.FloatData;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.MixtureComponent;
import edu.cmu.sphinx.util.LogMath;

/**
 * Some tests which test the functionality of <code>MixtureComponentt</code>s.
 * <p/>
 * Currently testing is restricted to univariate gaussians. It should be extended to test highdimensional gaussians as
 * well.
 */
public class MixtureComponentTest {

    @BeforeSuite
    public void setupLogMath() {
        LogMath.setUseTable(true);
    }

    /**
     * Compute the density values of a sampled interval with an univariate <code>MixtureComponent</code> and compare
     * values with the precomputed-computed ones.
     */
    @Test
    public void testUnivariateDensity() {

        float minX = 10;
        float maxX = 30;
        float resolution = 0.1f;

        float mean = 20;
        float var = 3;

        MixtureComponent gaussian = new MixtureComponent(new float[]{mean}, new float[]{var});

        for (float curX = minX; curX <= maxX; curX += resolution) {
            double gauLogScore = gaussian.getScore(new FloatData(new float[]{curX}, 16000, 0));

            double manualScore = (1 / sqrt(var * 2 * PI)) * exp((-0.5 / var) * (curX - mean) * (curX - mean));
            double gauScore = LogMath.getInstance().logToLinear((float) gauLogScore);

            Assert.assertEquals(manualScore, gauScore, 1E-5);
        }
    }


    /** Tests whether working with different types transformations works properly. */
    @Test
    public void testUnivariateMeanTransformation() {
        float mean = 20;
        float var = 0.001f;

        MixtureComponent gaussian = new MixtureComponent(new float[]{mean}, new float[][]{{2}}, new float[]{5}, new float[]{var}, null, null);
        Assert.assertTrue(LogMath.getInstance().logToLinear(gaussian.getScore(new float[]{2 * mean + 5})) > 10);
    }


    /** Tests whether a <code>MixtureComponent</code>s can be cloned (using deep copying). */
    @Test
    public void testClone() throws CloneNotSupportedException {
        MixtureComponent gaussian = new MixtureComponent(new float[]{2}, new float[][]{{3}}, new float[]{4}, new float[]{5}, new float[][]{{6}}, new float[]{7});

        MixtureComponent clonedGaussian = gaussian.clone();

        Assert.assertTrue(!clonedGaussian.equals(gaussian));

        Assert.assertTrue(gaussian.getMean() != clonedGaussian.getMean());
        Assert.assertTrue(gaussian.getVariance() != clonedGaussian.getVariance());
        Assert.assertTrue(gaussian.getScore(new float[]{2}) == clonedGaussian.getScore(new float[]{2}));
    }
}

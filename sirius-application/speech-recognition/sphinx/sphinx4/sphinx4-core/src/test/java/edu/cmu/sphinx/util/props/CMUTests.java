package edu.cmu.sphinx.util.props;

import java.io.File;
import java.io.IOException;

import org.testng.Assert;
import org.testng.annotations.Test;

import edu.cmu.sphinx.frontend.DataProcessor;
import edu.cmu.sphinx.frontend.DoubleData;
import edu.cmu.sphinx.frontend.FrontEnd;
import edu.cmu.sphinx.instrumentation.AccuracyTracker;
import edu.cmu.sphinx.instrumentation.BestPathAccuracyTracker;
import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.ConfigurationManagerUtils;

/** Some unit-tests for the ConfigurationManagerUtils. */
public class CMUTests {

    @Test
    public void testClassTesting() {
        Assert.assertTrue(ConfigurationManagerUtils.isImplementingInterface(FrontEnd.class, DataProcessor.class));
        Assert.assertTrue(ConfigurationManagerUtils.isImplementingInterface(DataProcessor.class, Configurable.class));
        Assert.assertFalse(ConfigurationManagerUtils.isImplementingInterface(Configurable.class, Configurable.class));

        Assert.assertFalse(ConfigurationManagerUtils.isSubClass(Configurable.class, Configurable.class));
        Assert.assertTrue(ConfigurationManagerUtils.isSubClass(Integer.class, Object.class));
        Assert.assertFalse(ConfigurationManagerUtils.isSubClass(Object.class, Object.class));

        Assert.assertTrue(ConfigurationManagerUtils.isSubClass(BestPathAccuracyTracker.class, AccuracyTracker.class));

        Assert.assertTrue(ConfigurationManagerUtils.isDerivedClass(BestPathAccuracyTracker.class, AccuracyTracker.class));
        Assert.assertTrue(ConfigurationManagerUtils.isDerivedClass(BestPathAccuracyTracker.class, BestPathAccuracyTracker.class));
        Assert.assertTrue(!ConfigurationManagerUtils.isDerivedClass(BestPathAccuracyTracker.class, DoubleData.class));
    }


    @Test
    public void setComponentPropertyTest() throws IOException {
        File configFile = new File("src/test/edu/cmu/sphinx/util/props/test/ConfigurationManagerTest.testconfig.sxl");
        ConfigurationManager cm = new ConfigurationManager(configFile.toURI().toURL());

        int newBeamWidth = 4711;
        ConfigurationManagerUtils.setProperty(cm, "beamWidth", String.valueOf(newBeamWidth));

        DummyComp dummyComp = (DummyComp) cm.lookup("duco");
        Assert.assertEquals(newBeamWidth, dummyComp.getBeamWidth());
    }
}

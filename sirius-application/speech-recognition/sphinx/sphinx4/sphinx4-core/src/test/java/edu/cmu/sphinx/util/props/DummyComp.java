package edu.cmu.sphinx.util.props;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Logger;

import org.testng.Assert;
import org.testng.annotations.Test;

import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Boolean;
import edu.cmu.sphinx.util.props.S4Component;
import edu.cmu.sphinx.util.props.S4Double;
import edu.cmu.sphinx.util.props.S4Integer;
import edu.cmu.sphinx.util.props.S4String;


public class DummyComp implements Configurable {

    /** doc of beamWidth. */
    @S4Integer(defaultValue = 4)
    public static final String PROP_BEAM_WIDTH = "beamWidth";

    @S4String(defaultValue = "salami&cheese")
    public static final String PROP_BEST_PIZZA = "bestPizza";

    @S4Boolean(defaultValue = true)
    public static final String PROP_USE_FOOBAR = "useFooBar";

    @S4Boolean(defaultValue = true)
    public static final String PROP_USE_FOOBAZ = "useFooBaz";

    /** doc of frontend. */
    @S4Component(type = DummyFrontEnd.class, defaultClass = AnotherDummyFrontEnd.class)
    public static final String PROP_FRONTEND = "frontend";

    @S4Double(defaultValue = 1.3, range = {-1, 15})
    public static final String PROP_ALPHA = "alpha";

    /** doc of the string. */
    @S4String(defaultValue = "sphinx4", range = {"sphinx4", "htk"})
    public static final String PROP_BEST_ASR = "bestAsrSystem";


    private int beamWidth;
    private DummyFrontEnd frontEnd;
    private String bestAsr;
    private double alpha;
    private boolean useFooBaz;

    private Logger logger;


    public int getBeamWidth() {
        return beamWidth;
    }


    public DummyFrontEnd getFrontEnd() {
        return frontEnd;
    }


    public String getBestASR() {
        return bestAsr;
    }


    public double getAlpha() {
        return alpha;
    }


    public Logger getLogger() {
        return logger;
    }


    public void newProperties(PropertySheet ps) throws PropertyException {
        frontEnd = (DummyFrontEnd) ps.getComponent(PROP_FRONTEND);
        beamWidth = ps.getInt(PROP_BEAM_WIDTH);
        bestAsr = ps.getString(PROP_BEST_ASR);
        alpha = ps.getDouble(PROP_ALPHA);
        useFooBaz = ps.getBoolean(PROP_USE_FOOBAZ);

        logger = ps.getLogger();
    }


    public String getName() {
        return "lalala";
    }


    @Test
    public void testGetDefaultInstance() throws PropertyException, InstantiationException {
        DummyComp dc = ConfigurationManager.getInstance(DummyComp.class);

        Assert.assertEquals(4, dc.getBeamWidth());
        Assert.assertEquals(1.3, dc.getAlpha(), 1E-10);
        Assert.assertEquals (false, useFooBaz);

        DummyFrontEnd fe = dc.getFrontEnd();
        Assert.assertTrue(fe != null);
        Assert.assertTrue(fe instanceof AnotherDummyFrontEnd);
        Assert.assertTrue(fe.getDataProcs().size() == 3);
        Assert.assertTrue(fe.getDataProcs().get(0) instanceof DummyProcessor);
        Assert.assertTrue(fe.getDataProcs().get(1) instanceof AnotherDummyProcessor);
        Assert.assertTrue(fe.getDataProcs().get(2) instanceof DummyProcessor);

        Assert.assertTrue(dc.getBestASR().equals("sphinx4"));
        Assert.assertTrue(dc.getLogger() != null);
    }


    /** Use the all defaults defined by the annotations to instantiate a Configurable. */
    @Test
    public void testCustomizedDefaultInstance() throws PropertyException, InstantiationException {
        Map<String, Object> defaultProps = new HashMap<String, Object>();
        defaultProps.put(DummyComp.PROP_FRONTEND, new DummyFrontEnd());

        DummyComp dc = ConfigurationManager.getInstance(DummyComp.class, defaultProps);

        Assert.assertEquals(4, dc.getBeamWidth());
        Assert.assertEquals(1.3, dc.getAlpha(), 1E-10);
        Assert.assertTrue(dc.getFrontEnd() != null);
        Assert.assertTrue(dc.getBestASR().equals("sphinx4"));
        Assert.assertTrue(dc.getLogger() != null);
    }


    @Test
    public void testUseXmlConfig() throws IOException, PropertyException, InstantiationException {
        // probably you need to adpat this path. testconfig is located in the same folder as test
        File configFile = new File("src/test/edu/cmu/sphinx/util/props/test/ConfigurationManagerTest.testconfig.sxl");
        if (!configFile.exists())
            Assert.fail("can not find configuration file to be used for test");

        ConfigurationManager cm = new ConfigurationManager(configFile.toURI().toURL());

        DummyComp dc = (DummyComp) cm.lookup("duco");

        Assert.assertEquals(dc.getBeamWidth(), 123);
        Assert.assertEquals(9.99, dc.getAlpha(), 1E-10);

        Assert.assertTrue(dc.getFrontEnd() != null);
        Assert.assertTrue(dc.getFrontEnd().isUseMfccs());
        Assert.assertTrue(dc.getFrontEnd().getDataProcs().size() == 2);

        Assert.assertTrue(dc.getBestASR().equals("sphinx4"));
        Assert.assertTrue(dc.getLogger() != null);
    }
}



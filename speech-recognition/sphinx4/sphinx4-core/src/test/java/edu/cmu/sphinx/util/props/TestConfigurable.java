package edu.cmu.sphinx.util.props;

import java.util.HashMap;
import java.util.Map;

import org.testng.Assert;
import org.testng.annotations.Test;

import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.RawPropertyData;
import edu.cmu.sphinx.util.props.S4Component;
import edu.cmu.sphinx.util.props.S4Double;
import edu.cmu.sphinx.util.props.S4String;

/**
 * DOCUMENT ME!
 *
 * @author Holger Brandl
 */
public class TestConfigurable implements Configurable {

    // note: no default component here
    @S4Component(type = AnotherDummyProcessor.class)
    public static final String PROP_DATA_PROC = "dataProc";
    private DummyProcessor dataProc;

    @S4String(mandatory = false)
    public static final String PROP_ASTRING = "mystring";
    private String myString;

    @S4Double(defaultValue = 1.3)
    public static final String PROP_GAMMA = "gamma";
    private double gamma;


    public void newProperties(PropertySheet ps) throws PropertyException {
        dataProc = (DummyProcessor) ps.getComponent(PROP_DATA_PROC);
        myString = ps.getString(PROP_ASTRING);
        gamma = ps.getDouble(PROP_GAMMA);
    }


    public String getName() {
        return this.getClass().getName();
    }


    public double getGamma() {
        return gamma;
    }


    public DummyProcessor getDataProc() {
        return dataProc;
    }


    @Test
    // note: it is not a bug but a feature of this test to print a stacktrace
    public void testDynamicConfCreationWithoutDefaultProperty() {
        try {
            ConfigurationManager cm = new ConfigurationManager();

            String instanceName = "testconf";
            cm.addConfigurable(TestConfigurable.class, instanceName);

            cm.lookup(instanceName);
            Assert.fail("add didn't fail without given default frontend");
        } catch (NullPointerException e) {
        } catch (PropertyException e) {
        }
    }


    @Test
    public void testNullStringProperty() throws PropertyException, InstantiationException {
        HashMap<String, Object> props = new HashMap<String, Object>();
        props.put("dataProc", new AnotherDummyProcessor());

        TestConfigurable teco = ConfigurationManager.getInstance(TestConfigurable.class, props);
        Assert.assertTrue(teco.myString == null);
    }


    @Test
    public void testPropSheetFromConfigurableInstance() throws PropertyException, InstantiationException {
        String testString = "test";

        Map<String, Object> props = new HashMap<String, Object>();
        props.put(PROP_ASTRING, testString);
        props.put(PROP_DATA_PROC, new DummyProcessor());
        TestConfigurable tc = ConfigurationManager.getInstance(TestConfigurable.class, props);

        // now create a property sheet in order to modify the configurable
        PropertySheet propSheet = new PropertySheet(tc, null, new RawPropertyData("tt", tc.getClass().getName()), new ConfigurationManager());
        propSheet.setComponent(PROP_DATA_PROC, "tt", new AnotherDummyProcessor());
        tc.newProperties(propSheet);

        // test whether old props were preserved and new ones were applied

        // FIXME: Its by design not possible to preserve the old properties without have a CM
        // probably we should remove the possibility to let the user create PropertySheet instances.
        // Assert.assertTrue(tc.myString.equals(testString));
        // Assert.assertTrue(tc.gamma == testDouble);
        Assert.assertTrue(tc.dataProc != null && tc.dataProc instanceof AnotherDummyProcessor);
    }
}

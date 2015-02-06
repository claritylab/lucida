package edu.cmu.sphinx.util.props;

import static java.lang.Double.MIN_VALUE;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;
import static org.testng.Assert.assertEquals;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import org.hamcrest.Matchers;
import org.testng.annotations.Test;


/**
 * Some unit tests, which ensure a proper implementation of configuration
 * management.
 * 
 * @author Holger Brandl
 */

public class ConfigurationManagerTest {

    @Test
    public void testDynamicConfCreation() throws PropertyException,
            InstantiationException {
        ConfigurationManager cm = new ConfigurationManager();

        String instanceName = "docu";
        Map<String, Object> props = new HashMap<String, Object>();
        props.put(DummyComp.PROP_FRONTEND, new DummyFrontEnd());
        cm.addConfigurable(DummyComp.class, instanceName, props);

        assertThat(cm.getPropertySheet(instanceName), notNullValue());
        assertThat(cm.lookup(instanceName), notNullValue());
        assertThat(cm.lookup(instanceName), instanceOf(DummyComp.class));
    }

    @Test
    public void testSerialization() throws IOException, PropertyException {
        URL url = getClass()
                .getResource("ConfigurationManagerTest.testconfig.sxl");
        ConfigurationManager cm = new ConfigurationManager(url);

        File tmpFile = File.createTempFile("ConfigurationManager", ".tmp.sxl");
        tmpFile.deleteOnExit();
        ConfigurationManagerUtils.save(cm, tmpFile);

        // Now reload it.
        url = tmpFile.toURI().toURL();
        ConfigurationManager cmReloaded = new ConfigurationManager(url);
        assertThat(cmReloaded, equalTo(cm));
    }

    @Test
    public void testDynamicConfiguruationChange() throws IOException,
            PropertyException, InstantiationException {
        URL url = getClass()
                .getResource("ConfigurationManagerTest.testconfig.sxl");
        ConfigurationManager cm = new ConfigurationManager(url);

        assertThat(cm.getInstanceNames(DummyFrontEndProcessor.class), empty());

        PropertySheet propSheet = cm.getPropertySheet("duco");
        propSheet.setDouble("alpha", 11);
        DummyComp duco = cm.lookup("duco");

        assertThat(cm.getInstanceNames(DummyFrontEndProcessor.class),
                   hasSize(1));

        // IMPORTANT because we assume the configurable to be instantiated
        // first at lookup there is no need to call newProperties here
        // duco.newProperties(propSheet);
        assertThat(duco.getAlpha(), closeTo(11, MIN_VALUE));
    }

    @Test
    public void testSerializeDynamicConfiguration() throws PropertyException,
            InstantiationException {
        ConfigurationManager cm = new ConfigurationManager();
        String frontEndName = "myFrontEnd";

        cm.addConfigurable(DummyFrontEnd.class, frontEndName);
        PropertySheet propSheet = cm.getPropertySheet(frontEndName);
        propSheet
                .setComponentList("dataProcs", Arrays.asList("fooBar"), Arrays
                        .<Configurable> asList(new AnotherDummyProcessor()));

        String xmlString = ConfigurationManagerUtils.toXML(cm);

        assertThat(xmlString, containsString(frontEndName));
        assertThat(xmlString, containsString("fooBar"));

        DummyFrontEnd frontEnd = (DummyFrontEnd) cm.lookup(frontEndName);
        assertThat(frontEnd.getDataProcs(), hasSize(1));
        assertThat(frontEnd.getDataProcs().get(0),
                   instanceOf(AnotherDummyProcessor.class));
    }

    @Test
    public void testXmlExtendedConfiguration() {
        URL url = getClass().getResource("ConfigurationManagerTest.sxl");
        ConfigurationManager cm = new ConfigurationManager(url);

        String instanceName = "duco";
        assertThat(cm.getPropertySheet(instanceName), notNullValue());
        assertThat(cm.lookup(instanceName), notNullValue());
        assertThat(cm.lookup(instanceName), instanceOf(DummyComp.class));

        DummyComp docu = (DummyComp) cm.lookup(instanceName);

        // Test the parameters were successfully overridden.
        assertThat(docu.getFrontEnd().getDataProcs(), Matchers.empty());
        assertThat(docu.getBeamWidth(), equalTo(4711));

        // Test the the non-overridden properties of the parent-configuration
        // were preserved.
        assertThat(cm.lookup("processor"), notNullValue());
        // Test the global properties:
        assertThat(cm.getGlobalProperty("myalpha"), equalTo("-5"));
        assertThat(cm.getGlobalProperty("hiddenproductad"),
                   equalTo("opencards"));
    }

    @Test
    public void testGetComponentClass() {
        URL url = getClass().getResource("ConfigurationManagerTest.sxl");
        ConfigurationManager cm = new ConfigurationManager(url);

        String instanceName = "duco";
        PropertySheet ps = cm.getPropertySheet(instanceName);
        assertEquals(ps.getComponentClass("frontend"), DummyFrontEnd.class);
        assertEquals(ps.getComponentClass("anotherFrontend"),
                     DummyFrontEnd.class);
    }
}

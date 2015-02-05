package edu.cmu.sphinx.util.props;

import org.testng.Assert;
import org.testng.annotations.Test;

import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

/**
 * DOCUMENT ME!
 *
 * @author Holger Brandl
 */
public class ComponentPropertyTest implements Configurable {

    @S4Component(type = DummyProcessor.class, defaultClass = AnotherDummyProcessor.class)
    public static final String PROP_DATA_PROC = "dataProc";
    private DummyProcessor dataProc;


    public void newProperties(PropertySheet ps) throws PropertyException {
        dataProc = (DummyProcessor) ps.getComponent(PROP_DATA_PROC);
    }


    public String getName() {
        return this.getClass().getName();
    }


    @Test
    public void testDefInstance() throws PropertyException, InstantiationException {
        ComponentPropertyTest cpt = ConfigurationManager.getInstance(ComponentPropertyTest.class);

        Assert.assertTrue(cpt != null);
        Assert.assertTrue(cpt.dataProc instanceof AnotherDummyProcessor);
    }
}

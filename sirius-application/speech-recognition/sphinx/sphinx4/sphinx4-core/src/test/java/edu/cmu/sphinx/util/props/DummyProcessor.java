package edu.cmu.sphinx.util.props;

import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;

/**
 * DOCUMENT ME!
 *
 * @author Holger Brandl
 */
public class DummyProcessor implements DummyFrontEndProcessor {


    public void newProperties(PropertySheet ps) throws PropertyException {
    }


    public String getName() {
        return this.getClass().getName();
    }
}

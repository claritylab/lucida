package edu.cmu.sphinx.util.props;

import java.util.logging.Logger;

/**
 * An default (abstract) implementation of a configurable that implements a meaning {@code toString()} and keeps a
 * references to the {@code Confurable}'s logger.
 *
 * @author Holger Brandl
 */
public abstract class ConfigurableAdapter implements Configurable{

    private String name;
    protected Logger logger;

    public ConfigurableAdapter() {
    }

    protected void initLogger() {
        this.name = getClass().getSimpleName();
        init( name , Logger.getLogger( name ) );
    }

    public void newProperties(PropertySheet ps) throws PropertyException {
        init( ps.getInstanceName(), ps.getLogger());
    }

    private void init(String name, Logger logger) {
        this.name = name;
        this.logger = logger;
    }

    /** Returns the configuration name this {@code Configurable}. */
    public String getName() {
        // fix null names
        return name != null ? name : getClass().getSimpleName();
    }


    /**
     * Returns the name of this BaseDataProcessor.
     *
     * @return the name of this BaseDataProcessor
     */
    @Override
    public String toString() {
        return getName();
    }
}

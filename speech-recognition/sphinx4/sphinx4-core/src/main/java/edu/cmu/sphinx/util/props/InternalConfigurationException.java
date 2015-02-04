package edu.cmu.sphinx.util.props;

/**
 * Indicates that a problem occurred while setting one or more properties for this component. This includes errors as
 * improper type for component(-lists) properties, out-of-range-problems for double-, int- and ranged string-properties,
 * instantiation errors and undefined mandatory properties.
 * <p/>
 * This exception is instantiable only by the configuration management classes itself. In order to indicate problems
 * within Configurable.newProperties which are not coped by types or ranges (eg file-not-found, complex configuration
 * logic problems, etc.) <code>PropertyException</code> (which superclasses this class) can be used.
 * <p/>
 * The intention of the class is to make a clear distinction between core configuration errors and high level user
 * specific problems.
 *
 * @author Holger Brandl
 */
@SuppressWarnings("serial")
public class InternalConfigurationException extends PropertyException {

    InternalConfigurationException(String instanceName, String propertyName, String msg) {
        super(instanceName, propertyName, msg);
    }


    InternalConfigurationException(Throwable cause, String instanceName, String propertyName, String msg) {
        super(cause, instanceName, propertyName, msg);
    }
}

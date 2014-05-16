/*
 * 
 * Copyright 1999-2004 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.util.props;

/** Indicates that a problem occurred while setting one or more properties for this component */
@SuppressWarnings("serial")
public class PropertyException extends RuntimeException {

    private String instanceName;
    private String propertyName;


    /**
     * Creates a new property exception.
     *
     * @param instanceName The component this exception is related to.  (or <code>null</code> if unknown)
     * @param propertyName The name of the component-property which the problem is related. (or <code>null</code> if
     *                     unknown)
     * @param msg          a description of the problem.
     */
    public PropertyException(String instanceName, String propertyName, String msg) {
        this(null, instanceName, propertyName, msg);
    }


    /**
     * Creates a new property exception.
     *
     * @param cause        The cause of exception. (or <code>null</code> if unknown)
     * @param instanceName The component this exception is related to.  (or <code>null</code> if unknown)
     * @param propertyName The name of the component-property which the problem is related. (or <code>null</code> if
     *                     unknown)
     * @param msg          a description of the problem.
     */
    public PropertyException(Throwable cause, String instanceName, String propertyName, String msg) {
        super(msg, cause);

        this.instanceName = instanceName;
        this.propertyName = propertyName;
    }


    public PropertyException(Exception e) {
        super(e);
    }


    /**
     * Retrieves the name of the offending property
     *
     * @return the name of the offending property
     */
    public String getProperty() {
        return propertyName;
    }


    /**
     * Returns a string representation of this object
     *
     * @return the string representation of the object.
     */
    @Override
    public String toString() {
        return "Property exception component:'" + instanceName + "' property:'" + propertyName + "' - " + getMessage() + '\n'
                + super.toString();
    }
}

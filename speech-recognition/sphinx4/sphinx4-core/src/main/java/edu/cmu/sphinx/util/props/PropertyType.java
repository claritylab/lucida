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

import java.util.List;
import java.net.URL;

/** An enum type that defines the possible property types. */
public enum PropertyType {

    INT("int") {
        @Override protected boolean validateString(String obj) throws Exception {
            Integer.parseInt(obj);
            return true;
        }
    },
    BOOLEAN("boolean") {
        @Override protected boolean validateString(String obj) throws Exception {
            obj = obj.toLowerCase();
            return "true".equals(obj) || "false".equals(obj);
        }
    },
    FLOAT("float") {
        @Override protected boolean validateString(String obj) throws Exception {
            Float.parseFloat(obj);
            return true;
        }
    },
    DOUBLE("double") {
        @Override protected boolean validateString(String obj) throws Exception {
            Double.parseDouble(obj);
            return true;
        }
    },
    COMPONENT("Component", String.class),
    COMPONENT_LIST("ComponentList", List.class),
    STRING("String", String.class),
    /**
     * A Resource type. Resources are in one of the following forms:
     *
     * <ul>
     * <li> a URL such as http://www.cmu.edu/foo.zip
     * <li> a simple file location (e.g.  /lab/speech/data/wsj.jar)
     * <li> a resource in a jar file in the form:
     * resource:/FullyQualifiedClassName!resourceName
     * </ul>
     */
    RESOURCE("Resource") {
        @Override
        public boolean validateString(String obj) throws Exception {
            // First see if it is a resource
            if (obj.toLowerCase().startsWith("resource:/"))
                return true;

            // if it doesn't have a protocol spec add a "file:" to it, to make it a URL
            if (obj.indexOf(':') == -1)
                obj = "file:" + obj;

            // Check to see if it is a URL
            new URL(obj);
            return true;
        }
    },
    STRING_LIST("StringList", List.class);

    

    /** Display name of this PropertyType. */
    private final String displayName;

    /**
     * Calls to {@link #isValid} will check if an object is instance of the class.
     * If null, {@link #validateString} will be used to validate the object. */
    private Class<?> checkClass;

    /**
     * Creates type of the property 
     * @param displayName  name of the property to output
     */
    private PropertyType(String displayName) {
        this.displayName = displayName;
    }

    /**
     * Creates type of the property 
     * @param displayName  name of the property to output
     * @param checkClass   checked class
     */
    private PropertyType(String displayName, Class<?> checkClass) {
        this.displayName = displayName;
        this.checkClass = checkClass;
    }

    @Override
    public String toString() {
        return displayName;
    }

    /**
     * Validates the given String.<br>
     * Should be overridden if there exists a value of obj which is invalid.
     *
     * @param obj String to validate
     * @return true if obj is valid, false otherwise
     * @throws Exception if obj is not valid
     */
    protected boolean validateString(String obj) throws Exception {
        return true; // default implementation
    }

    /**
     * Determines if the given object can be converted to this type.
     * 
     * @param obj the object to verify
     * @return true if the object can be converted to an object of this type.
     */
    public boolean isValid(Object obj) {
        if (checkClass != null)
            return checkClass.isInstance(obj);
        if (obj instanceof String) {
            try {
                return validateString((String)obj);
            } catch (Exception e) {
                return false;
            }
        }
        return false;
    }
}

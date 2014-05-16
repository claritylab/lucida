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

import java.util.HashMap;
import java.util.List;
import java.util.Map;


/** Holds the raw property data just as it has come in from the properties file. */
public class RawPropertyData {

    private String name;
    private String className;
    private Map<String, Object> properties;
    
    /**
     * Creates a raw property data item.
     *
     * @param name the name of the item
     * @param className the class name of the item
     */
    public RawPropertyData(String name, String className) {
        this(name, className, new HashMap<String, Object>());
    }

    /**
     * Creates a raw property data item, using a given property map.
     *
     * @param name the name of the item
     * @param className the class name of the item
     * @param properties existing property map to use
     */
    public RawPropertyData(String name, String className, Map<String, Object> properties) {
        this.name = name;
        this.className = className;
        this.properties = properties;
    }

    /**
     * Adds a new property with a {@code String} value.
     *
     * @param propName the name of the property
     * @param propValue the value of the property
     */
    public void add(String propName, String propValue) {
        properties.put(propName, propValue);
    }

    /**
     * Adds a new property with a {@code List<String>} value.
     *
     * @param propName the name of the property
     * @param propValue the value of the property
     */
    public void add(String propName, List<String> propValue) {
        properties.put(propName, propValue);
    }

    /**
     * Removes an existing property.
     *
     * @param propName the name of the property
     */
    public void remove(String propName) {
        properties.remove(propName);
    }

    /** Returns the className. */
    public String getClassName() {
        return className;
    }

    /** @return Returns the name. */
    public String getName() {
        return name;
    }

    /** @return Returns the properties. */
    public Map<String, Object> getProperties() {
        return properties;
    }

    /**
     * Determines if the map already contains an entry for a property.
     *
     * @param propName the property of interest
     * @return true if the map already contains this property
     */
    public boolean contains(String propName) {
        return properties.get(propName) != null;
    }

    /** Returns a copy of this property data instance with all ${}-fields resolved. */
    public RawPropertyData flatten(ConfigurationManager cm) {
        RawPropertyData copyRPD = new RawPropertyData(name, className);

        for (Map.Entry<String, Object> entry : properties.entrySet()) {
            Object propVal = entry.getValue();
            if (propVal instanceof String) {
                if (((String) propVal).startsWith("${"))
                    propVal = cm.getGloPropReference(ConfigurationManagerUtils.stripGlobalSymbol((String) propVal));
            }

            copyRPD.properties.put(entry.getKey(), propVal);
        }

        return copyRPD;
    }

    /**
     * Lookup a global symbol with a given name (and resolves
     *
     * @param key              the name of the property
     * @param globalProperties
     * @return the property value or null if it doesn't exist.
     */
    public String getGlobalProperty(String key, Map<String, String> globalProperties) {
        if (!key.startsWith("${")) // is symbol already flat
            return key;

        while (true) {
            key = globalProperties.get(key);
            if (key == null || !(key.startsWith("${") && key.endsWith("}")))
                return key;
        }
    }

    /**
     * Provide information stored inside this Object, used mainly for debugging/testing.
     *
     * @return Description of object
     */
    @Override
    public String toString(){
        StringBuilder output = new StringBuilder().append("name : ").append(name);
        for (Object value : properties.values()) {
            if (value != null) {
                if (value instanceof String) {
                    output.append("value string : ");
                }
                output.append(value);
            }
        }
         return output.toString();
    }
}

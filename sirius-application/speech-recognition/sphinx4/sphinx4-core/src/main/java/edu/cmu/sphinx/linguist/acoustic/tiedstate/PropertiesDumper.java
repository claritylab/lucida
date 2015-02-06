/*
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


package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Properties;

/** Dumps out information about an acoustic model. */

public class PropertiesDumper {

    private final Properties props;

    /** Dumps the properties file 'model.props' that is in the same directory as this class.
     * @param argv*/
    public static void main(String[] argv) {
        try {
            PropertiesDumper dumper = new PropertiesDumper("model.props");
            System.out.println();
            System.out.println(dumper);
        } catch (IOException ioe) {
            ioe.printStackTrace();
        }
    }


    /**
     * Constructs a PropertiesDumper of the given acoustic model properties file.
     *
     * @param propsFile the properties file to dump
     * @throws java.io.IOException
     */
    public PropertiesDumper(String propsFile) throws IOException {
        props = new Properties();
        props.load(getClass().getResource(propsFile).openStream());
    }


    /**
     * Constructs a PropertiesDumper of the given acoustic model properties.
     *
     * @param properties the Properties object to dump
     * @throws java.io.IOException
     */
    public PropertiesDumper(Properties properties) throws IOException {
        props = properties;
    }


    /**
     * Returns a string of the properties.
     *
     * @return a string of the properties
     */
    @Override
    public String toString() {
        StringBuilder result = new StringBuilder().append(props.get("description")).append('\n');
        List<?> propnames = Collections.list(props.propertyNames());
        List<String> list = new ArrayList<String>();
        for (Object obj : propnames) {
            if (obj instanceof String) {
                list.add((String)obj);
            }
        }
        Collections.sort(list);
        for (String key : list) {
            String value = (String)props.get(key);
            result.append("\n\t").append(key).append(": ").append(value);
        }
        result.append('\n');
        return result.toString();
    }
}



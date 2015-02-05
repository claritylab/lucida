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

/**
 * Defines the interface that must be implemented by any configurable component in Sphinx-4.  The life cycle of a
 * component is as follows:
 * <p/>
 * <ul><li> <b>Class Parsing</b> The class file is parsed in order to determine all its configurable properties.  These
 * are defined using <code>public static final String</code> fields which are annotated with one of the following
 * annotations: <ul> <li>S4Integer <li>S4Double <li>S4Boolean <li>S4Component <li>S4ComponentList </ul> Further
 * informations about property-specific fields can be found in the javadoc of the property-annotation-definitions. Only
 * names of annotated properties will be allowed by the configuration system later on.
 * <p/>
 * <li> <b>Construction</b> - The (empty) component constructor is called in order to instantiate the component.
 * Typically the constructor does little, if any work, since the component has not been configured yet.
 * <p/>
 * <li> <b> Configuration</b> - Shortly after instantiation, the component's <code>newProperties</code> method is
 * called. This method is called with a <code>PropertySheet</code> containing the properties (usually taken from an
 * external configuration file). The component should extract the properties from the property sheet. If some properties
 * defined for a component does not fulfill the property definition given by the annotation (type, range, etc.) a
 * <code>PropertyException</code> is thrown. Typically, once a component gets its configuration data via the
 * <code>newData</code> method, the component will initialize itself.
 * <p/>
 * Note: In most cases <code>newProperties</code> is called only once as a result of system configuration during
 * startup. But nevertheless it is possible (and sometimes necessary) to reconfigure a component while it's running.
 * Therefore, a well behaved component should react properly to multiple <code>newProperties</code> calls. </ul>
 * <p/>
 * <p><b>Connecting to other components</b> <p> Components often need to interact with other components in the system.
 * One of the design goals of Sphinx-4 is that it allows for very flexible hook up of components in the system.
 * Therefore, it is *not* considered good S4 style to hardcode which subcomponents a particular subcomponent is
 * interacting with.  Instead, the component should use the configuration manager to provide the hook up to another
 * component.
 * <p/>
 * For example, if a component needs to interact with a Linguist. Instead of explicitly setting which linguist is to be
 * used via a constructor or via a <code>setLinguist</code> call, the component should instead define a configuration
 * property for the linguist.  This would be done like so:
 * <p/>
 * <code> <pre>
 *     \@S4Component(type=Linguist.class)
 *     public static String PROP_LINGUIST = "linguist";
 * </pre> </code>
 * <p> The linguist is made available in the <code>newProperties</code> method, like so: <p>
 * <code> <pre>
 *     public void newProperties(PropertySheet propertySheet) {
 *      linguist = (Linguist) propertySheet.getComponent(PROP_LINGUIST);
 *     }
 * </pre> </code>
 * <p/>
 * This <code>getComponent</code> call will find the proper linguist based upon the configuration data.  Thus, if the
 * configuration for this component had the 'linguist' defined to be 'dynamicLexTreeLinguist', then the configuration
 * manager will look up and return a linguist with that name, creating and configuring it as necessary.  Of course, the
 * dynamicLexTreeLinguist itself may have a number of sub-components that will be created and configured as a result. If
 * the component doesn't exist (but was defined to mandatory) and no configuration information is found in the config
 * file for it, or if it is of the wrong type, a <code>PropertyException</code> will be thrown.
 */
public interface Configurable {


    /**
     * This method is called when this configurable component needs to be reconfigured.
     *
     * @param ps a property sheet holding the new data
     * @throws PropertyException if there is a problem with the properties.
     */
    public void newProperties(PropertySheet ps) throws PropertyException;
}

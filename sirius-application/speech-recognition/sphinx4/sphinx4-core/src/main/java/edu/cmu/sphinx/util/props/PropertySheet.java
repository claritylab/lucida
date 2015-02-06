package edu.cmu.sphinx.util.props;

import java.lang.annotation.Annotation;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * A property sheet which defines a collection of properties for a single component
 * in the system.
 *
 * @author Holger Brandl
 */
public class PropertySheet implements Cloneable {

    public static final String COMP_LOG_LEVEL = "logLevel";

    private Map<String, S4PropWrapper> registeredProperties = new HashMap<String, S4PropWrapper>();
    private Map<String, Object> propValues = new HashMap<String, Object>();

    /**
     * Maps the names of the component properties to their (possibly unresolved) values.
     * <p/>
     * Example: <code>frontend</code> to <code>${myFrontEnd}</code>
     */
    private Map<String, Object> rawProps = new HashMap<String, Object>();

    private ConfigurationManager cm;
    private Configurable owner;
    private Class<? extends Configurable> ownerClass;

    private String instanceName;


    public PropertySheet(Configurable configurable, String name, RawPropertyData rpd, ConfigurationManager ConfigurationManager) {
        this(configurable.getClass(), name, ConfigurationManager, rpd);
        owner = configurable;
    }


    public PropertySheet(Class<? extends Configurable> confClass, String name, ConfigurationManager cm, RawPropertyData rpd) {
        ownerClass = confClass;
        this.cm = cm;
        this.instanceName = name;

        parseClass(confClass);
        setConfigurableClass(confClass);

        // now apply all xml properties
        Map<String, Object> flatProps = rpd.flatten(cm).getProperties();
        rawProps = new HashMap<String, Object>(rpd.getProperties());

        for (String propName : rawProps.keySet())
            propValues.put(propName, flatProps.get(propName));
    }


    /**
     * Registers a new property which type and default value are defined by the given sphinx property.
     *
     * @param propName The name of the property to be registered.
     * @param property The property annotation masked by a proxy.
     */
    private void registerProperty(String propName, S4PropWrapper property) {
        if (property == null || propName == null)
            throw new InternalConfigurationException(getInstanceName(), propName, "property or its value is null");

        if (!registeredProperties.containsKey(propName))
            registeredProperties.put(propName, property);

        if (!propValues.containsKey(propName)) {
            propValues.put(propName, null);
            rawProps.put(propName, null);
        }
    }


    /** Returns the property names <code>name</code> which is still wrapped into the annotation instance. */
    public S4PropWrapper getProperty(String name, Class<?> propertyClass) throws PropertyException {
        if (!propValues.containsKey(name))
            throw new InternalConfigurationException(getInstanceName(), name,
                    "Unknown property '" + name + "' ! Make sure that you've annotated it.");

        S4PropWrapper s4PropWrapper = registeredProperties.get(name);
        
        if (s4PropWrapper == null) {
            throw new InternalConfigurationException(getInstanceName(), name, "Property is not an annotated property of " + getConfigurableClass());
        }

        try {
            propertyClass.cast(s4PropWrapper.getAnnotation());
        } catch (ClassCastException e) {
            throw new InternalConfigurationException(e, getInstanceName(), name, "Property annotation " + s4PropWrapper.getAnnotation() + " doesn't match the required type " + propertyClass.getName());
        }

        return s4PropWrapper;
    }


    /**
     * Gets the value associated with this name
     *
     * @param name the name
     * @return the value
     */
    public String getString(String name) throws PropertyException {
        S4PropWrapper s4PropWrapper = getProperty(name, S4String.class);
        S4String s4String = ((S4String) s4PropWrapper.getAnnotation());

        if (propValues.get(name) == null) {
            boolean isDefDefined = !s4String.defaultValue().equals(S4String.NOT_DEFINED);

            if (s4String.mandatory()) {
                if (!isDefDefined)
                    throw new InternalConfigurationException(getInstanceName(), name, "mandatory property is not set!");
            }
            propValues.put(name, isDefDefined ? s4String.defaultValue() : null);
        }

        String propValue = flattenProp(name);

        // Check range
        List<String> range = Arrays.asList(s4String.range());
        if (!range.isEmpty() && !range.contains(propValue))
            throw new InternalConfigurationException(getInstanceName(), name, " is not in range (" + range + ')');

        return propValue;
    }


    private String flattenProp(String name) {
        Object value = propValues.get(name);
        return value instanceof String ? (String)value : null;
    }


    /**
     * Gets the value associated with this name
     *
     * @param name the name
     * @return the value
     * @throws edu.cmu.sphinx.util.props.PropertyException
     *          if the named property is not of this type
     */
    public int getInt(String name) throws PropertyException {
        S4PropWrapper s4PropWrapper = getProperty(name, S4Integer.class);
        S4Integer s4Integer = (S4Integer) s4PropWrapper.getAnnotation();

        if (propValues.get(name) == null) {
            boolean isDefDefined = !(s4Integer.defaultValue() == S4Integer.NOT_DEFINED);

            if (s4Integer.mandatory()) {
                if (!isDefDefined)
                    throw new InternalConfigurationException(getInstanceName(), name, "mandatory property is not set!");
            } else if (!isDefDefined)
                throw new InternalConfigurationException(getInstanceName(), name, "no default value for non-mandatory property");

            propValues.put(name, s4Integer.defaultValue());
        }

        Object propObject = propValues.get(name);
        Integer propValue = propObject instanceof Integer ? (Integer) propObject : Integer.decode(flattenProp(name));

        int[] range = s4Integer.range();
        if (range.length != 2)
            throw new InternalConfigurationException(getInstanceName(), name, Arrays.toString(range) + " is not of expected range type, which is {minValue, maxValue)");

        if (propValue < range[0] || propValue > range[1])
            throw new InternalConfigurationException(getInstanceName(), name, " is not in range (" + Arrays.toString(range) + ')');

        return propValue;
    }


    /**
     * Gets the value associated with this name
     *
     * @param name the name
     * @return the value
     * @throws edu.cmu.sphinx.util.props.PropertyException
     *          if the named property is not of this type
     */
    public float getFloat(String name) throws PropertyException {
        return ((Double) getDouble(name)).floatValue();
    }


    /**
     * Gets the value associated with this name
     *
     * @param name the name
     * @return the value
     * @throws edu.cmu.sphinx.util.props.PropertyException
     *          if the named property is not of this type
     */
    public double getDouble(String name) throws PropertyException {
        S4PropWrapper s4PropWrapper = getProperty(name, S4Double.class);
        S4Double s4Double = (S4Double) s4PropWrapper.getAnnotation();

        if (propValues.get(name) == null) {
            boolean isDefDefined = !(s4Double.defaultValue() == S4Double.NOT_DEFINED);

            if (s4Double.mandatory()) {
                if (!isDefDefined)
                    throw new InternalConfigurationException(getInstanceName(), name, "mandatory property is not set!");
            } else if (!isDefDefined)
                throw new InternalConfigurationException(getInstanceName(), name, "no default value for non-mandatory property");

            propValues.put(name, s4Double.defaultValue());
        }

        Object propObject = propValues.get(name);
        Double propValue;
	
        if (propObject instanceof Double)
    	    propValue = (Double)propObject;
        else if (propObject instanceof Number)
            propValue = ((Number)propObject).doubleValue();
        else
    	    propValue = Double.valueOf(flattenProp(name));

        double[] range = s4Double.range();
        if (range.length != 2)
            throw new InternalConfigurationException(getInstanceName(), name, Arrays.toString(range) + " is not of expected range type, which is {minValue, maxValue)");

        if (propValue < range[0] || propValue > range[1])
            throw new InternalConfigurationException(getInstanceName(), name, " is not in range (" + Arrays.toString(range) + ')');

        return propValue;
    }


    /**
     * Gets the value associated with this name
     *
     * @param name the name
     * @return the value
     * @throws edu.cmu.sphinx.util.props.PropertyException
     *          if the named property is not of this type
     */
    public Boolean getBoolean(String name) throws PropertyException {
        S4PropWrapper s4PropWrapper = getProperty(name, S4Boolean.class);
        S4Boolean s4Boolean = (S4Boolean) s4PropWrapper.getAnnotation();

        if (propValues.get(name) == null)
            propValues.put(name, s4Boolean.defaultValue());
 
        Object propObject = propValues.get(name);
        Boolean propValue;
        
        if (propObject instanceof Boolean)
            propValue = (Boolean) propObject;
        else
            propValue = Boolean.valueOf(flattenProp(name));
    
        return propValue;
    }

    /**
     * Gets a component associated with the given parameter name. First search
     * the component in property table, then try to get component by name from
     * the manager, then creates component with default properties.
     * 
     * @param name
     *            the parameter name
     * @return the component associated with the name
     * @throws edu.cmu.sphinx.util.props.PropertyException
     *             if the component does not exist or is of the wrong type.
     */
    public Configurable getComponent(String name) throws PropertyException {
        S4PropWrapper s4PropWrapper = getProperty(name, S4Component.class);
        Configurable configurable = null;

        S4Component s4Component = (S4Component) s4PropWrapper.getAnnotation();
        Class<? extends Configurable> expectedType = s4Component.type();

        Object propVal = propValues.get(name);

        if (propVal != null && propVal instanceof Configurable) {
            return (Configurable) propVal;
        }

        if (propVal != null && propVal instanceof String) {
            PropertySheet ps = cm.getPropertySheet(flattenProp(name));
            if (ps != null)
                configurable = ps.getOwner();
            else
                throw new InternalConfigurationException(getInstanceName(), name, "component '" + flattenProp(name)
                        + "' is missing");
        }

        if (configurable != null && !expectedType.isInstance(configurable))
            throw new InternalConfigurationException(getInstanceName(), name, "mismatch between annotation and component type");

        if (configurable != null) {
            propValues.put(name, configurable);
            return configurable;
        }

        configurable = getComponentFromAnnotation(name, s4Component);

        propValues.put(name, configurable);
        return configurable;
    }


    private Configurable getComponentFromAnnotation(String name, S4Component s4Component) {
        Configurable configurable;
        Class<? extends Configurable> defClass = s4Component.defaultClass();

        if (defClass.equals(Configurable.class) && s4Component.mandatory()) {
            throw new InternalConfigurationException(getInstanceName(), name, "mandatory property is not set!");
        }

        if (Modifier.isAbstract(defClass.getModifiers()) && s4Component.mandatory())
            throw new InternalConfigurationException(getInstanceName(), name, defClass.getName() + " is abstract!");

        // because we're forced to use the default type, make sure that it
        // is set
        if (defClass.equals(Configurable.class)) {
            if (s4Component.mandatory()) {
                throw new InternalConfigurationException(getInstanceName(), name, instanceName
                        + ": no default class defined for " + name);
            } else {
                return null;
            }
        }

        configurable = ConfigurationManager.getInstance(defClass);
        if (configurable == null) {
            throw new InternalConfigurationException(getInstanceName(), name, "instantiation of referenenced configurable failed");
        }
        
        return configurable;
    }

    /** Returns the class of of a registered component property without instantiating it. */
    public Class<? extends Configurable> getComponentClass(String propName) {
        Class<? extends Configurable> defClass = null;

        if (propValues.get(propName) != null)
            try {
                Class<?> objClass = Class.forName((String) propValues.get(propName));
                defClass = objClass.asSubclass(Configurable.class);
            } catch (ClassNotFoundException e) {
                PropertySheet ps = cm.getPropertySheet(flattenProp(propName));
                defClass = ps.ownerClass;
            }
        else {
            S4Component comAnno = (S4Component) registeredProperties.get(propName).getAnnotation();
            defClass = comAnno.defaultClass();
            if (comAnno.mandatory())
                defClass = null;
        }

        return defClass;
    }

    /**
     * Gets a list of float numbers associated with the given parameter name
     *
     * @param name the parameter name
     * @return a list of floats associated with the name.
     * @throws InternalConfigurationException if parameters are not double values.
     */
    public List<String> getStringList(String name) throws InternalConfigurationException {
        getProperty(name, S4StringList.class);

        return ConfigurationManagerUtils.toStringList (propValues.get(name));
    }
    
    /**
     * Gets a list of components associated with the given parameter name
     * 
     * @param name
     *            the parameter name
     * @param tclass
     *            the class of the list elements
     * @return the component associated with the name
     * @throws edu.cmu.sphinx.util.props.PropertyException
     *             if the component does not exist or is of the wrong type.
     */
    public <T> List<T> getComponentList(String name, Class<T> tclass)
            throws InternalConfigurationException {
        getProperty(name, S4ComponentList.class);

        List<?> components = (List<?>) propValues.get(name);

        assert registeredProperties.get(name).getAnnotation() instanceof S4ComponentList;
        S4ComponentList annotation = (S4ComponentList) registeredProperties
                .get(name).getAnnotation();

        // no components names are available and no component list was yet
        // loaded therefore load the default list of components from the 
        // annotation
        if (components == null) {
            List<Class<? extends Configurable>> defClasses = Arrays
                    .asList(annotation.defaultList());

            // if (annotation.mandatory() && defClasses.isEmpty())
            // throw new InternalConfigurationException(getInstanceName(), name,
            // "mandatory property is not set!");

            List<Configurable> defaultComponents = new ArrayList<Configurable>();

            for (Class<? extends Configurable> defClass : defClasses) {
                defaultComponents.add(ConfigurationManager.getInstance(defClass));
            }

            propValues.put(name, defaultComponents);
        
        } else if (!components.isEmpty()
                && !(components.get(0) instanceof Configurable)) {

            List<Configurable> resolvedComponents = new ArrayList<Configurable>();

            for (Object componentName : components) {
                Configurable configurable = cm.lookup((String) componentName);

                if (configurable != null) {
                    resolvedComponents.add(configurable);
                } else if (!annotation.beTolerant()) {
                    throw new InternalConfigurationException(name,
                            (String) componentName, "lookup of list-element '"
                                    + componentName + "' failed!");
                }
            }

            propValues.put(name, resolvedComponents);
        }

        List<?> values = (List<?>) propValues.get(name);
        ArrayList<T> result = new ArrayList<T>();
        for (Object obj : values) {
            if (tclass.isInstance(obj)) {
                result.add(tclass.cast(obj));
            } else {
                throw new InternalConfigurationException(getInstanceName(),
                        name, "Not all elements have required type " + tclass + " Found one of type " + obj.getClass());
            }
        }
        return result;
    }

    
    /**
     * Parses the string with multiple URL's separated by ;. Return the list of
     * resources to load
     * 
     * @param name
     *            list with URL's
     * @return list of resources
     */
    public List<URL> getResourceList(String name) {
        List<URL> resourceList = new ArrayList<URL>();
        String pathListString = getString(name);

        if (pathListString != null) {
            for (String url : pathListString.split(";")) {
                try {
                    URL resourceUrl = new URL(url);
                    resourceList.add(resourceUrl);
                } catch (MalformedURLException mue) {
                    throw new IllegalArgumentException(url
                            + " is not a valid URL.");
                }
            }
        }
        return resourceList;
    }
    
    
    public String getInstanceName() {
        return instanceName;
    }

    public void setInstanceName(String newInstanceName) {
        this.instanceName = newInstanceName;
    }


    /** Returns true if the owner of this property sheet is already instantiated. */
    public boolean isInstanciated() {
        return !(owner == null);
    }


    /**
     * Returns the owner of this property sheet. In most cases this will be the configurable instance which was
     * instrumented by this property sheet.
     */
    public synchronized Configurable getOwner() {
        try {

            if (!isInstanciated()) {
                // ensure that all mandatory properties are set before instantiating the component
                Collection<String> undefProps = getUndefinedMandatoryProps();
                if (!undefProps.isEmpty()) {
                    throw new InternalConfigurationException(getInstanceName(),
                            undefProps.toString(), "not all mandatory properties are defined");
                }

                owner = ownerClass.newInstance();
                owner.newProperties(this);
            }
        } catch (IllegalAccessException e) {
            throw new InternalConfigurationException(e, getInstanceName(), null, "Can't access class " + ownerClass);
        } catch (InstantiationException e) {
            throw new InternalConfigurationException(e, getInstanceName(), null, "Can't instantiate class " + ownerClass);
        }

        return owner;
    }


    /**
     * Returns the set of all component properties which were tagged as mandatory but which are not set (or no default
     * value is given).
     */
    public Collection<String> getUndefinedMandatoryProps() {
        Collection<String> undefProps = new ArrayList<String>();
        for (String propName : getRegisteredProperties()) {
            Annotation anno = registeredProperties.get(propName).getAnnotation();

            boolean isMandatory = false;
            if (anno instanceof S4Component) {
                isMandatory = ((S4Component) anno).mandatory() && ((S4Component) anno).defaultClass() == null;
            } else if (anno instanceof S4String) {
                isMandatory = ((S4String) anno).mandatory() && ((S4String) anno).defaultValue().equals(S4String.NOT_DEFINED);
            } else if (anno instanceof S4Integer) {
                isMandatory = ((S4Integer) anno).mandatory() && ((S4Integer) anno).defaultValue() == S4Integer.NOT_DEFINED;
            } else if (anno instanceof S4Double) {
                isMandatory = ((S4Double) anno).mandatory() && ((S4Double) anno).defaultValue() == S4Double.NOT_DEFINED;
            }

            if (isMandatory && !((rawProps.get(propName) != null) || (propValues.get(propName) != null)))
                undefProps.add(propName);
        }
        return undefProps;
    }


    /** Returns the class of the owner configurable of this property sheet. */
    public Class<? extends Configurable> getConfigurableClass() {
        return ownerClass;
    }


    /**
     * Sets the configurable class of this object.
     *
     * @throws RuntimeException if the the <code>Configurable</code> is already instantiated.
     */
    void setConfigurableClass(Class<? extends Configurable> confClass) {
        ownerClass = confClass;

        // Don't allow changes of the class if the configurable has already been instantiated
        if (isInstanciated())
            throw new RuntimeException("class is already instantiated");

        // clean up the properties if necessary
	// registeredProperties.clear();

        final Collection<String> classProperties = new HashSet<String>();
        final Map<Field, Annotation> classProps = parseClass(ownerClass);
        for (Map.Entry<Field, Annotation> entry : classProps.entrySet()) {
            try {
                String propertyName = (String)entry.getKey().get(null);

                // make sure that there is not already another property with this name
                assert !classProperties.contains(propertyName) :
                        "duplicate property-name for different properties: " + propertyName;

                registerProperty(propertyName, new S4PropWrapper(entry.getValue()));
                classProperties.add(propertyName);
            } catch (IllegalAccessException e) {
                e.printStackTrace();
            }
        }
    }


    /**
     * Sets the given property to the given name
     *
     * @param name the simple property name
     */
    public void setString(String name, String value) throws PropertyException {
        // ensure that there is such a property
        if (!registeredProperties.containsKey(name))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name +
                    "' is not a registered string-property");

        Annotation annotation = registeredProperties.get(name).getAnnotation();
        if (!(annotation instanceof S4String))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name + "' is of type string");

        applyConfigurationChange(name, value, value);
    }


    /**
     * Sets the given property to the given name
     *
     * @param name  the simple property name
     * @param value the value for the property
     */
    public void setInt(String name, int value) throws PropertyException {
        // ensure that there is such a property
        if (!registeredProperties.containsKey(name))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name +
                    "' is not a registered int-property");

        Annotation annotation = registeredProperties.get(name).getAnnotation();
        if (!(annotation instanceof S4Integer))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name + "' is of type int");

        applyConfigurationChange(name, value, value);
    }


    /**
     * Sets the given property to the given name
     *
     * @param name  the simple property name
     * @param value the value for the property
     */
    public void setDouble(String name, double value) throws PropertyException {
        // ensure that there is such a property
        if (!registeredProperties.containsKey(name))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name +
                    "' is not a registered double-property");

        Annotation annotation = registeredProperties.get(name).getAnnotation();
        if (!(annotation instanceof S4Double))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name + "' is of type double");

        applyConfigurationChange(name, value, value);
    }


    /**
     * Sets the given property to the given name
     *
     * @param name  the simple property name
     * @param value the value for the property
     */
    public void setBoolean(String name, Boolean value) throws PropertyException {
        if (!registeredProperties.containsKey(name))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name +
                    "' is not a registered boolean-property");

        Annotation annotation = registeredProperties.get(name).getAnnotation();
        if (!(annotation instanceof S4Boolean))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name + "' is of type boolean");

        applyConfigurationChange(name, value, value);
    }


    /**
     * Sets the given property to the given name
     *
     * @param name   the simple property name
     * @param cmName the name of the configurable within the configuration manager (required for serialization only)
     * @param value  the value for the property
     */
    public void setComponent(String name, String cmName, Configurable value) throws PropertyException {
        if (!registeredProperties.containsKey(name))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name +
                    "' is not a registered compontent");

        Annotation annotation = registeredProperties.get(name).getAnnotation();
        if (!(annotation instanceof S4Component))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name + "' is of type component");


        applyConfigurationChange(name, cmName, value);
    }


    /**
     * Sets the given property to the given name
     *
     * @param name       the simple property name
     * @param valueNames the list of names of the configurables within the configuration manager (required for
     *                   serialization only)
     * @param value      the value for the property
     */
    public void setComponentList(String name, List<String> valueNames, List<Configurable> value) throws PropertyException {
        if (!registeredProperties.containsKey(name))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name +
                    "' is not a registered component-list");

        Annotation annotation = registeredProperties.get(name).getAnnotation();
        if (!(annotation instanceof S4ComponentList))
            throw new InternalConfigurationException(getInstanceName(), name, '\'' + name + "' is of type component-list");

        rawProps.put(name, valueNames);
        propValues.put(name, value);

        applyConfigurationChange(name, valueNames, value);
    }


    private void applyConfigurationChange(String propName, Object cmName, Object value) throws PropertyException {
        rawProps.put(propName, cmName);
        propValues.put(propName, value != null ? value : cmName);

        if (getInstanceName() != null)
            cm.fireConfChanged(getInstanceName(), propName);

        if (owner != null)
            owner.newProperties(this);
    }


    /**
     * Sets the raw property to the given name
     *
     * @param key the simple property name
     * @param val the value for the property
     */
    void setRaw(String key, Object val) {
        rawProps.put(key, val);
        propValues.put(key, null);
    }


    /**
     * Gets the raw value associated with this name
     *
     * @param name the name
     * @return the value as an object (it could be a String or a String[] depending upon the property type)
     */
    public Object getRaw(String name) {
        return rawProps.get(name);
    }


    /**
     * Gets the raw value associated with this name, no global symbol replacement is performed.
     *
     * @param name the name
     * @return the value as an object (it could be a String or a String[] depending upon the property type)
     */
    public Object getRawNoReplacement(String name) {
        return rawProps.get(name);
    }


    /** Returns the type of the given property. */
    public PropertyType getType(String propName) {
        S4PropWrapper wrapper = registeredProperties.get(propName);
        if (wrapper == null) {
            throw new InternalConfigurationException(getInstanceName(), propName, " is not a valid property of" + getConfigurableClass());
        }

        Annotation annotation = wrapper.getAnnotation();
        if (annotation instanceof S4Component)
            return PropertyType.COMPONENT;
        else if (annotation instanceof S4ComponentList)
            return PropertyType.COMPONENT_LIST;
        else if (annotation instanceof S4Integer)
            return PropertyType.INT;
        else if (annotation instanceof S4Double)
            return PropertyType.DOUBLE;
        else if (annotation instanceof S4Boolean)
            return PropertyType.BOOLEAN;
        else if (annotation instanceof S4String)
            return PropertyType.STRING;
        else
            throw new RuntimeException("Unknown property type");
    }


    /**
     * Gets the owning property manager
     *
     * @return the property manager
     */
    ConfigurationManager getPropertyManager() {
        return cm;
    }


    /**
     * Returns a logger to use for this configurable component. The logger can be configured with the property:
     * 'logLevel' - The default logLevel value is defined (within the xml configuration file by the global property
     * 'defaultLogLevel' (which defaults to WARNING).
     * <p/>
     * implementation note: the logger became configured within the constructor of the parenting configuration manager.
     *
     * @return the logger for this component
     * @throws edu.cmu.sphinx.util.props.PropertyException
     *          if an error occurs
     */
    public Logger getLogger() {
        Logger logger;

        String baseName = ConfigurationManagerUtils.getLogPrefix(cm) + ownerClass.getName();
        if (instanceName != null) {
            logger = Logger.getLogger(baseName + '.' + instanceName);
        } else
            logger = Logger.getLogger(baseName);

        // if there's a logLevel set for component apply to the logger
        Object rawLogLevel = rawProps.get(COMP_LOG_LEVEL);
        if (rawLogLevel != null)
            logger.setLevel(rawLogLevel instanceof String ? Level.parse((String) rawLogLevel) : (Level) rawLogLevel);

        return logger;
    }


    /** Returns the names of registered properties of this PropertySheet object. */
    public Collection<String> getRegisteredProperties() {
        return Collections.unmodifiableCollection(registeredProperties.keySet());
    }


    public void setCM(ConfigurationManager cm) {
        this.cm = cm;
    }


    /**
     * Returns true if two property sheet define the same object in terms of configuration. The owner (and the parent
     * configuration manager) are not expected to be the same.
     */
    @Override
    public boolean equals(Object obj) {
        if (obj == null || !(obj instanceof PropertySheet))
            return false;

        PropertySheet ps = (PropertySheet) obj;
        if (!rawProps.keySet().equals(ps.rawProps.keySet()))
            return false;

        // maybe we could test a little bit more here. suggestions?
        return true;
    }

    @Override
    public int hashCode() {
  	  	assert false : "hashCode not designed";
  	  	return 1; // any arbitrary constant will do 
    }

    @Override
    public String toString() {
        return getInstanceName() + "; isInstantiated=" + isInstanciated() + "; props=" + rawProps.keySet();
    }
        
    @Override
    protected PropertySheet clone() throws CloneNotSupportedException {
        PropertySheet ps = (PropertySheet)super.clone();

        ps.registeredProperties = new HashMap<String, S4PropWrapper>(this.registeredProperties);
        ps.propValues = new HashMap<String, Object>(this.propValues);

        ps.rawProps = new HashMap<String, Object>(this.rawProps);

        // make deep copy of raw-lists
        for (String regProp : ps.getRegisteredProperties()) {
            if (getType(regProp) == PropertyType.COMPONENT_LIST) {
                ps.rawProps.put(regProp, ConfigurationManagerUtils.toStringList(rawProps.get(regProp)));
                ps.propValues.put(regProp, null);
            }
        }

        ps.cm = cm;
        ps.owner = null;
        ps.instanceName = this.instanceName;

        return ps;
    }


    /** Validates a configuration, by ensuring that only valid property-names have been used to configure the component. */
    public boolean validate() {
        for (String propName : rawProps.keySet()) {
            if (propName.equals(ConfigurationManagerUtils.GLOBAL_COMMON_LOGLEVEL))
                continue;

            if (!registeredProperties.containsKey(propName))
                return false;
        }

        return true;
    }


    /**
     * use annotation based class parsing to detect the configurable properties of a <code>Configurable</code>-class
     *
     * @param configurable of type Class
     */
    private static Map<Field, Annotation> parseClass(Class<? extends Configurable> configurable) {
        Field[] classFields = configurable.getFields();

        Map<Field, Annotation> s4props = new HashMap<Field, Annotation>();
        for (Field field : classFields) {
            Annotation[] annotations = field.getAnnotations();

            for (Annotation annotation : annotations) {
                Annotation[] superAnnotations = annotation.annotationType().getAnnotations();

                for (Annotation superAnnotation : superAnnotations) {
                    if (superAnnotation instanceof S4Property) {
                        int fieldModifiers = field.getModifiers();
                        assert Modifier.isStatic(fieldModifiers) : "property fields are assumed to be static";
                        assert Modifier.isPublic(fieldModifiers) : "property fields are assumed to be public";
                        assert Modifier.isFinal(fieldModifiers) : "property fields are assumed to be final";
                        assert field.getType().equals(String.class) : "properties fields are assumed to be instances of java.lang.String";

                        s4props.put(field, annotation);
                    }
                }
            }
        }

        return s4props;
    }
}

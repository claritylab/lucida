package edu.cmu.sphinx.util.props;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;
import java.util.logging.Handler;
import java.util.logging.ConsoleHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import edu.cmu.sphinx.util.SphinxLogFormatter;

/**
 * Some static utility methods which ease the handling of system configurations.
 *
 * @author Holger Brandl
 */
public final class ConfigurationManagerUtils {

    // this pattern matches strings of the form '${word}'
    private static final Pattern globalSymbolPattern = Pattern.compile("\\$\\{(\\w+)\\}");

    /**
     * A common property (used by all components) that sets the log level for the component.
     */
    public final static String GLOBAL_COMMON_LOGLEVEL = "logLevel";

    /**
     * The default file suffix of configuration files.
     */
    public static final String CM_FILE_SUFFIX = ".sxl";


    // disabled constructor because the class is just a collection of utilities for handling system configurations

    private ConfigurationManagerUtils() {
    }


    /**
     * Validates that only annotated property names have been used to setup this instance of {@code
     * edu.cmu.sphinx.util.props.ConfigurationManager}.
     *
     * @return {@code true} if it is a valid configuration.
     */
    public boolean validateConfiguration(ConfigurationManager cm) {
        for (String compName : cm.getComponentNames()) {
            if (!cm.getPropertySheet(compName).validate())
                return false;
        }

        return true;
    }


    /**
     * Strips the ${ and } off of a global symbol of the form ${symbol}.
     *
     * @param symbol the symbol to strip
     * @return the stripped symbol
     */
    public static String stripGlobalSymbol(String symbol) {
        Matcher matcher = globalSymbolPattern.matcher(symbol);
        if (matcher.matches()) {
            return matcher.group(1);
        } else {
            return symbol;
        }
    }


    public static void editConfig(ConfigurationManager cm, String name) {
        PropertySheet ps = cm.getPropertySheet(name);
        boolean done;

        if (ps == null) {
            System.out.println("No component: " + name);
            return;
        }
        System.out.println(name + ':');

        Collection<String> propertyNames = ps.getRegisteredProperties();
        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));

        for (String propertyName : propertyNames) {
            try {
                Object value = ps.getRaw(propertyName);
                String svalue;

                if (value instanceof List<?>) {
                    continue;
                } else if (value instanceof String) {
                    svalue = (String) value;
                } else {
                    svalue = "DEFAULT";
                }
                done = false;

                while (!done) {
                    System.out.print("  " + propertyName + " [" + svalue + "]: ");
                    String in = br.readLine();
                    if (in.isEmpty()) {
                        done = true;
                    } else if (in.equals(".")) {
                        return;
                    } else {

                        cm.getPropertySheet(name).setRaw(propertyName, in);
                        done = true;
                    }
                }
            } catch (IOException ioe) {
                System.out.println("Trouble reading input");
                return;
            }
        }
    }


    // remark: the replacement of xml/sxl suffix is not necessary and just done to improve readability
    public static String getLogPrefix(ConfigurationManager cm) {
        if (cm.getConfigURL() != null)
            return new File(cm.getConfigURL().getFile()).getName().replace(".sxl", "").replace(".xml", "") + '.';
        else
            return "S4CM.";
    }
    
     /**
     * Configure the logger
     */
    public static void configureLogger(ConfigurationManager cm) {

        // Allow others to override the logging settings.
        if (System.getProperty("java.util.logging.config.class") != null
                || System.getProperty("java.util.logging.config.file") != null) {
            return;
        }
        // apply the log level (if defined) for the root logger (because we're using package based logging now)

        String cmPrefix = getLogPrefix(cm);
        Logger cmRootLogger = Logger.getLogger(cmPrefix.substring(0, cmPrefix.length() - 1));

        // we need to determine the root-level here, because the logManager will reset it
        Level rootLevel = Logger.getLogger("").getLevel();

        configureLogger(cmRootLogger);

        String level = cm.getGlobalProperty(GLOBAL_COMMON_LOGLEVEL);
        if (level == null)
            level = Level.WARNING.getName();

        cmRootLogger.setLevel(Level.parse(level));

        // restore the old root logger level
        Logger.getLogger("").setLevel(rootLevel);
    }


    /**
     * Configures a logger to use the sphinx4-log-formatter.
     */
    public static void configureLogger(Logger logger) {

        logger.setUseParentHandlers(false);

	boolean hasHandler = false;

        for (Handler handler : logger.getHandlers()) {
            if (handler.getFormatter() instanceof SphinxLogFormatter) {
                hasHandler = true;
                break;
            }
        }

        if (!hasHandler) {
            ConsoleHandler handler = new ConsoleHandler();
            handler.setFormatter(new SphinxLogFormatter());
            logger.addHandler(handler);
        }
    }


    /**
     * This method will automatically rename all components of <code>subCM</code> for which there is component named the
     * same in the <code>baseCM</code> .
     * <p/>
     * Note: This is required when merging two system configurations into one.
     *
     * @return A map which maps all renamed component names to their new names.
     */
    public static Map<String, String> fixDuplicateNames(ConfigurationManager baseCM, ConfigurationManager subCM) {
        Map<String, String> renames = new HashMap<String, String>();

        for (String compName : subCM.getComponentNames()) {
            String uniqueName = compName;

            int i = 0;

            while (baseCM.getComponentNames().contains(uniqueName) ||
                    (subCM.getComponentNames().contains(uniqueName) && !uniqueName.equals(compName))) {

                i++;
                uniqueName = compName + i;
            }

            subCM.renameConfigurable(compName, uniqueName);
            renames.put(compName, uniqueName);
        }

        return renames;
    }


    /**
     * converts a configuration manager instance into a xml-string .
     * <p/>
     * Note: This methods will not instantiate configurables.
     */
    public static String toXML(ConfigurationManager cm) {
        StringBuilder sb = new StringBuilder();
        sb.append("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        sb.append("\n<!--    Sphinx-4 Configuration file--> \n\n");

        sb.append("<config>");

        Pattern pattern = Pattern.compile("\\$\\{(\\w+)\\}");

        Map<String, String> globalProps = cm.getGlobalProperties();
        for (Map.Entry<String, String> entry : globalProps.entrySet()) {
            String propName = entry.getKey();

            Matcher matcher = pattern.matcher(propName);
            propName = matcher.matches() ? matcher.group(1) : propName;

            sb.append("\n\t<property name=\"").append(propName).append("\" value=\"").append(entry.getValue()).append("\"/>");
        }

        for (String instanceName : cm.getComponentNames())
            sb.append("\n\n").append(propSheet2XML(instanceName, cm.getPropertySheet(instanceName)));

        sb.append("\n</config>");
        return sb.toString();
    }


    private static String propSheet2XML(String instanceName, PropertySheet ps) {
        StringBuilder sb = new StringBuilder();
        sb.append("\t<component name=\"").append(instanceName).append("\" type=\"").append(ps.getConfigurableClass().getName()).append("\">");

        for (String propName : ps.getRegisteredProperties()) {
            String predec = "\n\t\t<property name=\"" + propName + "\" ";
            if (ps.getRawNoReplacement(propName) == null)
                continue;  // if the property was net defined within the xml-file

            switch (ps.getType(propName)) {

                case COMPONENT_LIST:
                    sb.append("\n\t\t<propertylist name=\"").append(propName).append("\">");
                    List<String> compNames = toStringList(ps.getRawNoReplacement(propName));
                    for (String compName : compNames)
                        sb.append("\n\t\t\t<item>").append(compName).append("</item>");
                    sb.append("\n\t\t</propertylist>");
                    break;
                default:
                    sb.append(predec).append("value=\"").append(ps.getRawNoReplacement(propName)).append("\"/>");
            }
        }

        sb.append("\n\t</component>\n\n");
        return sb.toString();
    }


    public static void save(ConfigurationManager cm, File cmLocation) {
        if (!cmLocation.getName().endsWith(CM_FILE_SUFFIX))
            System.err.println("WARNING: Serialized s4-configuration should have the suffix '" + CM_FILE_SUFFIX + '\'');

        assert cm != null;
        try {
            PrintWriter pw = new PrintWriter(new FileOutputStream(cmLocation));
            String configXML = ConfigurationManagerUtils.toXML(cm);
            pw.print(configXML);
            pw.flush();
            pw.close();
        } catch (FileNotFoundException e1) {
            e1.printStackTrace();
        }
    }


    /**
     * Shows the current configuration
     */
    public static void showConfig(ConfigurationManager cm) {
        System.out.println(" ============ config ============= ");
        for (String allName : cm.getInstanceNames(Configurable.class)) {
            showConfig(cm, allName);
        }
    }


    /**
     * Show the configuration for the component with the given name
     *
     * @param name the component name
     */
    public static void showConfig(ConfigurationManager cm, String name) {
//        Symbol symbol = cm.getsymbolTable.get(name);

        if (!cm.getComponentNames().contains(name)) {
            System.out.println("No component: " + name);
            return;
        }
        System.out.println(name + ':');

        PropertySheet properties = cm.getPropertySheet(name);

        for (String propertyName : properties.getRegisteredProperties()) {
            System.out.print("    " + propertyName + " = ");
            Object obj;
            obj = properties.getRaw(propertyName);
            if (obj instanceof String) {
                System.out.println(obj);
            } else if (obj instanceof List<?>) {
                List<?> l = (List<?>) obj;
                for (Iterator<?> k = l.iterator(); k.hasNext();) {
                    System.out.print(k.next());
                    if (k.hasNext()) {
                        System.out.print(", ");
                    }
                }
                System.out.println();
            } else {
                System.out.println("[DEFAULT]");
            }
        }
    }


    /**
     * Applies the system properties to the raw property map. System properties should be of the form
     * compName[paramName]=paramValue
     * <p/>
     * List types cannot currently be set from system properties.
     *
     * @param rawMap the map of raw property values
     * @param global global properties
     * @throws PropertyException if an attempt is made to set a parameter for an unknown component.
     */
    static void applySystemProperties(Map<String, RawPropertyData> rawMap, Map<String, String> global)
            throws PropertyException {
        Properties props = System.getProperties();
        for (Enumeration<?> e = props.keys(); e.hasMoreElements();) {
            String param = (String) e.nextElement();
            String value = props.getProperty(param);

            // search for parameters of the form component[parameter]=value
            // these go in the property sheet for the component
            int lb = param.indexOf('[');
            int rb = param.indexOf(']');

            if (lb > 0 && rb > lb) {
                String compName = param.substring(0, lb);
                String paramName = param.substring(lb + 1, rb);
                RawPropertyData rpd = rawMap.get(compName);
                if (rpd != null) {
                    rpd.add(paramName, value);
                } else {
                    throw new InternalConfigurationException(compName, param,
                            "System property attempting to set parameter "
                                    + " for unknown component " + compName
                                    + " (" + param + ')');
                }
            }

            // look for parameters of the form foo=bar
            // these go in the global map

            else if (param.indexOf('.') == -1) {
                global.put(param, value);
            }
        }
    }


    /**
     * Renames a given <code>Configurable</code>. The configurable component named <code>oldName</code> is assumed to be
     * registered to the CM. Renaming does not only affect the configurable itself but possibly global property values
     * and properties of other components.
     */
    static void renameComponent(ConfigurationManager cm, String oldName, String newName) {
        assert cm != null;
        assert oldName != null && newName != null;
        if (cm.getPropertySheet(oldName) == null) {
            throw new RuntimeException("no configurable (to be renamed) named " + oldName + " is contained in the CM");
        }

        // this iteration is a little hacky. It would be much better to maintain the links to a configurable in a special table
        for (String instanceName : cm.getComponentNames()) {
            PropertySheet propSheet = cm.getPropertySheet(instanceName);

            for (String propName : propSheet.getRegisteredProperties()) {
                if (propSheet.getRawNoReplacement(propName) == null)
                    continue;  // if the property was net defined within the xml-file

                switch (propSheet.getType(propName)) {

                    case COMPONENT_LIST:
                        List<String> compNames = toStringList(propSheet.getRawNoReplacement(propName));
                        for (int i = 0; i < compNames.size(); i++) {
                            String compName = compNames.get(i);
                            if (compName.equals(oldName)) {
                                compNames.set(i, newName);
                            }
                        }

                        break;
                    case COMPONENT:
                        if (propSheet.getRawNoReplacement(propName).equals(oldName)) {
                            propSheet.setRaw(propName, newName);
                        }
                    default:
                    	break;
                }
            }
        }

        PropertySheet ps = cm.getPropertySheet(oldName);
        ps.setInstanceName(newName);

        // it might be possible that the component is the value of a global property
        for (Map.Entry<String, String> entry : cm.getGlobalProperties().entrySet()) {
            if (entry.getValue().equals(oldName))
                cm.setGlobalProperty(entry.getKey(), newName);
        }
    }


    /**
     * Gets a resource associated with the given parameter name given an property sheet.
     *
     * @param name the parameter name
     * @param ps   The property sheet which contains the property
     * @return the resource associated with the name or NULL if it doesn't exist.
     * @throws PropertyException if the resource cannot be found
     */
    public static URL getResource(String name, PropertySheet ps) throws PropertyException {

        String location = ps.getString(name);
        if (location == null) {
            throw new InternalConfigurationException(ps.getInstanceName(), name, "Required resource property '" + name + "' not set");
        }

        try {
            URL url = resourceToURL(location);

            if (url == null) {
                throw new InternalConfigurationException(ps.getInstanceName(), name, "Can't locate " + location);
            }
            return url;
        } catch (MalformedURLException e) {
            throw new InternalConfigurationException(e, ps.getInstanceName(), name, "Bad URL " + location + e.getMessage());
        }
        
    }

    final static Pattern jarPattern = Pattern.compile("resource:(.*)", Pattern.CASE_INSENSITIVE);

    public static URL resourceToURL(String location) throws MalformedURLException {
        Matcher jarMatcher = jarPattern.matcher(location);
        if (jarMatcher.matches()) {
            String resourceName = jarMatcher.group(1);
            return ConfigurationManagerUtils.class.getResource(resourceName);
        } else {
            if (location.indexOf(':') == -1) {
                location = "file:" + location;
            }
            return new URL(location);
        }
    }


    /**
     * @return <code>true</code> if <code>aClass</code> is either equal to <code>poosibleParent</code>, a subclass of
     *         it, or implements it if <code>possibleParent</code> is an interface.
     */
    public static boolean isDerivedClass(Class<?> derived, Class<?> parent) {
        return parent.isAssignableFrom(derived);
    }


    public static boolean isImplementingInterface(Class<?> aClass, Class<?> interfaceClass) {
        assert interfaceClass.isInterface();

        Class<?> superClass = aClass.getSuperclass();
        if (superClass != null && isImplementingInterface(superClass, interfaceClass))
            return true;

        for (Class<?> curInterface : aClass.getInterfaces()) {
            if (curInterface.equals(interfaceClass) || isImplementingInterface(curInterface, interfaceClass))
                return true;
        }

        return false;
    }


    public static boolean isSubClass(Class<?> aClass, Class<?> possibleSuperclass) {
        while (aClass != null && !aClass.equals(Object.class)) {
            aClass = aClass.getSuperclass();

            if (aClass != null && aClass.equals(possibleSuperclass))
                return true;
        }

        return false;
    }


    /**
     * Why do we need this method? The reason is, that we would like to avoid this method to be part of the
     * <code>PropertySheet</code>-API. In some circumstances it is nevertheless required to get access to the managing
     * <code>ConfigurationManager</code>.
     */
    public static ConfigurationManager getPropertyManager(PropertySheet ps) {
        return ps.getPropertyManager();
    }


    /**
     * Returns a map of all component-properties of this config-manager (including their associated property-sheets.
     */
    public static Map<String, List<PropertySheet>> listAllsPropNames(ConfigurationManager cm) {
        Map<String, List<PropertySheet>> allProps = new HashMap<String, List<PropertySheet>>();

        for (String configName : cm.getComponentNames()) {
            PropertySheet ps = cm.getPropertySheet(configName);

            for (String propName : ps.getRegisteredProperties()) {
                if (!allProps.containsKey(propName))
                    allProps.put(propName, new ArrayList<PropertySheet>());

                allProps.get(propName).add(ps);
            }
        }

        return allProps;
    }


    public static void dumpPropStructure(ConfigurationManager cm) {
        Map<String, List<PropertySheet>> allProps = listAllsPropNames(cm);

        System.out.println("Property-structure of '" + cm.getConfigURL() + "':");

        // print non-ambiguous props first
        System.out.println("\nUnambiguous properties = ");
        for (Map.Entry<String, List<PropertySheet>> entry : allProps.entrySet()) {
            if (entry.getValue().size() == 1)
                System.out.print(entry.getKey() + ", ");
        }

        // now print ambiguous properties (including the associated components
        System.out.println("\n\nAmbiguous properties: ");
        for (Map.Entry<String, List<PropertySheet>> entry : allProps.entrySet()) {
            if (entry.getValue().size() == 1)
                continue;

            System.out.print(entry.getKey() + '=');
            for (PropertySheet ps : entry.getValue()) {
                System.out.print(ps.getInstanceName() + ", ");
            }
            System.out.println();
        }
    }


    /**
     * Attempts to set the value of an arbitrary component-property. If the property-name is ambiguous  with respect to
     * the given <code>ConfiguratioManager</code> an extended syntax (componentName->propName) can be used to access the
     * property.
     * <p/>
     * Beside component properties it is also possible to modify the class of a configurable, but this is only allowed if
     * the configurable under question has not been instantiated yet. Furthermore the user has to ensure to set all
     * mandatory component properties.
     */
    public static void setProperty(ConfigurationManager cm, String propName, String propValue) {
        assert propValue != null;

        Map<String, List<PropertySheet>> allProps = listAllsPropNames(cm);
        Set<String> configurableNames = cm.getComponentNames();

        if (!allProps.containsKey(propName) && !propName.contains("->") && !configurableNames.contains(propName))
            throw new RuntimeException("No property or configurable '" + propName + "' in configuration '" + cm.getConfigURL() + "'!");

        // if a configurable-class should be modified
        if (configurableNames.contains(propName)) {
            try {
                final Class<? extends Configurable> confClass = Class.forName(propValue).asSubclass(Configurable.class);
                ConfigurationManagerUtils.setClass(cm.getPropertySheet(propName), confClass);
            } catch (ClassNotFoundException e) {
                throw new RuntimeException(e);
            }

            return;
        }

        if (!propName.contains("->") && allProps.get(propName).size() > 1) {
            throw new RuntimeException("Property-name '" + propName + "' is ambiguous with respect to configuration '"
                    + cm.getConfigURL() + "'. Use 'componentName->propName' to disambiguate your request.");
        }

        String componentName;

        // if disambiguation syntax is used find the correct PS first
        if (propName.contains("->")) {
            String[] splitProp = propName.split("->");
            componentName = splitProp[0];
            propName = splitProp[1];
        } else {
            componentName = allProps.get(propName).get(0).getInstanceName();
        }

        setProperty(cm, componentName, propName, propValue);
    }

    public static void setProperty(ConfigurationManager cm, String componentName, String propName, String propValue) {

        // now set the property
        PropertySheet ps = cm.getPropertySheet(componentName);
        if (ps == null)
            throw new RuntimeException("Component '" + propName + "' is not registered to this system configuration '");

        // set the value to null if the string content is 'null
        if (propValue.equals("null"))
            propValue = null;

        switch (ps.getType(propName)) {
            case BOOLEAN:
                ps.setBoolean(propName, Boolean.valueOf(propValue));
                break;
            case DOUBLE:
                ps.setDouble(propName, new Double(propValue));
                break;
            case INT:
                ps.setInt(propName, new Integer(propValue));
                break;
            case STRING:
                ps.setString(propName, propValue);
                break;
            case COMPONENT:
                ps.setComponent(propName, propValue, null);
                break;
            case COMPONENT_LIST:
                List<String> compNames = new ArrayList<String>();
                for (String component : propValue.split(";")) {
                    compNames.add(component.trim());
                }

                ps.setComponentList(propName, compNames, null);
                break;
            default:
                throw new RuntimeException("unknown property-type");
        }
    }

    public static URL getURL(File file) {
        try {
            return file.toURI().toURL();
        } catch (MalformedURLException e) {
            e.printStackTrace();
        }

        return null;
    }


    /**
     * Returns the not yet instantiated components registered to this configuration manager.
     */
    public static Collection<String> getNonInstaniatedComps(ConfigurationManager cm) {
        Collection<String> nonInstComponents = new ArrayList<String>();

        for (String compName : cm.getComponentNames()) {
            if (!cm.getPropertySheet(compName).isInstanciated())
                nonInstComponents.add(compName);
        }
        return nonInstComponents;
    }


    public static void setClass(PropertySheet ps, Class<? extends Configurable> confClass) {
        if (ps.isInstanciated())
            throw new RuntimeException("configurable " + ps.getInstanceName() + "has already been instantiated");

        ps.setConfigurableClass(confClass);
    }


    public static List<String> toStringList(Object obj) {
        List<String> result = new ArrayList<String>();
        if (!(obj instanceof List<?>))
            return null;
        for (Object o : (List<?>) obj) {
            if (o instanceof String) {
                result.add((String) o);
            }
        }
        return result;
    }
}

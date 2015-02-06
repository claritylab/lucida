package edu.cmu.sphinx.util.props.tools;

import edu.cmu.sphinx.util.props.*;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.util.Collection;
import java.util.List;

/**
 * Dumps a given configuration manager as GDL. ...
 *
 * @author Holger Brandl
 */
public class GDLDumper {


    /**
     * Dumps the given component as GDL to the given stream
     *
     * @param out  where to dump the GDL
     * @param name the name of the component to dump
     */
    public static void dumpComponentAsGDL(ConfigurationManager cm, PrintStream out, String name) {

        out.println("node: {title: \"" + name + "\" color: " + getColor(cm, name)
                + '}');

        PropertySheet ps = cm.getPropertySheet(name);
        Collection<String> propertyNames = ps.getRegisteredProperties();

        for (String propertyName : propertyNames) {
            PropertyType propType = ps.getType(propertyName);
            Object val = ps.getRaw(propertyName);

            if (val != null) {
                if (propType == PropertyType.COMPONENT) {
                    out.println("edge: {source: \"" + name
                            + "\" target: \"" + val + "\"}");
                } else if (propType == PropertyType.COMPONENT_LIST) {
                    List<?> list = (List<?>) val;
                    for (Object listElement : list) {
                        out.println("edge: {source: \"" + name
                                + "\" target: \"" + listElement + "\"}");
                    }
                }
            }
        }
    }


    /**
     * Dumps the config as a GDL plot
     *
     * @param path where to output the GDL
     * @throws java.io.IOException if an error occurs
     */
    public static void showConfigAsGDL(ConfigurationManager ConfigurationManager, String path) throws IOException {
        PrintStream out = new PrintStream(new FileOutputStream(path));
        dumpGDLHeader(out);
        for (String componentName : ConfigurationManager.getInstanceNames(Configurable.class)) {
            dumpComponentAsGDL(ConfigurationManager, out, componentName);
        }
        dumpGDLFooter(out);
        out.close();
    }


    /**
     * Outputs the GDL header
     *
     * @param out the output stream
     */
    public static void dumpGDLHeader(PrintStream out) {
        out.println(" graph: {title: \"unix evolution\" ");
        out.println("         layoutalgorithm: tree");
        out.println("          scaling        : 2.0");
        out.println("          colorentry 42  : 152 222 255");
        out.println("     node.shape     : ellipse");
        out.println("      node.color     : 42 ");
        out.println("node.height    : 32  ");
        out.println("node.fontname  : \"helvB08\"");
        out.println("edge.color     : darkred");
        out.println("edge.arrowsize :  6    ");
        out.println("node.textcolor : darkblue ");
        out.println("splines        : yes");
    }


    /**
     * Gets the color for the given component
     *
     * @param ConfigurationManager
     * @param componentName        the name of the component @return the color name for the component
     */
    public static String getColor(ConfigurationManager ConfigurationManager, String componentName) {
        try {
            Configurable c = ConfigurationManager.lookup(componentName);
            Class<? extends Configurable> cls = c.getClass();
            if (cls.getName().indexOf(".recognizer") > 1) {
                return "cyan";
            } else if (cls.getName().indexOf(".tools") > 1) {
                return "darkcyan";
            } else if (cls.getName().indexOf(".decoder") > 1) {
                return "green";
            } else if (cls.getName().indexOf(".frontend") > 1) {
                return "orange";
            } else if (cls.getName().indexOf(".acoustic") > 1) {
                return "turquoise";
            } else if (cls.getName().indexOf(".linguist") > 1) {
                return "lightblue";
            } else if (cls.getName().indexOf(".instrumentation") > 1) {
                return "lightgrey";
            } else if (cls.getName().indexOf(".util") > 1) {
                return "lightgrey";
            }
        } catch (PropertyException e) {
            return "black";
        }
        return "darkgrey";
    }


    /**
     * Dumps the footer for GDL output
     *
     * @param out the output stream
     */
    public static void dumpGDLFooter(PrintStream out) {
        out.println("}");
    }
}

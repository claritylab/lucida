package edu.cmu.sphinx.util.props.tools;

import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.PropertySheet;

import java.io.IOException;
import java.io.PrintStream;
import java.io.FileOutputStream;
import java.util.Collection;
import java.util.List;

/**
 * Dumps a given configuration manager to HTML.
 *
 * @author Holger Brandl
 */
public class HTMLDumper {

    /**
     * Dumps the config as a set of HTML tables
     *
     * @param path where to output the HTML
     * @throws java.io.IOException if an error occurs
     */
    public static void showConfigAsHTML(ConfigurationManager ConfigurationManager, String path) throws IOException {
        PrintStream out = new PrintStream(new FileOutputStream(path));
        dumpHeader(out);
        for (String componentName : ConfigurationManager.getInstanceNames(Configurable.class)) {
            dumpComponentAsHTML(out, componentName, ConfigurationManager.getPropertySheet(componentName));
        }
        dumpFooter(out);
        out.close();
    }


    /**
     * Dumps the footer for HTML output
     *
     * @param out the output stream
     */
    public static void dumpFooter(PrintStream out) {
        out.println("</body>");
        out.println("</html>");
    }


    /**
     * Dumps the header for HTML output
     *
     * @param out the output stream
     */
    public static void dumpHeader(PrintStream out) {
        out.println("<html><head>");
        out.println("    <title> Sphinx-4 Configuration</title");
        out.println("</head>");
        out.println("<body>");
    }


    /**
     * Dumps the given component as HTML to the given stream
     *
     * @param out  where to dump the HTML
     * @param name the name of the component to dump
     */
    public static void dumpComponentAsHTML(PrintStream out, String name, PropertySheet properties) {
        out.println("<table border=1>");
        //        out.println("<table border=1 width=\"%80\">");
        out.print("    <tr><th bgcolor=\"#CCCCFF\" colspan=2>");
        //       out.print("<a href="")
        out.print(name);
        out.print("</a>");
        out.println("</td></tr>");

        out.println("    <tr><th bgcolor=\"#CCCCFF\">Property</th><th bgcolor=\"#CCCCFF\"> Value</th></tr>");
        Collection<String> propertyNames = properties.getRegisteredProperties();

        for (String propertyName : propertyNames) {
            out.print("    <tr><th align=\"leftt\">" + propertyName + "</th>");
            Object obj;
            obj = properties.getRaw(propertyName);
            if (obj instanceof String) {
                out.println("<td>" + obj + "</td></tr>");
            } else if (obj instanceof List<?>) {
                List<?> l = (List<?>) obj;
                out.println("    <td><ul>");
                for (Object listElement : l) {
                    out.println("        <li>" + listElement + "</li>");
                }
                out.println("    </ul></td>");
            } else {
                out.println("<td>DEFAULT</td></tr>");
            }
        }
        out.println("</table><br>");
    }
}

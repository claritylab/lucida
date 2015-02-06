package edu.cmu.sphinx.tools.batch;

import edu.cmu.sphinx.util.props.ConfigurationManager;
import edu.cmu.sphinx.util.props.ConfigurationManagerUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * A little test application which instantiates a sphinx4-system and allows to reconfigure its component-parameters.
 *
 * @author Holger Brandl
 */
public class SphinxShell {

    public static void main(String[] args) throws IOException {
        if (args.length == 0 || (args.length == 1 && (args[0].startsWith("-h") || args[0].startsWith("--h")))) {
            System.out.println("Usage: SphinxShell <config-xml-file> *([[<component>->]<parameter>=<value>] )");
            System.out.println("Example: SphinxShell foobar.xml beamWidth=123 phoneDecoder->autoAllocate=true");

            System.out.println("\nOther options are: ");
            System.out.println(" -h : Prints this help message");
            System.out.println(" -l <config-xml-file> : Prints a list of all component properties");

            System.exit(-1);
        }

        // dump the properties of an xml-configuration
        if (args.length == 2 && args[0].equals("-l")) {
            ConfigurationManagerUtils.dumpPropStructure(new ConfigurationManager(new File(args[1]).toURI().toURL()));
            System.exit(0);
        }

        File configFile = new File(args[0]);
        if (!configFile.isFile())
            throw new FileNotFoundException("Can not open '" + configFile + '\'');

        ConfigurationManager cm = new ConfigurationManager(configFile.toURI().toURL());

        // skip the first argument because it's the filename
        for (int i = 1; i < args.length; i++) {
            String[] splitArg = args[i].split("=");

            assert splitArg.length == 2;

            String propName = splitArg[0];
            String propValue = splitArg[1];
            ConfigurationManagerUtils.setProperty(cm, propName, propValue);
        }
    }
}

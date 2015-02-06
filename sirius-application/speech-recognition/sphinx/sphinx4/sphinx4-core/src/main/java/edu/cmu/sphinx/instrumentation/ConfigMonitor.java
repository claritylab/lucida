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
package edu.cmu.sphinx.instrumentation;


import edu.cmu.sphinx.util.props.*;
import edu.cmu.sphinx.util.props.tools.GDLDumper;
import edu.cmu.sphinx.util.props.tools.HTMLDumper;

import java.io.File;
import java.io.IOException;
import java.util.logging.Logger;

/**
 * Shows the configuration currently in use. This monitor is typically added as a recognition monitor such that the
 * configuration is shown immediately after the recognizer is allocated.
 */
public class ConfigMonitor implements Configurable, Runnable, Monitor {

    /** The property that is used to indicate whether or not this monitor should show the current configuration. */
    @S4Boolean(defaultValue = false)
    public final static String PROP_SHOW_CONFIG = "showConfig";

    /**
     * The property that is used to indicate whether or not this monitor should dump the configuration in an HTML
     * document
     */
    @S4Boolean(defaultValue = false)
    public final static String PROP_SHOW_CONFIG_AS_HTML = "showConfigAsHTML";

    /**
     * The property that is used to indicate whether or not this monitor should dump the configuration in an GDL
     * document
     */
    @S4Boolean(defaultValue = false)
    public final static String PROP_SHOW_CONFIG_AS_GDL = "showConfigAsGDL";

    /**
     * The property that is used to indicate whether or not this monitor should save the configuration in an XML
     * document
     */
    @S4Boolean(defaultValue = false)
    public final static String PROP_SAVE_CONFIG_AS_XML = "saveConfigAsXML";


    @S4String(mandatory = false)
    public static final String PROP_OUTFILE = "file";

    // -------------------------
    // Configuration data
    // -------------------------
    private boolean showConfig;
    private boolean showHTML = true;
    private boolean saveXML;
    private boolean showGDL = true;

    private Logger logger;
    private ConfigurationManager cm;

    private String htmlPath = "config.html";
    private String gdlPath = "config.gdl";
    private String xmlPath = "config.xml";


    /* (non-Javadoc)
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();
        cm = ConfigurationManagerUtils.getPropertyManager(ps);

        showConfig = ps.getBoolean(PROP_SHOW_CONFIG);
        showHTML = ps.getBoolean(PROP_SHOW_CONFIG_AS_HTML);
        showGDL = ps.getBoolean(PROP_SHOW_CONFIG_AS_GDL);
        saveXML = ps.getBoolean(PROP_SAVE_CONFIG_AS_XML);

        if (ps.getString(PROP_OUTFILE) != null) {
            File outFile = new File(ps.getString(PROP_OUTFILE));

            if (outFile.getParentFile().isDirectory()) {
                htmlPath = outFile.getPath();
                gdlPath = outFile.getPath();
                xmlPath = outFile.getPath();
            }
        }
    }


    /* (non-Javadoc)
    * @see java.lang.Runnable#run()
    */
    public void run() {
        if (showConfig) {
            ConfigurationManagerUtils.showConfig(cm);
        }

        if (showHTML) {
            try {
                HTMLDumper.showConfigAsHTML(cm, "foo.html");
            } catch (IOException e) {
                logger.warning("Can't open " + htmlPath + ' ' + e);
            }
        }

        if (showGDL) {
            try {
                GDLDumper.showConfigAsGDL(cm, gdlPath);
            } catch (IOException e) {
                logger.warning("Can't open " + gdlPath + ' ' + e);
            }
        }

        if (saveXML) {
            ConfigurationManagerUtils.save(cm, new File(xmlPath));
        }
    }
}

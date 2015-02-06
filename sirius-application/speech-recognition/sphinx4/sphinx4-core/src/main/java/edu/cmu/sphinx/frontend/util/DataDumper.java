/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.endpoint.SpeechClassifiedData;
import edu.cmu.sphinx.util.props.*;

import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.Locale;


/** Dumps the data */
public class DataDumper extends BaseDataProcessor {

    /** The property that specifies whether data dumping is enabled */
    @S4Boolean(defaultValue = true)
    public final static String PROP_ENABLE = "enable";

    /** The property that specifies the format of the output. */
    @S4String(defaultValue = "0.00000E00;-0.00000E00")
    public final static String PROP_OUTPUT_FORMAT = "outputFormat";

    /** The property that enables the output of signals. */
    @S4Boolean(defaultValue = true)
    public final static String PROP_OUTPUT_SIGNALS = "outputSignals";

    // --------------------------
    // Configuration data
    // --------------------------
    private boolean enable;
    private boolean outputSignals;
    private DecimalFormat formatter;

    public DataDumper( boolean enable, String format, boolean outputSignals ) {
        initLogger();
        this.formatter = new DecimalFormat(format, new DecimalFormatSymbols(Locale.US));
        this.outputSignals = outputSignals;
        this.enable = enable;
    }

    public DataDumper() {

    }
    
    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        
        logger = ps.getLogger();

        enable = ps.getBoolean(PROP_ENABLE);
        String format = ps.getString(PROP_OUTPUT_FORMAT);
        formatter = new DecimalFormat(format, new DecimalFormatSymbols(Locale.US));
        outputSignals = ps.getBoolean(PROP_OUTPUT_SIGNALS);
    }


    /** Constructs a DataDumper */
    @Override
    public void initialize() {
        super.initialize();
    }


    /**
     * Reads and returns the next Data object from this DataProcessor, return null if there is no more audio data.
     *
     * @return the next Data or <code>null</code> if none is available
     * @throws DataProcessingException if there is a data processing error
     */
    @Override
    public Data getData() throws DataProcessingException {
        Data input = getPredecessor().getData();
        if (enable) {
            dumpData(input);
        }
        return input;
    }


    /**
     * Dumps the given input data
     *
     * @param input the data to dump
     */
    private void dumpData(Data input) {
        logger.finer("dumping data...");
        
        if (input instanceof Signal) {
            if (outputSignals) {
                System.out.println("Signal: " + input);
            }
        } else if (input instanceof DoubleData) {
            DoubleData dd = (DoubleData) input;
            double[] values = dd.getValues();
            System.out.print("Frame " + values.length);
            for (double val : values) {
                System.out.print(' ' + formatter.format(val));
            }
            System.out.println();
        } else if (input instanceof SpeechClassifiedData) {
            SpeechClassifiedData dd = (SpeechClassifiedData) input;
            double[] values = dd.getValues();
            System.out.print("Frame ");
            if (dd.isSpeech())
            	System.out.print('*');
            else
            	System.out.print(' ');
            System.out.print(" " + values.length);
            for (double val : values) {
                System.out.print(' ' + formatter.format(val));
            }
            System.out.println();
        } else if (input instanceof FloatData) {
            FloatData fd = (FloatData) input;
            float[] values = fd.getValues();
            System.out.print("Frame " + values.length);
            for (float val : values) {
                System.out.print(' ' + formatter.format(val));
            }
            System.out.println();
        }
    }
}

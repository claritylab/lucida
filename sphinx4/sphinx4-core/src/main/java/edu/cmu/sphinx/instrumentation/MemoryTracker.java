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

import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.recognizer.Recognizer.State;
import edu.cmu.sphinx.recognizer.StateListener;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.decoder.ResultListener;
import edu.cmu.sphinx.util.props.*;

import java.text.DecimalFormat;

/** Monitors a recognizer for memory usage */
public class MemoryTracker
        implements
        ResultListener,
        StateListener,
        Monitor {

    /** The property that defines which recognizer to monitor */
    @S4Component(type = Recognizer.class)
    public final static String PROP_RECOGNIZER = "recognizer";

    /** The property that defines whether summary accuracy information is displayed */
    @S4Boolean(defaultValue = true)
    public final static String PROP_SHOW_SUMMARY = "showSummary";

    /** The property that defines whether detailed accuracy information is displayed */
    @S4Boolean(defaultValue = true)
    public final static String PROP_SHOW_DETAILS = "showDetails";

    private static final DecimalFormat memFormat = new DecimalFormat("0.00 Mb");
    // ------------------------------
    // Configuration data
    // ------------------------------
    private String name;
    private Recognizer recognizer;
    private boolean showSummary;
    private boolean showDetails;
    private float maxMemoryUsed;
    private int numMemoryStats;
    private float avgMemoryUsed;

    public MemoryTracker(Recognizer recognizer, boolean showSummary, boolean showDetails) {
        initRecognizer(recognizer);
        this.showSummary = showSummary;
        this.showDetails = showDetails;
    }

    public MemoryTracker() {
        
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        Recognizer newRecognizer = (Recognizer) ps.getComponent(
                PROP_RECOGNIZER);
        initRecognizer(newRecognizer);
        showSummary = ps.getBoolean(PROP_SHOW_SUMMARY);
        showDetails = ps.getBoolean(PROP_SHOW_DETAILS);
    }

    private void initRecognizer(Recognizer newRecognizer) {
        if (recognizer == null) {
            recognizer = newRecognizer;
            recognizer.addResultListener(this);
            recognizer.addStateListener(this);
        } else if (recognizer != newRecognizer) {
            recognizer.removeResultListener(this);
            recognizer.removeStateListener(this);
            recognizer = newRecognizer;
            recognizer.addResultListener(this);
            recognizer.addStateListener(this);
        }
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#getName()
    */
    public String getName() {
        return name;
    }


    /** Shows memory usage
     * @param show*/
    private void calculateMemoryUsage(boolean show) {
        float totalMem = Runtime.getRuntime().totalMemory()
                / (1024.0f * 1024.0f);
        float freeMem = Runtime.getRuntime().freeMemory() / (1024.0f * 1024.0f);
        float usedMem = totalMem - freeMem;
        if (usedMem > maxMemoryUsed) {
            maxMemoryUsed = usedMem;
        }

        numMemoryStats++;
        avgMemoryUsed = ((avgMemoryUsed * (numMemoryStats - 1)) + usedMem)
                / numMemoryStats;

        if (show) {
            System.out.println("   Mem  Total: " + memFormat.format(totalMem)
                    + "  " + "Free: " + memFormat.format(freeMem));
            System.out.println("   Used: This: " + memFormat.format(usedMem) + "  "
                    + "Avg: " + memFormat.format(avgMemoryUsed) + "  " + "Max: "
                    + memFormat.format(maxMemoryUsed));
        }
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.decoder.ResultListener#newResult(edu.cmu.sphinx.result.Result)
    */
    public void newResult(Result result) {
        if (result.isFinal()) {
            calculateMemoryUsage(showDetails);
        }
    }

    public void statusChanged(Recognizer.State status) {
        if (status == State.DEALLOCATED) {
            calculateMemoryUsage(showSummary);
        }
    }
}

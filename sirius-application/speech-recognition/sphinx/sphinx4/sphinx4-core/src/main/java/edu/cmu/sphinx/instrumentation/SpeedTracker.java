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

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.recognizer.Recognizer.State;
import edu.cmu.sphinx.recognizer.StateListener;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.decoder.ResultListener;
import edu.cmu.sphinx.util.TimerPool;
import edu.cmu.sphinx.util.props.*;

import java.text.DecimalFormat;

/** Monitors a recognizer for speed */
public class SpeedTracker
        implements
        ResultListener,
        Resetable,
        StateListener,
        SignalListener,
        Monitor {

    /** The property that defines which recognizer to monitor */
    @S4Component(type = Recognizer.class)
    public final static String PROP_RECOGNIZER = "recognizer";
    /** The property that defines which frontend to monitor */
    @S4Component(type = FrontEnd.class)
    public final static String PROP_FRONTEND = "frontend";
    /** The property that defines whether summary accuracy information is displayed */
    @S4Boolean(defaultValue = true)
    public final static String PROP_SHOW_SUMMARY = "showSummary";

    /** The property that defines whether detailed accuracy information is displayed */
    @S4Boolean(defaultValue = true)
    public final static String PROP_SHOW_DETAILS = "showDetails";

    /** The property that defines whether detailed response information is displayed */
    @S4Boolean(defaultValue = false)
    public final static String PROP_SHOW_RESPONSE_TIME = "showResponseTime";

    /** The property that defines whether detailed timer information is displayed */
    @S4Boolean(defaultValue = false)
    public final static String PROP_SHOW_TIMERS = "showTimers";

    private static final DecimalFormat timeFormat = new DecimalFormat("0.00");


    // ------------------------------
    // Configuration data
    // ------------------------------
    private String name;
    private Recognizer recognizer;
    private FrontEnd frontEnd;
    private boolean showSummary;
    private boolean showDetails;
    private boolean showTimers;
    private long startTime;
    private float audioTime;
    private float processingTime;
    private float totalAudioTime;
    private float totalProcessingTime;

    private boolean showResponseTime;
    private int numUtteranceStart;
    private long maxResponseTime = Long.MIN_VALUE;
    private long minResponseTime = Long.MAX_VALUE;
    private long totalResponseTime;


    public SpeedTracker(Recognizer recognizer, FrontEnd frontEnd, boolean showSummary, boolean showDetails, boolean showResponseTime, boolean showTimers) {
        initRecognizer(recognizer);
        initFrontEnd(frontEnd);
        this.showSummary = showSummary;
        this.showDetails = showDetails;
        this.showResponseTime = showResponseTime;
        this.showTimers = showTimers;
    }

    public SpeedTracker() {
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        initRecognizer((Recognizer) ps.getComponent(PROP_RECOGNIZER));
        initFrontEnd((FrontEnd) ps.getComponent(PROP_FRONTEND));
        showSummary = ps.getBoolean(PROP_SHOW_SUMMARY);
        showDetails = ps.getBoolean(PROP_SHOW_DETAILS);
        showResponseTime = ps.getBoolean(PROP_SHOW_RESPONSE_TIME);
        showTimers = ps.getBoolean(PROP_SHOW_TIMERS);
    }

    private void initFrontEnd(FrontEnd newFrontEnd) {
        if (frontEnd == null) {
            frontEnd = newFrontEnd;
            frontEnd.addSignalListener(this);
        } else if (frontEnd != newFrontEnd) {
            frontEnd.removeSignalListener(this);
            frontEnd = newFrontEnd;
            frontEnd.addSignalListener(this);
        }
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


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.decoder.ResultListener#newResult(edu.cmu.sphinx.result.Result)
    */
    public void newResult(Result result) {
        if (result.isFinal()) {
            processingTime = (getTime() - startTime) / 1000.0f;
            totalAudioTime += audioTime;
            totalProcessingTime += processingTime;
            if (showDetails) {
                showAudioUsage();
            }
        }
    }


    /** Shows the audio usage data */
    protected void showAudioUsage() {
        System.out.print("   This  Time Audio: " + timeFormat.format(audioTime)
                + 's');
        System.out.print("  Proc: " + timeFormat.format(processingTime) + 's');
        System.out.println("  Speed: " + timeFormat.format(getSpeed())
                + " X real time");
        showAudioSummary();
    }


    /** Shows the audio summary data */
    protected void showAudioSummary() {
        System.out.print("   Total Time Audio: "
                + timeFormat.format(totalAudioTime) + 's');
        System.out.print("  Proc: " + timeFormat.format(totalProcessingTime)
                + 's');
        System.out.println("  Speed: "
                + timeFormat.format(getCumulativeSpeed()) + " X real time");

        if (showResponseTime) {
            float avgResponseTime =
                    (float) totalResponseTime / (numUtteranceStart * 1000);
            System.out.println
                    ("   Response Time:  Avg: " + avgResponseTime + 's' +
                            "  Max: " + ((float) maxResponseTime / 1000) +
                            "s  Min: " + ((float) minResponseTime / 1000) + 's');
        }
    }


    /**
     * Returns the speed of the last decoding as a fraction of real time.
     *
     * @return the speed of the last decoding
     */
    public float getSpeed() {
        if (processingTime == 0 || audioTime == 0) {
            return 0;
        } else {
            return (processingTime / audioTime);
        }
    }


    /** Resets the speed statistics */
    public void reset() {
        totalProcessingTime = 0;
        totalAudioTime = 0;
        numUtteranceStart = 0;
    }


    /**
     * Returns the cumulative speed of this decoder as a fraction of real time.
     *
     * @return the cumulative speed of this decoder
     */
    public float getCumulativeSpeed() {
        if (totalProcessingTime == 0 || totalAudioTime == 0) {
            return 0;
        } else {
            return (totalProcessingTime / totalAudioTime);
        }
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.frontend.SignalListener#signalOccurred(edu.cmu.sphinx.frontend.Signal)
    */
    public void signalOccurred(Signal signal) {
        if (signal instanceof DataStartSignal) {
            startTime = getTime();
            long responseTime = (System.currentTimeMillis() - signal.getTime());
            totalResponseTime += responseTime;
            if (responseTime > maxResponseTime) {
                maxResponseTime = responseTime;
            }
            if (responseTime < minResponseTime) {
                minResponseTime = responseTime;
            }
            numUtteranceStart++;
        } else if (signal instanceof DataEndSignal) {
            DataEndSignal endSignal = (DataEndSignal) signal;
            audioTime = endSignal.getDuration() / 1000f;
        }
    }


    /**
     * Returns the current time in milliseconds
     *
     * @return the time in milliseconds.
     */
    private long getTime() {
        return System.currentTimeMillis();
    }


    public void statusChanged(Recognizer.State status) {
        if (status == State.ALLOCATED) {
            if (showTimers) {
                TimerPool.dumpAll();
            }
        }

        if (status == State.DEALLOCATING) {
            if (showTimers) {
                TimerPool.dumpAll();
            }
        }

        if (status == State.DEALLOCATED) {
            if (showSummary) {
                showAudioSummary();
            }

        }
    }
}

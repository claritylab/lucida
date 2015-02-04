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
package edu.cmu.sphinx.recognizer;

import edu.cmu.sphinx.decoder.Decoder;
import edu.cmu.sphinx.decoder.ResultProducer;
import edu.cmu.sphinx.decoder.ResultListener;
import edu.cmu.sphinx.instrumentation.Monitor;
import edu.cmu.sphinx.instrumentation.Resetable;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.util.props.*;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * The Sphinx-4 recognizer. This is the main entry point for Sphinx-4. Typical usage of a recognizer is like so:
 * <p/>
 * <pre><code>
 *  public void recognizeDigits() {
 *      URL digitsConfig = new URL("file:./digits.xml");
 *      ConfigurationManager cm = new ConfigurationManager(digitsConfig);
 *      Recognizer sphinxDigitsRecognizer
 *          = (Recognizer) cm.lookup("digitsRecognizer");
 *      boolean done = false;
 *      Result result;
 * <p/>
 *      sphinxDigitsRecognizer.allocate();
 * <p/>
 *     // echo spoken digits, quit when 'nine' is spoken
 * <p/>
 *      while (!done) {
 *           result = sphinxDigitsRecognizer.recognize();
 *           System.out.println(&quot;Result: &quot; + result);
 *           done = result.toString().equals(&quot;nine&quot;);
 *      }
 * <p/>
 *      sphinxDigitsRecognizer.deallocate();
 *   }
 * </code></pre>
 * <p/>
 * Note that some Recognizer methods may throw an IllegalStateException if the recognizer is not in the proper state
 */
public class Recognizer implements Configurable, ResultProducer {

    /** The property for the decoder to be used by this recognizer. */
    @S4Component(type = Decoder.class)
    public final static String PROP_DECODER = "decoder";

    /** The property for the set of monitors for this recognizer */
    @S4ComponentList(type = Monitor.class)
    public final static String PROP_MONITORS = "monitors";

    /** Defines the possible states of the recognizer. */
    public static enum State { DEALLOCATED, ALLOCATING, ALLOCATED, READY, RECOGNIZING, DEALLOCATING, ERROR }

    private String name;
    private Decoder decoder;
    private State currentState = State.DEALLOCATED;

    private final List<StateListener> stateListeners = Collections.synchronizedList(new ArrayList<StateListener>());
    private List<Monitor> monitors;


    public Recognizer(Decoder decoder, List<Monitor> monitors) {
        this.decoder = decoder;
        this.monitors = monitors;
        name = null;
    }

    public Recognizer() {
    }

    /* (non-Javadoc)
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        decoder = (Decoder) ps.getComponent(PROP_DECODER);
        monitors = ps.getComponentList(PROP_MONITORS, Monitor.class);

        name = ps.getInstanceName();
    }


    /**
     * Performs recognition for the given number of input frames, or until a 'final' result is generated. This method
     * should only be called when the recognizer is in the <code>allocated</code> state.
     *
     * @param referenceText what was actually spoken
     * @return a recognition result
     * @throws IllegalStateException if the recognizer is not in the <code>ALLOCATED</code> state
     */
    public Result recognize(String referenceText) throws IllegalStateException {
        Result result = null;
        checkState(State.READY);
        try {
            setState(State.RECOGNIZING);
            result = decoder.decode(referenceText);
        } finally {
            setState(State.READY);
        }
        return result;
    }


    /**
     * Performs recognition for the given number of input frames, or until a 'final' result is generated. This method
     * should only be called when the recognizer is in the <code>allocated</code> state.
     *
     * @return a recognition result
     * @throws IllegalStateException if the recognizer is not in the <code>ALLOCATED</code> state
     */
    public Result recognize() throws IllegalStateException {
        return recognize(null);
    }


    /**
     * Checks to ensure that the recognizer is in the given state.
     *
     * @param desiredState the state that the recognizer should be in
     * @throws IllegalStateException if the recognizer is not in the desired state.
     */
    private void checkState(State desiredState) {
        if (currentState != desiredState) {
            throw new IllegalStateException("Expected state " + desiredState
                    + " actual state " + currentState);
        }
    }


    /**
     * sets the current state
     *
     * @param newState the new state
     */
    private void setState(State newState) {
        currentState = newState;
        synchronized (stateListeners) {
            for (StateListener sl : stateListeners) {
                sl.statusChanged(currentState);
            }
        }
    }


    /**
     * Allocate the resources needed for the recognizer. Note this method make take some time to complete. This method
     * should only be called when the recognizer is in the <code> deallocated </code> state.
     *
     * @throws IllegalStateException if the recognizer is not in the <code>DEALLOCATED</code> state
     */
    public void allocate() throws IllegalStateException {
        checkState(State.DEALLOCATED);
        setState(State.ALLOCATING);
        decoder.allocate();
        setState(State.ALLOCATED);
        setState(State.READY);
    }


    /**
     * Deallocates the recognizer. This method should only be called if the recognizer is in the <code> allocated
     * </code> state.
     *
     * @throws IllegalStateException if the recognizer is not in the <code>ALLOCATED</code> state
     */
    public void deallocate() throws IllegalStateException {
        checkState(State.READY);
        setState(State.DEALLOCATING);
        decoder.deallocate();
        setState(State.DEALLOCATED);
    }


    /**
     * Retrieves the recognizer state. This method can be called in any state.
     *
     * @return the recognizer state
     */
    public State getState() {
        return currentState;
    }


    /** Resets the monitors monitoring this recognizer */
    public void resetMonitors() {
        for (Monitor listener : monitors) {
            if (listener instanceof Resetable)
              ((Resetable)listener).reset();
        }
    }


    /**
     * Adds a result listener to this recognizer. A result listener is called whenever a new result is generated by the
     * recognizer. This method can be called in any state.
     *
     * @param resultListener the listener to add
     */
    public void addResultListener(ResultListener resultListener) {
        decoder.addResultListener(resultListener);
    }


    /**
     * Adds a status listener to this recognizer. The status listener is called whenever the status of the recognizer
     * changes. This method can be called in any state.
     *
     * @param stateListener the listener to add
     */
    public void addStateListener(StateListener stateListener) {
        stateListeners.add(stateListener);
    }


    /**
     * Removes a previously added result listener. This method can be called in any state.
     *
     * @param resultListener the listener to remove
     */
    public void removeResultListener(ResultListener resultListener) {
        decoder.removeResultListener(resultListener);
    }


    /**
     * Removes a previously added state listener. This method can be called in any state.
     *
     * @param stateListener the state listener to remove
     */
    public void removeStateListener(StateListener stateListener) {
        stateListeners.remove(stateListener);
    }


    /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
    @Override
    public String toString() {
        return "Recognizer: " + name + " State: " + currentState;
    }

}

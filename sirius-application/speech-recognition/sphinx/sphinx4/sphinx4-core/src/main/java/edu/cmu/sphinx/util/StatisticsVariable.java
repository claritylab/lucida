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

package edu.cmu.sphinx.util;

import java.util.HashMap;
import java.util.Map;

/**
 * Represents a named value. A StatisticsVariable may be used to track data in a fashion that will allow the data to be
 * viewed or dumped at any time.  Statistics are kept in a pool and are grouped in contexts. Statistics can be dumped
 * as a whole or by context.
 */
public class StatisticsVariable {

    private static final Map<String, StatisticsVariable> pool = new HashMap<String, StatisticsVariable>();

    /** the value of this StatisticsVariable. It can be manipulated directly by the application. */
    public double value;

    private final String name;        // the name of this value
    private boolean enabled;        // if true this var is enabled


    /**
     * Gets the StatisticsVariable with the given name from the given context. If the statistic does not currently
     * exist, it is created. If the context does not currently exist, it is created.
     *
     * @param statName the name of the StatisticsVariable
     * @return the StatisticsVariable with the given name and context
     */
    static public StatisticsVariable getStatisticsVariable(String statName) {

        StatisticsVariable stat = pool.get(statName);
        if (stat == null) {
            stat = new StatisticsVariable(statName);
            pool.put(statName, stat);
        }
        return stat;
    }


    /**
     * Gets the StatisticsVariable with the given name for the given instance and context. This is a convenience
     * function.
     *
     * @param instanceName the instance name of creator
     * @param statName     the name of the StatisticsVariable
     */
    static public StatisticsVariable getStatisticsVariable(
            String instanceName, String statName) {
        return getStatisticsVariable(instanceName + '.' + statName);
    }


    /** Dump all of the StatisticsVariable in the given context */
    static public void dumpAll() {
        System.out.println(" ========= statistics  " + "=======");
        for (StatisticsVariable stats : pool.values()) {
            stats.dump();
        }
    }


    /** Resets all of the StatisticsVariables in the given context */
    static public void resetAll() {
        for (StatisticsVariable stats : pool.values()) {
            stats.reset();
        }
    }


    /**
     * Contructs a StatisticsVariable with the given name and context
     *
     * @param statName the name of this StatisticsVariable
     */
    private StatisticsVariable(String statName) {
        this.name = statName;
        this.value = 0.0;
    }


    /**
     * Retrieves the name of this StatisticsVariable
     *
     * @return the name of this StatisticsVariable
     */
    public String getName() {
        return name;
    }


    /**
     * Retrieves the value for this StatisticsVariable
     *
     * @return the current value for this StatisticsVariable
     */
    public double getValue() {
        return value;
    }


    /**
     * Sets the value for this StatisticsVariable
     *
     * @param value the new value
     */
    public void setValue(double value) {
        this.value = value;
    }


    /** Resets this StatisticsVariable. The value is set to zero. */
    public void reset() {
        setValue(0.0);
    }


    /** Dumps this StatisticsVariable. */
    public void dump() {
        if (isEnabled()) {
            System.out.println(name + ' ' + value);
        }
    }


    /**
     * Determines if this StatisticsVariable is enabled
     *
     * @return true if enabled
     */
    public boolean isEnabled() {
        return enabled;
    }


    /**
     * Sets the enabled state of this StatisticsVariable
     *
     * @param enabled the new enabled state
     */
    public void setEnabled(boolean enabled) {
        this.enabled = enabled;
    }


    /** Some simple tests for the StatisticsVariable */
    public static void main(String[] args) {
        StatisticsVariable loops =
                StatisticsVariable.getStatisticsVariable("main", "loops");
        StatisticsVariable sum =
                StatisticsVariable.getStatisticsVariable("main", "sum");

        StatisticsVariable foot =
                StatisticsVariable.getStatisticsVariable("body", "foot");
        StatisticsVariable leg =
                StatisticsVariable.getStatisticsVariable("body", "leg");
        StatisticsVariable finger =
                StatisticsVariable.getStatisticsVariable("body", "finger");

        foot.setValue(2);
        leg.setValue(2);
        finger.setValue(10);

        StatisticsVariable.dumpAll();
        StatisticsVariable.dumpAll();

        for (int i = 0; i < 1000; i++) {
            loops.value++;
            sum.value += i;
        }

        StatisticsVariable.dumpAll();


        StatisticsVariable loopsAlias =
                StatisticsVariable.getStatisticsVariable("main", "loops");
        StatisticsVariable sumAlias =
                StatisticsVariable.getStatisticsVariable("main", "sum");

        for (int i = 0; i < 1000; i++) {
            loopsAlias.value++;
            sumAlias.value += i;
        }

        StatisticsVariable.dumpAll();
        StatisticsVariable.resetAll();
        StatisticsVariable.dumpAll();
    }
}

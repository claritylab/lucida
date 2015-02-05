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

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DoubleData;
import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Integer;

import java.util.Arrays;


/**
 * Plots positive energy values of a cepstrum to stdout. The energy value is assumed to be the first element of the
 * double array returned by <code>Data.getValues()</code>. For negative energy value, a "-" will be printed out. The
 * plots look like the following, one line per cepstrum. The energy value for that particular cepstrum is printed at the
 * end of the line.
 * <p/>
 * <code> <br>+......7 <br>+......7 <br>Cepstrum: SPEECH_START <br>+......7 <br>+.......8 <br>+......7 <br>+.......8
 * <br>+.......8 <br>+........9 <br>+............14 <br>+...........13 <br>+...........13 <br>+...........13
 * <br>+.............15 <br>+.............15 <br>+..............16 <br>+..............16 <br>+..............16
 * <br>+.............15 <br>+............14 <br>+............14 <br>+............14 <br>+............14
 * <br>+.............15 <br>+..............16 <br>+...............17 <br>+...............17 <br>+...............17
 * <br>+...............17 <br>+...............17 <br>+...............17 <br>+..............16 <br>+.............15
 * <br>+............14 <br>+............14 <br>+............14 <br>+...........13 <br>+........9 <br>+.......8
 * <br>+......7 <br>+......7 <br>+......7 <br>Cepstrum: SPEECH_END <br>+......7 </code>
 */
public class EnergyPlotter implements Configurable {

    /** The maximum level of energy for which a plot string will be preconstructed. */
    @S4Integer(defaultValue = 20)
    public static final String PROP_MAX_ENERGY = "maxEnergy";

    private int maxEnergy;
    private String[] plots;

    public EnergyPlotter(int maxEnergy) {
        this.maxEnergy = maxEnergy;
        buildPlots(maxEnergy);
    }

    public EnergyPlotter() {

    }
    
    public void newProperties(PropertySheet ps) throws PropertyException {
        maxEnergy = ps.getInt(PROP_MAX_ENERGY);
        buildPlots(maxEnergy);
    }

    /**
     * Builds the strings for the plots.
     *
     * @param maxEnergy the maximum energy value
     */
    private void buildPlots(int maxEnergy) {
        plots = new String[maxEnergy + 1];
        for (int i = 0; i < maxEnergy + 1; i++) {
            plots[i] = getPlotString(i);
        }
    }


    /**
     * Returns the plot string for the given energy.
     *
     * @param energy the energy level
     */
    private String getPlotString(int energy) {
        char[] plot = new char[energy];
        Arrays.fill(plot, '.');
        if (energy > 0) {
            if (energy < 10) {
                plot[plot.length - 1] = (char) ('0' + energy);
            } else {
                plot[plot.length - 2] = '1';
                plot[plot.length - 1] = (char) ('0' + (energy - 10));
            }
        }
        return ('+' + new String(plot));
    }


    /**
     * Plots the energy values of the given Data to System.out. If the Data contains a signal, it prints the signal.
     *
     * @param cepstrum the Data to plot
     */
    public void plot(Data cepstrum) {
        if (cepstrum != null) {
            if (cepstrum instanceof DoubleData) {
                int energy = (int) ((DoubleData) cepstrum).getValues()[0];
                System.out.println(getPlot(energy));
            } else {
                System.out.println(cepstrum);
            }
        }
    }


    /**
     * Returns the corresponding plot String for the given energy value. The energy value should be positive or zero. If
     * its negative, It will output the string "-".
     *
     * @param energy
     * @return energy the energy value
     */
    private String getPlot(int energy) {
        if (energy < 0) {
            return "-";
        } else if (energy <= maxEnergy) {
            return plots[energy];
        } else {
            return getPlotString(energy);
        }
    }
}

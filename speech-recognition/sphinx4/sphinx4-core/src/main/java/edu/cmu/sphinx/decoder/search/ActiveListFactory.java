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
package edu.cmu.sphinx.decoder.search;

import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.*;

/** Creates new active lists. */
public abstract class ActiveListFactory implements Configurable {


    /**
     * property that sets the desired (or target) size for this active list.  This is sometimes referred to as the beam
     * size
     */
    @S4Integer(defaultValue = -1)
    public final static String PROP_ABSOLUTE_BEAM_WIDTH = "absoluteBeamWidth";

    /**
     * Property that sets the minimum score relative to the maximum score in the list for pruning.  Tokens with a score
     * less than relativeBeamWidth * maximumScore will be pruned from the list
     */
    @S4Double(defaultValue = 1E-80)
    public final static String PROP_RELATIVE_BEAM_WIDTH = "relativeBeamWidth";

    /**
     * Property that indicates whether or not the active list will implement 'strict pruning'.  When strict pruning is
     * enabled, the active list will not remove tokens from the active list until they have been completely scored.  If
     * strict pruning is not enabled, tokens can be removed from the active list based upon their entry scores. The
     * default setting is false (disabled).
     */
    @S4Boolean(defaultValue = true)
    public final static String PROP_STRICT_PRUNING = "strictPruning";

    protected LogMath logMath;
    protected int absoluteBeamWidth;
    protected float logRelativeBeamWidth;

    /**
     * 
     * @param absoluteBeamWidth
     * @param relativeBeamWidth
     */
    public ActiveListFactory(int absoluteBeamWidth,double relativeBeamWidth){
        logMath = LogMath.getInstance();
        this.absoluteBeamWidth = absoluteBeamWidth;
        this.logRelativeBeamWidth = logMath.linearToLog(relativeBeamWidth);      
    }

    public ActiveListFactory() {
    }


    public void newProperties(PropertySheet ps) throws PropertyException {
        logMath = LogMath.getInstance();
        absoluteBeamWidth = ps.getInt(PROP_ABSOLUTE_BEAM_WIDTH);
        double relativeBeamWidth = ps.getDouble(PROP_RELATIVE_BEAM_WIDTH);

        logRelativeBeamWidth = logMath.linearToLog(relativeBeamWidth);
    }


    /**
     * Creates a new active list of a particular type
     *
     * @return the active list
     */
    public abstract ActiveList newInstance();
}

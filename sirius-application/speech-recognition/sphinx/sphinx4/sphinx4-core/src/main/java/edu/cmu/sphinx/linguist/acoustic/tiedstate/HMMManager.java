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

package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import edu.cmu.sphinx.linguist.acoustic.HMM;
import edu.cmu.sphinx.linguist.acoustic.HMMPosition;
import edu.cmu.sphinx.linguist.acoustic.Unit;

import java.util.*;
import java.util.logging.Logger;


/**
 * Manages HMMs. This HMMManager groups {@link edu.cmu.sphinx.linguist.acoustic.HMM HMMs} together by their {@link
 * edu.cmu.sphinx.linguist.acoustic.HMMPosition position} with the word.
 */
public class HMMManager implements Iterable<HMM> {

    private final List<HMM> allHMMs = new ArrayList<HMM>();
    private final Map<HMMPosition, Map<Unit, HMM>> hmmsPerPosition = new EnumMap<HMMPosition, Map<Unit, HMM>>(HMMPosition.class);

    public HMMManager () {
        for (HMMPosition pos : HMMPosition.values())
            hmmsPerPosition.put(pos, new HashMap<Unit, HMM>());
    }

    /**
     * Put an HMM into this manager
     *
     * @param hmm the hmm to manage
     */
    public void put(HMM hmm) {
        hmmsPerPosition.get(hmm.getPosition()).put(hmm.getUnit(), hmm);
        allHMMs.add(hmm);
    }


    /**
     * Retrieves an HMM by position and unit
     *
     * @param position the position of the HMM
     * @param unit     the unit that this HMM represents
     * @return the HMM for the unit at the given position or null if no HMM at the position could be found
     */
    public HMM get(HMMPosition position, Unit unit) {
        return hmmsPerPosition.get(position).get(unit);
    }


    /**
     * Gets an iterator that iterates through all HMMs
     *
     * @return an iterator that iterates through all HMMs
     */
    public Iterator<HMM> iterator() {
        return allHMMs.iterator();
    }


    /**
     * Returns the number of HMMS in this manager
     *
     * @return the number of HMMs
     */
    private int getNumHMMs() {
        int count = 0;

        for (Map<Unit, HMM> map : hmmsPerPosition.values()) {
            if (map != null) {
                count += map.size();
            }
        }
        return count;
    }


    /**
     * Log information about this manager
     *
     * @param logger logger to use for this logInfo
     */
    public void logInfo(Logger logger) {
        logger.info("HMM Manager: " + getNumHMMs() + " hmms");
    }
}

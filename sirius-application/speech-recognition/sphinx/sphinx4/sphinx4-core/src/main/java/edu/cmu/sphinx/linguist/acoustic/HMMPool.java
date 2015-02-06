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

package edu.cmu.sphinx.linguist.acoustic;

import edu.cmu.sphinx.util.TimerPool;

import java.util.Iterator;
import java.util.Map;
import java.util.EnumMap;
import java.util.logging.Level;
import java.util.logging.Logger;


/**
 * The HMMPool provides the ability to manage units via small integer IDs.  Context Independent units and context
 * dependent units can be converted to an ID. IDs can be used to quickly retrieve a unit or an hmm associated with the
 * unit.  This class operates under the constraint that context sizes are exactly one, which is generally only valid for
 * large vocabulary tasks.
 */

public class HMMPool {

    private AcousticModel model;
    private Unit[] unitTable;
    private Map<HMMPosition, HMM[]> hmmTable;
    private int numCIUnits;
    private Logger logger;
    private UnitManager unitManager;


    protected HMMPool(){
    }

    /**
     * Constructs a HMMPool object.
     *
     * @param model the model to use for the pool
     */
    public HMMPool(AcousticModel model, Logger logger, UnitManager unitManager)
    {
        this.logger = logger;
        int maxCIUnits = 0;
        this.model = model;
        this.unitManager = unitManager;
        TimerPool.getTimer(this,"Build HMM Pool").start();

        if (model.getLeftContextSize() != 1)
            throw new Error("LexTreeLinguist: Unsupported left context size");

        if (model.getRightContextSize() != 1)
            throw new Error("LexTreeLinguist: Unsupported right context size");

        // count CI units:
        for (Iterator<Unit> i = model.getContextIndependentUnitIterator(); i.hasNext();) {
            Unit unit = i.next();
            logger.fine("CI unit " + unit);
            if (unit.getBaseID() > maxCIUnits) {
                maxCIUnits = unit.getBaseID();
            }
        }

        numCIUnits = maxCIUnits + 1;

        unitTable = new Unit[numCIUnits * numCIUnits * numCIUnits];

        for (Iterator<HMM> i = model.getHMMIterator(); i.hasNext();) {
            HMM hmm = i.next();
            Unit unit = hmm.getUnit();
            int id = getID(unit);
            unitTable[id] = unit;
            if (logger.isLoggable(Level.FINER)) {
                logger.finer("Unit " + unit + " id " + id);
            }
        }

        // build up the hmm table to allow quick access to the hmms
        hmmTable = new EnumMap<HMMPosition, HMM[]>(HMMPosition.class);
        for (HMMPosition position : HMMPosition.values()) {
            HMM[] hmms = new HMM[unitTable.length];
            hmmTable.put(position, hmms);
            for (int j = 1; j < unitTable.length; j++) {
                Unit unit = unitTable[j];
                if (unit == null) {
                    unit = synthesizeUnit(j);
                }
                if (unit != null) {
                    hmms[j] = model.lookupNearestHMM(unit, position, false);
                    assert hmms[j] != null;
                }
            }
        }
        TimerPool.getTimer(this,"Build HMM Pool").stop();
    }


    public AcousticModel getModel() {
        return model;
    }


    /**
     * Given a unit ID, generate a full context dependent unit that will allow us to look for a suitable hmm
     *
     * @param id the unit id
     * @return a context dependent unit for the ID
     */
    private Unit synthesizeUnit(int id) {
        int centralID = getCentralUnitID(id);
        int leftID = getLeftUnitID(id);
        int rightID = getRightUnitID(id);

        if (centralID == 0 || leftID == 0 || rightID == 0) {
            return null;
        }

        Unit centralUnit = unitTable[centralID];
        Unit leftUnit = unitTable[leftID];
        Unit rightUnit = unitTable[rightID];

        assert centralUnit != null;
        assert leftUnit != null;
        assert rightUnit != null;

        Unit[] lc = new Unit[1];
        Unit[] rc = new Unit[1];
        lc[0] = leftUnit;
        rc[0] = rightUnit;
        LeftRightContext context = LeftRightContext.get(lc, rc);

        Unit unit = unitManager.getUnit(
                centralUnit.getName(), centralUnit.isFiller(),
                context);

        if (logger.isLoggable(Level.FINER)) {
            logger.finer("Missing " + getUnitNameFromID(id)
                    + " returning " + unit);
        }
        return unit;
    }


    /**
     * Returns the number of CI units
     *
     * @return the number of CI Units
     */
    public int getNumCIUnits() {
        return numCIUnits;
    }


    /**
     * Gets the unit for the given id
     *
     * @param unitID the id for the unit
     * @return the unit associated with the ID
     */
    public Unit getUnit(int unitID) {
        return unitTable[unitID];
    }


    /**
     * Given a unit id and a position, return the HMM associated with the
     * unit/position.
     *
     * @param unitID   the id of the unit
     * @param position the position within the word
     * @return the hmm associated with the unit/position
     */
    public HMM getHMM(int unitID, HMMPosition position) {
        return hmmTable.get(position)[unitID];
    }


    /**
     * given a unit return its ID
     *
     * @param unit the unit
     * @return an ID
     */
    public int getID(Unit unit) {
        if (unit.isContextDependent()) {
            LeftRightContext context = (LeftRightContext) unit.getContext();
            assert context.getLeftContext().length == 1;
            assert context.getRightContext().length == 1;
            return buildID(getSimpleUnitID(unit),
                           getSimpleUnitID(context.getLeftContext()[0]),
                           getSimpleUnitID(context.getRightContext()[0]));
        } else {
            return getSimpleUnitID(unit);
        }
    }


    /**
     * Returns a context independent ID
     *
     * @param unit the unit of interest
     * @return the ID of the central unit (ignoring any context)
     */
    private int getSimpleUnitID(Unit unit) {
        return unit.getBaseID();
    }


    public boolean isValidID(int unitID) {
        return unitID >= 0 &&
               unitID < unitTable.length &&
               unitTable[unitID] != null;
    }


    /**
     * Builds an id from the given unit and its left and right unit ids
     *
     * @param unitID  the id of the central unit
     * @param leftID  the id of the left context unit
     * @param rightID the id of the right context unit
     * @return the id for the context dependent unit
     */
    public int buildID(int unitID, int leftID, int rightID) {
        // special case ... if the unitID is associated with
        // filler than we have no context ... so use the CI
        // form

        if (unitTable[unitID] == null)
            return -1;

        int id;
        if (unitTable[unitID].isFiller()) {
            id = unitID;
        } else {
            id = unitID * (numCIUnits * numCIUnits)
                    + (leftID * numCIUnits)
                    + rightID;
        }

        assert id < unitTable.length;

        return id;
    }


    /**
     * Given a unit id extract the left context unit id
     *
     * @param id the unit id
     * @return the unit id of the left context (0 means no left context)
     */
    private int getLeftUnitID(int id) {
        return (id / numCIUnits) % numCIUnits;
    }


    /**
     * Given a unit id extract the right context unit id
     *
     * @param id the unit id
     * @return the unit id of the right context (0 means no right context)
     */
    private int getRightUnitID(int id) {
        return id % numCIUnits;
    }


    /**
     * Given a unit id extract the central unit id
     *
     * @param id the unit id
     * @return the central unit id
     */
    private int getCentralUnitID(int id) {
        return id / (numCIUnits * numCIUnits);
    }


    /**
     * Given an ID, build up a name for display
     *
     * @return the name baed on the ID
     */
    private String getUnitNameFromID(int id) {
        int centralID = getCentralUnitID(id);
        int leftID = getLeftUnitID(id);
        int rightID = getRightUnitID(id);

        String cs = unitTable[centralID] == null ? "(" + centralID + ')' :
                unitTable[centralID].toString();
        String ls = unitTable[leftID] == null ? ("(" + leftID + ')') :
                unitTable[leftID].toString();
        String rs = unitTable[rightID] == null ? "(" + rightID + ')' :
                unitTable[rightID].toString();

        return cs + '[' + ls + ',' + rs + ']';
    }

    /**
     * Retrieves an HMM for a unit in context. If there is no direct match, the
     * nearest match will be used. Note that we are currently only dealing with,
     * at most, single unit left and right contexts.
     * 
     * @param base
     *            the base CI unit
     * @param lc
     *            the left context
     * @param rc
     *            the right context
     * @param pos
     *            the position of the base unit within the word
     * @return the HMM. (This should never return null)
     */
    public HMM getHMM(Unit base, Unit lc, Unit rc, HMMPosition pos) {
        int id = -1;
        int bid = getID(base);
        int lid = getID(lc);
        int rid = getID(rc);

        if (!isValidID(bid)) {
            logger.severe("Bad HMM Unit: " + base.getName());
            return null;
        }
        if (!isValidID(lid)) {
            logger.severe("Bad HMM Unit: " + lc.getName());
            return null;
        }
        if (!isValidID(rid)) {
            logger.severe("Bad HMM Unit: " + rc.getName());
            return null;
        }
        id = buildID(bid, lid, rid);
        if (id < 0) {
            logger.severe("Unable to build HMM Unit ID for " + base.getName()
                    + " lc=" + lc.getName() + " rc=" + rc.getName());
            return null;
        }
        HMM hmm = getHMM(id, pos);
        if (hmm == null) {
            logger.severe("Missing HMM Unit for " + base.getName() + " lc="
                    + lc.getName() + " rc=" + rc.getName());
        }

        return hmm;
    }

    

    /** Dumps out info about this pool */
    public void dumpInfo() {
        logger.info("Max CI Units " + numCIUnits);
        logger.info("Unit table size " + unitTable.length);

        if (logger.isLoggable(Level.FINER)) {
            for (int i = 0; i < unitTable.length; i++) {
                logger.finer(String.valueOf(i) + ' ' + unitTable[i]);
            }
        }
    }


    /**
     * A quick and dirty benchmark to get an idea how long the HMM lookups will take.  This experiment shows that on a
     * 1GHZ sparc system, the lookup takes a little less than 1uSec.  This is probably fast enough.
     */

    static final HMMPosition[] pos = {
            HMMPosition.BEGIN, HMMPosition.END, HMMPosition.SINGLE,
            HMMPosition.INTERNAL};    
    
    static final int[] ids = {9206, 9320, 9620, 9865, 14831, 15836};

    void benchmark() {
        int nullCount = 0;
        System.out.println("benchmarking ...");
        TimerPool.getTimer(this,"hmmPoolBenchmark").start();

        for (int i = 0; i < 1000000; i++) {
            int id = ids[i % ids.length];
            HMMPosition position = pos[i % pos.length];
            HMM hmm = getHMM(id, position);
            if (hmm == null) {
                nullCount++;
            }
        }
        TimerPool.getTimer(this,"hmmPoolBenchmark").stop();
        System.out.println("null count " + nullCount);
    }
}


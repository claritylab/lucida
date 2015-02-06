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
package edu.cmu.sphinx.linguist.util;

import edu.cmu.sphinx.linguist.Linguist;
import edu.cmu.sphinx.linguist.SearchState;
import edu.cmu.sphinx.linguist.SearchStateArc;
import edu.cmu.sphinx.util.Timer;
import edu.cmu.sphinx.util.TimerPool;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

/** Gets successors from a linguist and times them */
public class LinguistTimer {

    private final Linguist linguist;
    private final boolean details;
    int totalStates;
    int totalEmittingStates;
    int totalNonEmittingStates;
    int totalFinalStates;
    int maxSuccessors;


    /**
     * Creats a LinguistTimer
     *
     * @param linguist the linguist to time
     * @param details  if true print out details
     */
    public LinguistTimer(Linguist linguist, boolean details) {
        this.linguist = linguist;
        this.details = details;
    }
    /**
     * tests the linguist
     */
    /**
     * Times the lingust
     *
     * @param numRuns   the number of simulated runs
     * @param numFrames the number of simulated frames
     * @param maxBeam   the size of the beam
     */
    public void timeLinguist(int numRuns, int numFrames, int maxBeam) {
        // this test invokes the linguist using access patterns that
        // are similar to a real search. It allows for timing and
        // profiling of the linguist, independent of the search
        // or scoring
        Random random = new Random(1000);
        Timer frameTimer = TimerPool.getTimer(this, "frameTimer");
        Timer totalTimer = TimerPool.getTimer(this, "totalTimer");
        // Note: this comparator imposes orderings that are
        // inconsistent with equals.
        System.out.println("TestLinguist: runs " + numRuns + " frames "
                + numFrames + " beam " + maxBeam);
        totalTimer.start();
        for (int runs = 0; runs < numRuns; runs++) {
            int level = 0;
            List<SearchState> activeList = new ArrayList<SearchState>();
            activeList.add(linguist.getSearchGraph().getInitialState());
            linguist.startRecognition();
            for (int i = 0; i < numFrames; i++) {
                List<SearchState> oldList = activeList;
                activeList = new ArrayList<SearchState>(maxBeam * 10);
                frameTimer.start();
                for (SearchState nextStates : oldList) {
                    expandState(level, activeList, nextStates);
                }
                frameTimer.stop();
                Collections.shuffle(activeList, random);
                if (activeList.size() > maxBeam) {
                    activeList = activeList.subList(0, maxBeam);
                }
            }
            linguist.stopRecognition();
            frameTimer.dump();
        }
        totalTimer.stop();
        System.out.println(" MaxSuccessors : " + maxSuccessors);
        System.out.println(" TotalStates   : " + totalStates);
        System.out.println(" TotalEmitting : " + totalEmittingStates);
        System.out.println("   NonEmitting : " + totalNonEmittingStates);
        System.out.println("  Final States : " + totalFinalStates);
        TimerPool.dumpAll();
    }


    /**
     * expand the give search state
     *
     * @param level      the nesting level
     * @param activeList where next states are placed
     * @param state      the search state to expand
     */
    private void expandState(int level, List<SearchState> activeList, SearchState state) {
        SearchStateArc[] newStates = state.getSuccessors();
        totalStates++;
        // System.out.println(Utilities.pad(level * 2) + state);
        if (newStates.length > maxSuccessors) {
            maxSuccessors = newStates.length;
        }
        for (SearchStateArc newState : newStates) {
            SearchState ns = newState.getState();
            if (ns.isEmitting()) {
                totalEmittingStates++;
                activeList.add(ns);
            } else if (!ns.isFinal()) {
                totalNonEmittingStates++;
                activeList.add(ns);
                if (details && ns.isFinal()) {
                    System.out.println("result " + ns.toPrettyString());
                }
                expandState(level + 1, activeList, ns);
            } else {
                totalFinalStates++;
            }
            totalStates++;
        }
    }
}

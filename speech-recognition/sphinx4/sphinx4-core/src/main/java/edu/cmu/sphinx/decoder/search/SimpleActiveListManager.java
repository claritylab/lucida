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

package edu.cmu.sphinx.decoder.search;

import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Boolean;
import edu.cmu.sphinx.util.props.S4ComponentList;

import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.logging.Logger;

/**
 * A list of ActiveLists. Different token types are placed in different lists.
 * <p/>
 * This class is not thread safe and should only be used by a single thread.
 */
public class SimpleActiveListManager implements ActiveListManager {

    /**
     * This property is used in the Iterator returned by the getNonEmittingListIterator() method. When the
     * Iterator.next() method is called, this property determines whether the lists prior to that returned by next() are
     * empty (they should be empty). If they are not empty, an Error will be thrown.
     */
    @S4Boolean(defaultValue = false)
    public static final String PROP_CHECK_PRIOR_LISTS_EMPTY = "checkPriorListsEmpty";
    
    /** The property that defines the name of the active list factory to be used by this search manager. */
    @S4ComponentList(type = ActiveListFactory.class)
    public final static String PROP_ACTIVE_LIST_FACTORIES = "activeListFactories";

    // --------------------------------------
    // Configuration data
    // --------------------------------------
    private Logger logger;
    private boolean checkPriorLists;
    private List<ActiveListFactory> activeListFactories;
    private ActiveList[] currentActiveLists;


    /**
     * 
     * @param activeListFactories
     * @param checkPriorLists
     */
    public SimpleActiveListManager(List<ActiveListFactory> activeListFactories, boolean checkPriorLists) {
        this.logger = Logger.getLogger( getClass().getName() );

        this.activeListFactories = activeListFactories;
        this.checkPriorLists = checkPriorLists;
    }

    public SimpleActiveListManager() {
        
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();

        activeListFactories = ps.getComponentList(PROP_ACTIVE_LIST_FACTORIES, ActiveListFactory.class);
        checkPriorLists = ps.getBoolean(PROP_CHECK_PRIOR_LISTS_EMPTY);
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.decoder.search.ActiveListManager#setNumStateOrder(java.lang.Class[])
    */
    public void setNumStateOrder(int numStateOrder) {
        // check to make sure that we have the correct
        // number of active list factories for the given search states
        currentActiveLists = new ActiveList[numStateOrder];

        if (activeListFactories.isEmpty()) {
            logger.severe("No active list factories configured");
            throw new Error("No active list factories configured");
        }
        if (activeListFactories.size() != currentActiveLists.length) {
            logger.warning("Need " + currentActiveLists.length +
                    " active list factories, found " +
                    activeListFactories.size());
        }
        createActiveLists();
    }


    /**
     * Creates the emitting and non-emitting active lists. When creating the non-emitting active lists, we will look at
     * their respective beam widths (eg, word beam, unit beam, state beam).
     */
    private void createActiveLists() {
        int nlists = activeListFactories.size();
        for (int i = 0; i < currentActiveLists.length; i++) {
            int which = i;
            if (which >= nlists) {
                which = nlists - 1;
            }
            ActiveListFactory alf = activeListFactories.get(which);
            currentActiveLists[i] = alf.newInstance();
        }
    }


    /**
     * Adds the given token to the list
     *
     * @param token the token to add
     */
    public void add(Token token) {
        ActiveList activeList = findListFor(token);
        if (activeList == null) {
            throw new Error("Cannot find ActiveList for "
                    + token.getSearchState().getClass());
        }
        activeList.add(token);
    }


    /**
     * Given a token find the active list associated with the token type
     *
     * @param token
     * @return the active list
     */
    private ActiveList findListFor(Token token) {
        return currentActiveLists[token.getSearchState().getOrder()];
    }


    /**
     * Replaces an old token with a new token
     *
     * @param oldToken the token to replace (or null in which case, replace works like add).
     * @param newToken the new token to be placed in the list.
     */
    public void replace(Token oldToken, Token newToken) {
        ActiveList activeList = findListFor(oldToken);
        assert activeList != null;
        activeList.replace(oldToken, newToken);
    }


    /**
     * Returns the emitting ActiveList from the manager
     *
     * @return the emitting ActiveList
     */
    public ActiveList getEmittingList() {
        ActiveList list = currentActiveLists[currentActiveLists.length - 1];
        return list;
    }

    
    /**
     * Clears emitting list in manager
     */
	public void clearEmittingList() {
        ActiveList list = currentActiveLists[currentActiveLists.length - 1];
		currentActiveLists[currentActiveLists.length - 1] = list.newInstance();	
	}

	
    /**
     * Returns an Iterator of all the non-emitting ActiveLists. The iteration order is the same as the search state
     * order.
     *
     * @return an Iterator of non-emitting ActiveLists
     */
    public Iterator<ActiveList> getNonEmittingListIterator() {
        return (new NonEmittingListIterator());
    }


    private class NonEmittingListIterator implements Iterator<ActiveList> {

        private int listPtr;


        public NonEmittingListIterator() {
            listPtr = -1;
        }


        public boolean hasNext() {
            return listPtr + 1 < currentActiveLists.length - 1;
        }


        public ActiveList next() throws NoSuchElementException {
            listPtr++;

            if (listPtr >= currentActiveLists.length) {
                throw new NoSuchElementException();
            }
            if (checkPriorLists) {
                checkPriorLists();
            }
            return currentActiveLists[listPtr];
        }


        /** Check that all lists prior to listPtr is empty. */
        private void checkPriorLists() {
            for (int i = 0; i < listPtr; i++) {
                ActiveList activeList = currentActiveLists[i];
                if (activeList.size() > 0) {
                    throw new Error("At while processing state order"
                            + listPtr + ", state order " + i + " not empty");
                }
            }
        }


        public void remove() {
            currentActiveLists[listPtr] =
                    currentActiveLists[listPtr].newInstance();
        }
    }


    /** Outputs debugging info for this list manager */
    public void dump() {
        System.out.println("--------------------");
        for (ActiveList al : currentActiveLists) {
            dumpList(al);
        }
    }


    /**
     * Dumps out debugging info for the given active list
     *
     * @param al the active list to dump
     */
    private void dumpList(ActiveList al) {
        System.out.println("Size: " + al.size() + " Best token: " + al.getBestToken());
    }

}

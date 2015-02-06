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

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import edu.cmu.sphinx.decoder.scorer.Scoreable;
import edu.cmu.sphinx.linguist.SearchState;
/**
 * The token heap search manager that maintains the heap of best tokens for each
 * search state instead of single one best token
 * 
 */
public class TokenHeapSearchManager extends WordPruningBreadthFirstSearchManager {

    protected final int maxTokenHeapSize = 3;

    Map<Object, TokenHeap> bestTokenMap;

    @Override
    protected void createBestTokenMap() {
        int mapSize = activeList.size() << 2;
        if (mapSize == 0) {
            mapSize = 1;
        }
        bestTokenMap = new HashMap<Object, TokenHeap>(mapSize, 0.3F);
    }

    @Override
    protected void setBestToken(Token token, SearchState state) {
        Object key = getStateKey(state);

        TokenHeap th = bestTokenMap.get(key);
        if (th == null) {
            th = new TokenHeap(maxTokenHeapSize);
            bestTokenMap.put(key, th);
        }
        th.add(token);
    }

    @Override
    protected Token getBestToken(SearchState state) {
        Object key = getStateKey(state);

        // new way... if the heap for this state isn't full return
        // null, otherwise return the worst scoring token
        TokenHeap th = bestTokenMap.get(key);
        Token t;

        if (th == null) {
            return null;
        } else if ((t = th.get(state)) != null) {
            return t;
        } else if (!th.isFull()) {
            return null;
        } else {
            return th.getSmallest();
        }
    }

    /**
     * A quick and dirty token heap that allows us to perform token stack
     * experiments. It is not very efficient. We will likely replace this with
     * something better once we figure out how we want to prune things.
     */

    class TokenHeap {

        final Token[] tokens;
        int curSize;

        /**
         * Creates a token heap with the maximum size
         * 
         * @param maxSize
         *            the maximum size of the heap
         */
        TokenHeap(int maxSize) {
            tokens = new Token[maxSize];
        }

        /**
         * Adds a token to the heap
         * 
         * @param token
         *            the token to add
         */
        void add(Token token) {
            // first, if an identical state exists, replace
            // it.

            if (!tryReplace(token)) {
                if (curSize < tokens.length) {
                    tokens[curSize++] = token;
                } else if (token.getScore() > tokens[curSize - 1].getScore()) {
                    tokens[curSize - 1] = token;
                }
            }
            fixupInsert();
        }

        /**
         * Returns the smallest scoring token on the heap
         * 
         * @return the smallest scoring token
         */
        Token getSmallest() {
            if (curSize == 0) {
                return null;
            } else {
                return tokens[curSize - 1];
            }
        }

        /**
         * Determines if the heap is full
         * 
         * @return <code>true</code> if the heap is full
         */
        boolean isFull() {
            return curSize == tokens.length;
        }

        /**
         * Checks to see if there is already a token t on the heap that has the
         * same search state. If so, this token replaces that one
         * 
         * @param t
         *            the token to try to add to the heap
         * @return <code>true</code> if the token was added
         */
        private boolean tryReplace(Token t) {
            for (int i = 0; i < curSize; i++) {
                if (t.getSearchState().equals(tokens[i].getSearchState())) {
                    assert t.getScore() > tokens[i].getScore();
                    tokens[i] = t;
                    return true;
                }
            }
            return false;
        }

        /** Orders the heap after an insert */
        private void fixupInsert() {
            Arrays.sort(tokens, 0, curSize - 1, Scoreable.COMPARATOR);
        }

        /**
         * returns a token on the heap that matches the given search state
         * 
         * @param s
         *            the search state
         * @return the token that matches, or null
         */
        Token get(SearchState s) {
            for (int i = 0; i < curSize; i++) {
                if (tokens[i].getSearchState().equals(s)) {
                    return tokens[i];
                }
            }
            return null;
        }
    }
}

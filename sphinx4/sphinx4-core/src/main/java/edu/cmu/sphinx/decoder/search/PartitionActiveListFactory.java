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

import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;

import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

/** A factory for PartitionActiveLists */
public class PartitionActiveListFactory extends ActiveListFactory {

    /**
     * 
     * @param absoluteBeamWidth
     * @param relativeBeamWidth
     */
    public PartitionActiveListFactory(int absoluteBeamWidth, double relativeBeamWidth) {
        super(absoluteBeamWidth, relativeBeamWidth);
    }

    public PartitionActiveListFactory() {

    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.decoder.search.ActiveListFactory#newInstance()
    */
    @Override
    public ActiveList newInstance() {
        return new PartitionActiveList(absoluteBeamWidth, logRelativeBeamWidth);
    }


    /**
     * An active list that does absolute beam with pruning by partitioning the
     * token list based on absolute beam width, instead of sorting the token
     * list, and then chopping the list up with the absolute beam width. The
     * expected run time of this partitioning algorithm is O(n), instead of O(n log n) 
     * for merge sort.
     * <p/>
     * This class is not thread safe and should only be used by a single thread.
     * <p/>
     * Note that all scores are maintained in the LogMath log base.
     */
    class PartitionActiveList implements ActiveList {

        private int size;
        private final int absoluteBeamWidth;
        private final float logRelativeBeamWidth;
        private Token bestToken;
        // when the list is changed these things should be
        // changed/updated as well
        private Token[] tokenList;
        private final Partitioner partitioner = new Partitioner();


        /** Creates an empty active list
         * @param absoluteBeamWidth
         * @param logRelativeBeamWidth*/
        public PartitionActiveList(int absoluteBeamWidth,
                                   float logRelativeBeamWidth) {
            this.absoluteBeamWidth = absoluteBeamWidth;
            this.logRelativeBeamWidth = logRelativeBeamWidth;
            int listSize = 2000;
            if (absoluteBeamWidth > 0) {
                listSize = absoluteBeamWidth / 3;
            }
            this.tokenList = new Token[listSize];
        }


        /**
         * Adds the given token to the list
         *
         * @param token the token to add
         */
        public void add(Token token) {
            if (size < tokenList.length) {
                tokenList[size] = token;
                token.setLocation(size);
                size++;
            } else {
                // token array too small, double the capacity
                doubleCapacity();
                add(token);
            }
            if (bestToken == null || token.getScore() > bestToken.getScore()) {
                bestToken = token;
            }
        }


        /** Doubles the capacity of the Token array. */
        private void doubleCapacity() {
            tokenList = Arrays.copyOf(tokenList, tokenList.length * 2);
        }


        /**
         * Replaces an old token with a new token
         *
         * @param oldToken the token to replace (or null in which case, replace works like add).
         * @param newToken the new token to be placed in the list.
         */
        public void replace(Token oldToken, Token newToken) {
            if (oldToken != null) {
                int location = oldToken.getLocation();
                // check to see if the old token is still in the list
                if (location != -1 && tokenList[location] == oldToken) {
                    tokenList[location] = newToken;
                    newToken.setLocation(location);
                    oldToken.setLocation(-1);
                } else {
                    add(newToken);
                }
            } else {
                add(newToken);
            }
            if (bestToken == null || newToken.getScore() > bestToken.getScore()) {
                bestToken = newToken;
            }
        }


        /**
         * Purges excess members. Remove all nodes that fall below the relativeBeamWidth
         *
         * @return a (possible new) active list
         */
        public ActiveList purge() {
            // if the absolute beam is zero, this means there
            // should be no constraint on the abs beam size at all
            // so we will only be relative beam pruning, which means
            // that we don't have to sort the list
            if (absoluteBeamWidth > 0) {
                // if we have an absolute beam, then we will
                // need to sort the tokens to apply the beam
                if (size > absoluteBeamWidth) {
                    size = partitioner.partition(tokenList, size,
                            absoluteBeamWidth) + 1;
                }
            }
            return this;
        }


        /**
         * gets the beam threshold best upon the best scoring token
         *
         * @return the beam threshold
         */
        public float getBeamThreshold() {
            return getBestScore() + logRelativeBeamWidth;
        }


        /**
         * gets the best score in the list
         *
         * @return the best score
         */
        public float getBestScore() {
            float bestScore = -Float.MAX_VALUE;
            if (bestToken != null) {
                bestScore = bestToken.getScore();
            }
            // A sanity check
            // for (Token t : this) {
            //    if (t.getScore() > bestScore) {
            //         System.out.println("GBS: found better score "
            //             + t + " vs. " + bestScore);
            //    }
            // }
            return bestScore;
        }


        /**
         * Sets the best scoring token for this active list
         *
         * @param token the best scoring token
         */
        public void setBestToken(Token token) {
            bestToken = token;
        }


        /**
         * Gets the best scoring token for this active list
         *
         * @return the best scoring token
         */
        public Token getBestToken() {
            return bestToken;
        }


        /**
         * Retrieves the iterator for this tree.
         *
         * @return the iterator for this token list
         */
        public Iterator<Token> iterator() {
            return (new TokenArrayIterator(tokenList, size));
        }


        /**
         * Gets the list of all tokens
         *
         * @return the list of tokens
         */
        public List<Token> getTokens() {
            return Arrays.asList(tokenList).subList(0, size);
        }

        /**
         * Returns the number of tokens on this active list
         *
         * @return the size of the active list
         */
        public final int size() {
            return size;
        }


        /* (non-Javadoc)
        * @see edu.cmu.sphinx.decoder.search.ActiveList#createNew()
        */
        public ActiveList newInstance() {
            return PartitionActiveListFactory.this.newInstance();
        }
    }
}

class TokenArrayIterator implements Iterator<Token> {

    private final Token[] tokenArray;
    private final int size;
    private int pos;


    TokenArrayIterator(Token[] tokenArray, int size) {
        this.tokenArray = tokenArray;
        this.pos = 0;
        this.size = size;
    }


    /** Returns true if the iteration has more tokens. */
    public boolean hasNext() {
        return pos < size;
    }


    /** Returns the next token in the iteration. */
    public Token next() throws NoSuchElementException {
        if (pos >= tokenArray.length) {
            throw new NoSuchElementException();
        }
        return tokenArray[pos++];
    }


    /** Unimplemented, throws an Error if called. */
    public void remove() {
        throw new Error("TokenArrayIterator.remove() unimplemented");
    }
}

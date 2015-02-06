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

import edu.cmu.sphinx.decoder.scorer.Scoreable;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

/** A factory for simple active lists */
public class SimpleActiveListFactory extends ActiveListFactory {

    /**
     * 
     * @param absoluteBeamWidth
     * @param relativeBeamWidth
     * @param logMath
     */
    public SimpleActiveListFactory(int absoluteBeamWidth,
            double relativeBeamWidth)
    {
        super(absoluteBeamWidth, relativeBeamWidth);
    }

    public SimpleActiveListFactory() {
        
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
        return new SimpleActiveList(absoluteBeamWidth, logRelativeBeamWidth);
    }


    /**
     * An active list that tries to be simple and correct. This type of active list will be slow, but should exhibit
     * correct behavior. Faster versions of the ActiveList exist (HeapActiveList, TreeActiveList).
     * <p/>
     * This class is not thread safe and should only be used by a single thread.
     * <p/>
     * Note that all scores are maintained in the LogMath log domain
     */
    class SimpleActiveList implements ActiveList {

        private int absoluteBeamWidth = 2000;
        private final float logRelativeBeamWidth;
        private Token bestToken;
        private List<Token> tokenList = new LinkedList<Token>();


        /**
         * Creates an empty active list
         *
         * @param absoluteBeamWidth    the absolute beam width
         * @param logRelativeBeamWidth the relative beam width (in the log domain)
         */
        public SimpleActiveList(int absoluteBeamWidth,
                                float logRelativeBeamWidth) {
            this.absoluteBeamWidth = absoluteBeamWidth;
            this.logRelativeBeamWidth = logRelativeBeamWidth;
        }


        /**
         * Adds the given token to the list
         *
         * @param token the token to add
         */
        public void add(Token token) {
            tokenList.add(token);
            if (bestToken == null || token.getScore() > bestToken.getScore()) {
                bestToken = token;
            }
        }


        /**
         * Replaces an old token with a new token
         *
         * @param oldToken the token to replace (or null in which case, replace works like add).
         * @param newToken the new token to be placed in the list.
         */
        public void replace(Token oldToken, Token newToken) {
            add(newToken);
            if (oldToken != null) {
                if (!tokenList.remove(oldToken)) {
                    // Some optional debugging code here to dump out the paths
                    // when this "should never happen" error happens
                    // System.out.println("SimpleActiveList: remove "
                    //         + oldToken + " missing, but replaced by "
                    //         + newToken);
                    // oldToken.dumpTokenPath(true);
                    // newToken.dumpTokenPath(true);
                }
            }
        }


        /**
         * Purges excess members. Remove all nodes that fall below the relativeBeamWidth
         *
         * @return a (possible new) active list
         */
        public ActiveList purge() {
            if (absoluteBeamWidth > 0 && tokenList.size() > absoluteBeamWidth) {
                Collections.sort(tokenList, Scoreable.COMPARATOR);
                tokenList = tokenList.subList(0, absoluteBeamWidth);
            }
            return this;
        }


        /**
         * Retrieves the iterator for this tree.
         *
         * @return the iterator for this token list
         */
        public Iterator<Token> iterator() {
            return tokenList.iterator();
        }


        /**
         * Gets the set of all tokens
         *
         * @return the set of tokens
         */
        public List<Token> getTokens() {
            return tokenList;
        }


        /**
         * Returns the number of tokens on this active list
         *
         * @return the size of the active list
         */
        public final int size() {
            return tokenList.size();
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


        /* (non-Javadoc)
        * @see edu.cmu.sphinx.decoder.search.ActiveList#createNew()
        */
        public ActiveList newInstance() {
            return SimpleActiveListFactory.this.newInstance();
        }
    }
}

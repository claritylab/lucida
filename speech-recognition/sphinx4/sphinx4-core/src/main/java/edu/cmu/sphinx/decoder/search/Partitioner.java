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
import edu.cmu.sphinx.decoder.scorer.Scoreable;

/**
 * Partitions a list of tokens according to the token score, used
 * in {@link PartitionActiveListFactory}. This method is supposed 
 * to provide O(n) performance so it's more preferable than 
 */
public class Partitioner {

    /** Max recursion depth **/
    final private int MAX_DEPTH = 50;


    /**
     * Partitions sub-array of tokens around the end token. 
     * Put all elements less or equal then pivot to the start of the array,
     * shifting new pivot position
     *
     * @param tokens the token array to partition
     * @param start      the starting index of the subarray
     * @param end      the pivot and the ending index of the subarray, inclusive
     * @return the index (after partitioning) of the element around which the array is partitioned
     */
    private int endPointPartition(Token[] tokens, int start, int end) {
        Token pivot = tokens[end];
        float pivotScore = pivot.getScore();
               
        int i = start;
        int j = end - 1;
        
        while (true) {

            while (i < end && tokens[i].getScore() >= pivotScore)
                i++;                
            while (j > i && tokens[j].getScore() < pivotScore)
                j--;
            
            if (j <= i)
                break;
            
            Token current = tokens[j];
            setToken(tokens, j, tokens[i]);
            setToken(tokens, i, current);            
        }

        setToken(tokens, end, tokens[i]);
        setToken(tokens, i, pivot);
        return i;
    }


    /**
     * Partitions sub-array of tokens around the x-th token by selecting the midpoint of the token array as the pivot.
     * Partially solves issues with slow performance on already sorted arrays.
     *
     * @param tokens the token array to partition
     * @param start      the starting index of the subarray
     * @param end      the ending index of the subarray, inclusive
     * @return the index of the element around which the array is partitioned
     */
    private int midPointPartition(Token[] tokens, int start, int end) {
        int middle = (start + end) >>> 1;
        Token temp = tokens[end];
        setToken(tokens, end, tokens[middle]);
        setToken(tokens, middle, temp);
        return endPointPartition(tokens, start, end);
    }


    /**
     * Partitions the given array of tokens in place, so that the highest scoring n token will be at the beginning of
     * the array, not in any order.
     *
     * @param tokens the array of tokens to partition
     * @param size   the number of tokens to partition
     * @param n      the number of tokens in the final partition
     * @return the index of the last element in the partition
     */
    public int partition(Token[] tokens, int size, int n) {
        if (tokens.length > n) {
            return midPointSelect(tokens, 0, size - 1, n, 0);
        } else {
            return findBest(tokens, size);
        }
    }

    /**
     * Simply find the best token and put it in the last slot
     * 
     * @param tokens array of tokens
     * @param size the number of tokens to partition
     * @return index of the best token
     */
    private int findBest(Token[] tokens, int size) {
        int r = -1;
        float lowestScore = Float.MAX_VALUE;
        for (int i = 0; i < tokens.length; i++) {
            float currentScore = tokens[i].getScore();
            if (currentScore <= lowestScore) {
                lowestScore = currentScore;
                r = i; // "r" is the returned index
            }
        }

        // exchange tokens[r] <=> last token,
        // where tokens[r] has the lowest score
        int last = size - 1;
        if (last >= 0) {
            Token lastToken = tokens[last];
            setToken(tokens, last, tokens[r]);
            setToken(tokens, r, lastToken);
        }

        // return the last index
        return last;
    }


    private void setToken(Token[] list, int index, Token token) {
        list[index] = token;
        token.setLocation(index);
    }

    /**
     * Selects the token with the ith largest token score.
     *
     * @param tokens       the token array to partition
     * @param start        the starting index of the subarray
     * @param end          the ending index of the subarray, inclusive
     * @param targetSize   target size of the partition
     * @param depth        recursion depth to avoid stack overflow and fall back to simple partition.
     * @return the index of the token with the ith largest score
     */
    private int midPointSelect(Token[] tokens, int start, int end, int targetSize, int depth) {
        if (depth > MAX_DEPTH) {
            return simplePointSelect (tokens, start, end, targetSize);
        }
        if (start == end) {
            return start;
        }
        int partitionToken = midPointPartition(tokens, start, end);
        int newSize = partitionToken - start + 1;
        if (targetSize == newSize) {
            return partitionToken;
        } else if (targetSize < newSize) {
            return midPointSelect(tokens, start, partitionToken - 1, targetSize, depth + 1);
        } else {
            return midPointSelect(tokens, partitionToken + 1, end, targetSize - newSize, depth + 1);
        }
    }
    
    /**
     * Fallback method to get the partition
     *
     * @param tokens       the token array to partition
     * @param start        the starting index of the subarray
     * @param end          the ending index of the subarray, inclusive
     * @param targetSize   target size of the partition
     * @return the index of the token with the ith largest score
     */
    private int simplePointSelect(Token[] tokens, int start, int end, int targetSize) {
        Arrays.sort(tokens, start, end + 1, Scoreable.COMPARATOR);
        return start + targetSize - 1;
    }
    
}

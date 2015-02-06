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

import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.util.props.Configurable;

/**
 * Defines the interface for the SearchManager. The SearchManager's primary role is to execute the search for a given
 * number of frames. The SearchManager will return interim results as the recognition proceeds and when recognition
 * completes a final result will be returned.
 */
public interface SearchManager extends Configurable {

    /**
     * Allocates the resources necessary for this search. This should be called once before an recognitions are
     * performed
     */
    public void allocate();


    /**
     * Deallocates resources necessary for this search. This should be called once after all recognitions are completed
     * at the search manager is no longer needed.
     */
    public void deallocate();


    /**
     * Prepares the SearchManager for recognition.  This method must be called before <code> recognize </code> is
     * called. Typically, <code> start </code>  and <code> stop </code>  are called bracketing an utterance.
     */
    public void startRecognition();


    /** Performs post-recognition cleanup. This method should be called after recognize returns a final result. */
    public void stopRecognition();


    /**
     * Performs recognition. Processes no more than the given number of frames before returning. This method returns a
     * partial result after nFrames have been processed, or a final result if recognition completes while processing
     * frames.  If a final result is returned, the actual number of frames processed can be retrieved from the result.
     * This method may block while waiting for frames to arrive.
     *
     * @param nFrames the maximum number of frames to process. A final result may be returned before all nFrames are
     *                processed.
     * @return the recognition result, the result may be a partial or a final result; or return null if no frames are
     *         arrived
     */
    public Result recognize(int nFrames);
}



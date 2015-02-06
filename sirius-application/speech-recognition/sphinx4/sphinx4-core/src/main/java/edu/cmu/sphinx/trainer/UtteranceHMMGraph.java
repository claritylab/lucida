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

package edu.cmu.sphinx.trainer;

import edu.cmu.sphinx.linguist.acoustic.AcousticModel;
import edu.cmu.sphinx.linguist.acoustic.UnitManager;

/** Defines the Utterance Graph. */
public class UtteranceHMMGraph extends Graph implements UtteranceGraph {

    /** Constructor for class UtteranceHMMGraph. */
    public UtteranceHMMGraph(String context, Utterance utterance, AcousticModel acousticModel, UnitManager unitManager) {
        super();

        Graph transcriptGraph;
        for (utterance.startTranscriptIterator();
             utterance.hasMoreTranscripts();) {
            /* The transcript object has a pointer to its own dictionary
            */
            Transcript transcript = utterance.nextTranscript();
            transcriptGraph = new TranscriptHMMGraph(context, transcript,
                    acousticModel, unitManager);
            add(transcriptGraph);
        }
    }


    /**
     * Add a transcript graph to the current utterance graph.
     *
     * @param transcriptGraph the transcript graph
     */
    public void add(Graph transcriptGraph) {
        // Implementation requires a single transcript per
        // utterance. The copy will fail if the current utterance
        // graph is not empty.
        copyGraph(transcriptGraph);
    }

}

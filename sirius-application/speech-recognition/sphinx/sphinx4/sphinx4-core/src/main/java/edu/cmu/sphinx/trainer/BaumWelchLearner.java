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

import edu.cmu.sphinx.frontend.*;
import edu.cmu.sphinx.frontend.util.StreamCepstrumSource;
import edu.cmu.sphinx.linguist.acoustic.HMMState;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.SenoneHMM;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.SenoneHMMState;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.trainer.TrainerScore;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Component;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Logger;


/** Provides mechanisms for computing statistics given a set of states and input data. */
public class BaumWelchLearner implements Learner {


    @S4Component(type = FrontEnd.class)
    public static final String FRONT_END = "frontend";
    private FrontEnd frontEnd;

    @S4Component(type = StreamCepstrumSource.class)
    public static final String DATA_SOURCE = "source";
    private StreamCepstrumSource dataSource;

    private LogMath logMath;

    /*
     * The logger for this class
     */
    private static Logger logger =
            Logger.getLogger("edu.cmu.sphinx.trainer.BaumWelch");

    private Data curFeature;
    private UtteranceGraph graph;
    private TrainerScore[][] scoreArray;
    private int lastFeatureIndex;
    private int currentFeatureIndex;
    private float[] betas;
    private float[] outputProbs;
    private float[] componentScores;
    private float[] probCurrentFrame;
    private float totalLogScore;


    public void newProperties(PropertySheet ps) throws PropertyException {
        logMath = LogMath.getInstance();
        dataSource = (StreamCepstrumSource) ps.getComponent(DATA_SOURCE);

        frontEnd = (FrontEnd) ps.getComponent(FRONT_END);
        frontEnd.setDataSource(dataSource);
    }


    // Cut and paste from e.c.s.d.Recognizer.java
    /** Initialize and return the frontend based on the given sphinx properties. */
    protected FrontEnd getFrontEnd() {
        return frontEnd;
    }


    /**
     * Sets the learner to use a utterance.
     *
     * @param utterance the utterance
     * @throws IOException
     */
    public void setUtterance(Utterance utterance) throws IOException {
        String file = utterance.toString();
        InputStream is = new FileInputStream(file);
        dataSource.setInputStream(is, false);
    }


    /**
     * Returns a single frame of speech.
     *
     * @return a feature frame
     * @throws IOException
     */
    private boolean getFeature() {
        try {
            curFeature = frontEnd.getData();

            if (curFeature == null) {
                return false;
            }

            if (curFeature instanceof DataStartSignal) {
                curFeature = frontEnd.getData();
                if (curFeature == null) {
                    return false;
                }
            }

            if (curFeature instanceof DataEndSignal) {
                return false;
            }

            if (curFeature instanceof Signal) {
                throw new Error("Can't score non-content feature");
            }
        } catch (DataProcessingException dpe) {
            System.out.println("DataProcessingException " + dpe);
            dpe.printStackTrace();
            return false;
        }
        return true;
    }


    /** Starts the Learner. */
    public void start() {
    }


    /** Stops the Learner. */
    public void stop() {
    }


    /**
     * Initializes computation for current utterance and utterance graph.
     *
     * @param utterance the current utterance
     * @param graph     the current utterance graph
     * @throws IOException
     */
    public void initializeComputation(Utterance utterance,
                                      UtteranceGraph graph) throws IOException {
        setUtterance(utterance);
        setGraph(graph);
    }


    /**
     * Implements the setGraph method.
     *
     * @param graph the graph
     */
    public void setGraph(UtteranceGraph graph) {
        this.graph = graph;
    }


    /**
     * Prepares the learner for returning scores, one at a time. To do so, it performs the full forward pass, but
     * returns the scores for the backward pass one feature frame at a time.
     */
    private TrainerScore[][] prepareScore() {
        // scoreList will contain a list of score, which in turn are a
        // vector of TrainerScore elements.
        List<TrainerScore[]> scoreList = new ArrayList<TrainerScore[]>();
        int numStates = graph.size();
        TrainerScore[] score = new TrainerScore[numStates];
        betas = new float[numStates];
        outputProbs = new float[numStates];

        // First we do the forward pass. We need this before we can
        // return any probability. When we're doing the backward pass,
        // we can finally return a score for each call of this method.

        probCurrentFrame = new float[numStates];
        // Initialization of probCurrentFrame for the alpha computation
        Node initialNode = graph.getInitialNode();
        int indexInitialNode = graph.indexOf(initialNode);
        for (int i = 0; i < numStates; i++) {
            probCurrentFrame[i] = LogMath.LOG_ZERO;
        }
        // Overwrite in the right position
        probCurrentFrame[indexInitialNode] = 0.0f;

        for (initialNode.startOutgoingEdgeIterator();
             initialNode.hasMoreOutgoingEdges();) {
            Edge edge = initialNode.nextOutgoingEdge();
            Node node = edge.getDestination();
            int index = graph.indexOf(node);
            if (!node.isType("STATE")) {
                // Certainly non-emitting, if it's not in an HMM.
                probCurrentFrame[index] = 0.0f;
            } else {
                // See if it's the last state in the HMM, i.e., if
                // it's non-emitting.
                HMMState state = (HMMState) node.getObject();
                if (!state.isEmitting()) {
                    probCurrentFrame[index] = 0.0f;
                }
                assert false;
            }
        }

        // If getFeature() is true, curFeature contains a valid
        // Feature. If not, a problem or EOF was encountered.
        lastFeatureIndex = 0;
        while (getFeature()) {
            forwardPass(score);
            scoreList.add(score);
            lastFeatureIndex++;
        }
        logger.info("Feature frames read: " + lastFeatureIndex);
        // Prepare for beta computation
        for (int i = 0; i < probCurrentFrame.length; i++) {
            probCurrentFrame[i] = LogMath.LOG_ZERO;
        }
        Node finalNode = graph.getFinalNode();
        int indexFinalNode = graph.indexOf(finalNode);
        // Overwrite in the right position
        probCurrentFrame[indexFinalNode] = 0.0f;
        for (finalNode.startIncomingEdgeIterator();
             finalNode.hasMoreIncomingEdges();) {
            Edge edge = finalNode.nextIncomingEdge();
            Node node = edge.getSource();
            int index = graph.indexOf(node);
            if (!node.isType("STATE")) {
                // Certainly non-emitting, if it's not in an HMM.
                probCurrentFrame[index] = 0.0f;
                assert false;
            } else {
                // See if it's the last state in the HMM, i.e., if
                // it's non-emitting.
                HMMState state = (HMMState) node.getObject();
                if (!state.isEmitting()) {
                    probCurrentFrame[index] = 0.0f;
                }
            }
        }

        return scoreList.toArray(new TrainerScore[scoreList.size()][]);
    }


    /**
     * Gets the TrainerScore for the next frame
     *
     * @return the TrainerScore, or null if EOF was found
     */
    public TrainerScore[] getScore() {
        TrainerScore[] score;
        if (scoreArray == null) {
            // Do the forward pass, and create the necessary arrays
            scoreArray = prepareScore();
            currentFeatureIndex = lastFeatureIndex;
        }
        currentFeatureIndex--;
        if (currentFeatureIndex >= 0) {
            float logScore = LogMath.LOG_ZERO;
            score = scoreArray[currentFeatureIndex];
            assert score.length == betas.length;
            backwardPass(score);
            for (int i = 0; i < betas.length; i++) {
                score[i].setGamma();
                logScore = logMath.addAsLinear(logScore, score[i].getGamma());
            }
            if (currentFeatureIndex == lastFeatureIndex - 1) {
                TrainerScore.setLogLikelihood(logScore);
                totalLogScore = logScore;
            } else {
                if (Math.abs(totalLogScore - logScore) >
                        Math.abs(totalLogScore)) {
                    System.out.println("WARNING: log probabilities differ: " +
                            totalLogScore + " and " + logScore);
                }
            }
            return score;
        } else {
            // We need to clear this, so we start the next iteration
            // on a clean plate.
            scoreArray = null;
            return null;
        }
    }


    /**
     * Computes the acoustic scores using the current Feature and a given node in the graph.
     *
     * @param index the graph index
     * @return the overall acoustic score
     */
    private float calculateScores(int index) {
        float logScore;
        // Find the HMM state for this node
        SenoneHMMState state = (SenoneHMMState) graph.getNode(index).getObject();
        if ((state != null) && (state.isEmitting())) {
            // Compute the scores for each mixture component in this state
            componentScores = state.calculateComponentScore(curFeature);
            // Compute the overall score for this state
            logScore = state.getScore(curFeature);
            // For CI models, for now, we only try to use mixtures
            // with one component
            assert componentScores.length == 1;
        } else {
            componentScores = null;
            logScore = 0.0f;
        }
        return logScore;
    }


    /**
     * Does the forward pass, one frame at a time.
     *
     * @param score the objects transferring info to the buffers
     */
    private void forwardPass(TrainerScore[] score) {
        // Let's precompute the acoustic probabilities and create the
        // score object, one for each state
        for (int i = 0; i < graph.size(); i++) {
            outputProbs[i] = calculateScores(i);
            score[i] = new TrainerScore(curFeature,
                    outputProbs[i],
                    (HMMState) graph.getNode(i).getObject(),
                    componentScores);
            score[i].setAlpha(probCurrentFrame[i]);
        }

        // Now, the forward pass.
        float[] probPreviousFrame = probCurrentFrame;
        probCurrentFrame = new float[graph.size()];
        // First, the emitting states. We have to do this because the
        // emitting states use probabilities from the previous
        // frame. The non-emitting states, however, since they don't
        // consume frames, use probabilities from the current frame
        for (int indexNode = 0; indexNode < graph.size(); indexNode++) {
            Node node = graph.getNode(indexNode);
            // Treat dummy node (and initial and final nodes) the same
            // as non-emitting
            if (!node.isType("STATE")) {
                continue;
            }
            SenoneHMMState state = (SenoneHMMState) node.getObject();
            SenoneHMM hmm = (SenoneHMM) state.getHMM();
            if (!state.isEmitting()) {
                continue;
            }
            // Initialize the current frame probability with 0.0f, log scale
            probCurrentFrame[indexNode] = LogMath.LOG_ZERO;
            for (node.startIncomingEdgeIterator();
                 node.hasMoreIncomingEdges();) {
                // Finds out what the previous node and previous state are
                Node previousNode = node.nextIncomingEdge().getSource();
                int indexPreviousNode = graph.indexOf(previousNode);
                HMMState previousState = (HMMState) previousNode.getObject();
                float logTransitionProbability;
                // previous state could be have an associated hmm state...
                if (previousState != null) {
                    // Make sure that the transition happened from a state
                    // that either is in the same model, or was a
                    // non-emitting state
                    assert ((!previousState.isEmitting()) ||
                            (previousState.getHMM() == hmm));
                    if (!previousState.isEmitting()) {
                        logTransitionProbability = 0.0f;
                    } else {
                        logTransitionProbability =
                                hmm.getTransitionProbability(
                                        previousState.getState(),
                                        state.getState());
                    }
                } else {
                    // Previous state is a dummy state or beginning of
                    // utterance.
                    logTransitionProbability = 0.0f;
                }
                // Adds the alpha and transition from the previous
                // state into the current alpha
                probCurrentFrame[indexNode] =
                        logMath.addAsLinear(probCurrentFrame[indexNode],
                                probPreviousFrame[indexPreviousNode] +
                                        logTransitionProbability);
                // System.out.println("State= " + indexNode + " curr "
                // + probCurrentFrame[indexNode] + " prev " +
                // probPreviousFrame[indexNode] + " trans " +
                // logTransitionProbability);
            }
            // Finally, multiply by this state's output probability for the
            // current Feature (add in log scale)
            probCurrentFrame[indexNode] += outputProbs[indexNode];
            // System.out.println("State= " + indexNode + " alpha= " +
            // probCurrentFrame[indexNode]);
            score[indexNode].setAlpha(probCurrentFrame[indexNode]);
        }

        // Finally, the non-emitting states
        for (int indexNode = 0; indexNode < graph.size(); indexNode++) {
            Node node = graph.getNode(indexNode);
            HMMState state = null;
            SenoneHMM hmm = null;
            if (node.isType("STATE")) {
                state = (HMMState) node.getObject();
                hmm = (SenoneHMM) state.getHMM();
                if (state.isEmitting()) {
                    continue;
                }
            } else if (graph.isInitialNode(node)) {
                score[indexNode].setAlpha(LogMath.LOG_ZERO);
                probCurrentFrame[indexNode] = LogMath.LOG_ZERO;
                continue;
            }
            // Initialize the current frame probability 0.0f, log scale
            probCurrentFrame[indexNode] = LogMath.LOG_ZERO;
            for (node.startIncomingEdgeIterator();
                 node.hasMoreIncomingEdges();) {
                float logTransitionProbability;
                // Finds out what the previous node and previous state are
                Node previousNode = node.nextIncomingEdge().getSource();
                int indexPreviousNode = graph.indexOf(previousNode);
                if (previousNode.isType("STATE")) {
                    HMMState previousState =
                            (HMMState) previousNode.getObject();
                    // Make sure that the transition happened from a
                    // state that either is in the same model, or was
                    // a non-emitting state
                    assert ((!previousState.isEmitting()) ||
                            (previousState.getHMM() == hmm));
                    if (!previousState.isEmitting()) {
                        logTransitionProbability = 0.0f;
                    } else {
                        // previousState == state
                        logTransitionProbability =
                                hmm.getTransitionProbability(
                                        previousState.getState(),
                                        state.getState());
                    }
                } else {
                    logTransitionProbability = 0.0f;
                }
                // Adds the alpha and transition from the previous
                // state into the current alpha
                probCurrentFrame[indexNode] =
                        logMath.addAsLinear(probCurrentFrame[indexNode],
                                probCurrentFrame[indexPreviousNode] +
                                        logTransitionProbability);
                // System.out.println("State= " + indexNode + " curr "
                // + probCurrentFrame[indexNode] + " prev " +
                // probPreviousFrame[indexNode] + " trans " +
                // logTransitionProbability);
            }
            // System.out.println("State= " + indexNode + " alpha= " +
            // probCurrentFrame[indexNode]);

            // Non-emitting states have the equivalent of output
            // probability of 1.0. In log scale, this is the same as
            // adding 0.0f, or doing nothing.
            score[indexNode].setAlpha(probCurrentFrame[indexNode]);
        }
    }


    /**
     * Does the backward pass, one frame at a time.
     *
     * @param score the feature to be used
     */
    private void backwardPass(TrainerScore[] score) {
        // Now, the backward pass.
        for (int i = 0; i < graph.size(); i++) {
            outputProbs[i] = score[i].getScore();
            score[i].setBeta(probCurrentFrame[i]);
        }
        float[] probNextFrame = probCurrentFrame;
        probCurrentFrame = new float[graph.size()];

        // First, the emitting states
        for (int indexNode = 0; indexNode < graph.size(); indexNode++) {
            Node node = graph.getNode(indexNode);
            // Treat dummy node (and initial and final nodes) the same
            // as non-emitting
            if (!node.isType("STATE")) {
                continue;
            }
            HMMState state = (HMMState) node.getObject();
            SenoneHMM hmm = (SenoneHMM) state.getHMM();
            if (!state.isEmitting()) {
                continue;
            }
            // Initialize the current frame probability with log
            // probability of log(0f)
            probCurrentFrame[indexNode] = LogMath.LOG_ZERO;
            for (node.startOutgoingEdgeIterator();
                 node.hasMoreOutgoingEdges();) {
                float logTransitionProbability;
                // Finds out what the next node and next state are
                Node nextNode = node.nextOutgoingEdge().getDestination();
                int indexNextNode = graph.indexOf(nextNode);
                HMMState nextState = (HMMState) nextNode.getObject();
                if (nextState != null) {
                    // Make sure that the transition happened to a
                    // non-emitting state, or to the same model
                    assert ((!nextState.isEmitting()) ||
                            (nextState.getHMM() == hmm));
                    if (nextState.getHMM() != hmm) {
                        logTransitionProbability = 0.0f;
                    } else {
                        logTransitionProbability =
                                hmm.getTransitionProbability(state.getState(),
                                        nextState.getState());
                    }
                } else {
                    // Next state is a dummy state or beginning of
                    // utterance.
                    logTransitionProbability = 0.0f;
                }
                // Adds the beta, the output prob, and the transition
                // from the next state into the current beta
                probCurrentFrame[indexNode] =
                        logMath.addAsLinear(probCurrentFrame[indexNode],
                                probNextFrame[indexNextNode] +
                                        logTransitionProbability +
                                        outputProbs[indexNextNode]);
            }
            // System.out.println("State= " + indexNode + " beta= " + probCurrentFrame[indexNode]);
            score[indexNode].setBeta(probCurrentFrame[indexNode]);
        }

        // Now, the non-emitting states

        // We have to go backwards because for non-emitting states we
        // use the current frame probability, and we need to refer to
        // states that are downstream in the graph
        for (int indexNode = graph.size() - 1; indexNode >= 0; indexNode--) {
            Node node = graph.getNode(indexNode);
            HMMState state = null;
            if (node.isType("STATE")) {
                state = (HMMState) node.getObject();
                if (state.isEmitting()) {
                    continue;
                }
            } else if (graph.isFinalNode(node)) {
                score[indexNode].setBeta(LogMath.LOG_ZERO);
                probCurrentFrame[indexNode] = LogMath.LOG_ZERO;
                continue;
            }
            // Initialize the current frame probability with log(0f)
            probCurrentFrame[indexNode] = LogMath.LOG_ZERO;
            for (node.startOutgoingEdgeIterator();
                 node.hasMoreOutgoingEdges();) {
                float logTransitionProbability;
                // Finds out what the next node and next state are
                Node nextNode = node.nextOutgoingEdge().getDestination();
                int indexNextNode = graph.indexOf(nextNode);
                if (nextNode.isType("STATE")) {
                    HMMState nextState = (HMMState) nextNode.getObject();
                    // Make sure that the transition happened to a
                    // state that either is the same, or is emitting
                    assert ((nextState.isEmitting()) || (nextState == state));
                    // In any case, the transition (at this point) is
                    // assumed to be 1.0f, or 0.0f in log scale.
                    logTransitionProbability = 0.0f;
                    /*
                 if (!nextState.isEmitting()) {
                 logTransitionProbability = 0.0f;
                 } else {
                 logTransitionProbability =
                     hmm.getTransitionProbability(state.getState(),
                                  nextState.getState());
                 }
                 */
                } else {
                    logTransitionProbability = 0.0f;
                }
                // Adds the beta, the transition, and the output prob
                // from the next state into the current beta
                probCurrentFrame[indexNode] =
                        logMath.addAsLinear(probCurrentFrame[indexNode],
                                probCurrentFrame[indexNextNode] +
                                        logTransitionProbability);
            }
            // System.out.println("State= " + indexNode + " beta= " + probCurrentFrame[indexNode]);
            score[indexNode].setBeta(probCurrentFrame[indexNode]);
        }
    }

    /* Pseudo code:
    forward pass:
        token = maketoken(initialstate);
        List initialTokenlist = new List;
        newtokenlist.add(token);
    
        // Initial token is on a nonemitting state; no need to score;
        List newList = expandToEmittingStateList(initialTokenList){

        while (morefeatures){
           scoreTokenList(emittingTokenList, featurevector[timestamp]);
           pruneTokenList(emittingTokenList);
           List newList = expandToEmittingStateList(emittingTokenList){
           timestamp++;
        }
        // Some logic to expand to a final nonemitting state (how)?
        expandToNonEmittingStates(emittingTokenList);
    */

    /*
    private void forwardPass() {
        ActiveList activelist = new FastActiveList(createInitialToken());
	AcousticScorer acousticScorer = new ThreadedAcousticScorer();
	FeatureFrame featureFrame = frontEnd.getFeatureFrame(1, "");
	Pruner pruner = new SimplePruner();

        // Initialization code pushing initial state to emitting state here

        while ((featureFrame.getFeatures() != null)) {
            ActiveList nextActiveList = new FastActiveList();

            // At this point we have only emitting states. We score
            // and prune them
            ActiveList emittingStateList = new FastActiveList(); 
	                  // activelist.getEmittingStateList();
            acousticScorer.calculateScores(emittingStateList.getTokens());
	    // The pruner must clear up references to pruned objects
            emittingStateList = pruner.prune( emittingStateList);
            
            expandStateList(emittingStateList, nextActiveList); 

            while (nextActiveList.hasNonEmittingStates()){
		// extractNonEmittingStateList will pull out the list
		// of nonemitting states completely from the
		// nextActiveList. At this point nextActiveList does
		// not have a list of nonemitting states and must
		// instantiate a new one.
                ActiveList nonEmittingStateList = 
		    nextActiveList.extractNonEmittingStateList();
                nonEmittingStateList = pruner.prune(nonEmittingStateList);
                expandStateList(nonEmittingStateList, nextActiveList); 
            }
            activeList = newActiveList;
        }
    }
    */

    /* Pseudo code
    backward pass:
       state = finaltoken.state.wholelistofeverythingthatcouldbefinal;
        while (moreTokensAtCurrentTime) { 
            Token token = nextToken();
            State state = token.state;
            state.gamma = state.logalpha + state.logbeta - logtotalprobability;
            SentenceHMM.updateState(state,state.gamma,vector[state.timestamp]);
            // state.update (state.gamma, vector[state.timestamp], updatefunction());
            while token.hasMoreIncomingEdges() {
                Edge transition = token.nextIncomingEdge();
                double logalpha = transition.source.alpha;
                double logbeta  = transition.destination.beta;
                double logtransition = transition.transitionprob;
                // transition.posterior = alpha*transition*beta / 
                //                           totalprobability;
                double localtransitionbetascore = logtransition + logbeta + 
                                              transition.destination.logscore;
                double transition.posterior = localtransitionbetascore + 
                                              logalpha - logtotalprobability;
                transition.updateaccumulator(transition.posterior);
                // transition.updateaccumulator(transition.posterior, updatefunction());
                SentenceHMM.updateTransition(transition, transitionstate,state.gamma);
                transition.source.beta = Logadd(transition.source.beta,
                                                localtransitionbetascore);
                                        
            }
        }
    */

    /*
    private void expandStateList(ActiveList stateList, 
                                 ActiveList nextActiveList) {
        while (stateList.hasMoreTokens()) {
            Token token = emittingStateList.getNextToken();

	    // First get list of links to possible future states
            List successorList = getSuccessors(token);
            while (successorList.hasMoreEntries()) {
                UtteranceGraphEdge edge = successorList.getNextEntry();

		// create a token for the future state, if its not
		// already in active list; The active list will check
		// for the key "edge.destination()" in both of its
		// lists
                if (nextActiveList.hasState(edge.destination())) {
                    Token newToken = 
			nextActiveList.getTokenForState(edge.destination());
		} else {
                    Token newToken = new Token(edge.destination());
		}

		// create a link between current state and future state
                TrainerLink newlink = new TrainerLink(edge, token, newToken);
                newlink.logScore = token.logScore + edge.transition.logprob();

		// add link to the appropriate lists for source and
		// destination tokens
                token.addOutGoingLink(newlink);

                newToken.addIncomingLink(newlink);
                newToken.alpha = logAdd(newToken.alpha, newlink.logScore);

                // At this point, we have placed a new token in the
                // successor state, and linked the token at the
                // current state to the token at the non-emitting
                // states.

		// Add token to appropriate active list
                nextActiveList.add(newToken);
            }
        }
    }
    */

    /*
    private void expandToEmittingStateList(List tokenList){
	List emittingTokenList = new List();
	do {
	    List nonEmittingTokenList = new List();
	    expandtokens(newtokenlist, emittingTokenList, 
			 nonemittingTokenList);
	    while (nonEmittingTokenList.length() != 0);
	    return emittingTokenList;
	}
    }
    */

    /*
    private void expandtokens(List tokens, List nonEmittingStateList, 
			      List EmittingStateList){
	while (moreTokens){
	    sucessorlist = SentenceHMM.gettransitions(nextToken());
	    while (moretransitions()){
		transition = successor;
		State destinationState = successor.state;
		newtoken = gettokenfromHash(destinationState, 
					    currenttimestamp);
		newtoken.logscore = Logadd(newtoken.logscore,
				   token.logscore + transition.logscore);
		// Add transition to newtoken predecessor list?
		// Add transition to token sucessor list
		// Should we define a token "arc" for this. ??
		if (state.isemitting)
		    EmittingStateList.add(newtoken);
		else
		    nonEmittingStateList.add(newtoken);
	    } 
	}
    }
    */

}

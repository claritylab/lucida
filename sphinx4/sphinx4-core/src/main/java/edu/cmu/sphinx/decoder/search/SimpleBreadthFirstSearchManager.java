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

import edu.cmu.sphinx.decoder.pruner.Pruner;
import edu.cmu.sphinx.decoder.scorer.AcousticScorer;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.linguist.Linguist;
import edu.cmu.sphinx.linguist.SearchState;
import edu.cmu.sphinx.linguist.SearchStateArc;
import edu.cmu.sphinx.linguist.WordSearchState;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.StatisticsVariable;
import edu.cmu.sphinx.util.Timer;
import edu.cmu.sphinx.util.TimerPool;
import edu.cmu.sphinx.util.props.*;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;

import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Provides the breadth first search. To perform recognition an application should call initialize before recognition
 * begins, and repeatedly call <code> recognize </code> until Result.isFinal() returns true. Once a final result has
 * been obtained, <code> terminate </code> should be called.
 * <p/>
 * <p/>
 * All scores and probabilities are maintained in the log math log domain.
 * <p/>
 * For information about breadth first search please refer to "Spoken Language Processing", X. Huang, PTR
 */

// TODO - need to add in timing code.
public class SimpleBreadthFirstSearchManager extends TokenSearchManager {

    /** The property that defines the name of the linguist to be used by this search manager. */
    @S4Component(type = Linguist.class)
    public final static String PROP_LINGUIST = "linguist";

    /** The property that defines the name of the linguist to be used by this search manager. */
    @S4Component(type = Pruner.class)
    public final static String PROP_PRUNER = "pruner";

    /** The property that defines the name of the scorer to be used by this search manager. */
    @S4Component(type = AcousticScorer.class)
    public final static String PROP_SCORER = "scorer";

    /** The property that defines the name of the active list factory to be used by this search manager. */
    @S4Component(type = ActiveListFactory.class)
    public final static String PROP_ACTIVE_LIST_FACTORY = "activeListFactory";

    /**
     * The property that when set to <code>true</code> will cause the recognizer to count up all the tokens in the
     * active list after every frame.
     */
    @S4Boolean(defaultValue = false)
    public final static String PROP_SHOW_TOKEN_COUNT = "showTokenCount";

    /**
     * The property that sets the minimum score relative to the maximum score in the word list for pruning. Words with a
     * score less than relativeBeamWidth * maximumScore will be pruned from the list
     */
    @S4Double(defaultValue = 0.0)
    public final static String PROP_RELATIVE_WORD_BEAM_WIDTH = "relativeWordBeamWidth";

    /**
     * The property that controls whether or not relative beam pruning will be performed on the entry into a
     * state.
     */
    @S4Boolean(defaultValue = false)
    public final static String PROP_WANT_ENTRY_PRUNING = "wantEntryPruning";

    /**
     * The property that controls the number of frames processed for every time the decode growth step is skipped.
     * Setting this property to zero disables grow skipping. Setting this number to a small integer will increase the
     * speed of the decoder but will also decrease its accuracy. The higher the number, the less often the grow code is
     * skipped.
     */
    @S4Integer(defaultValue = 0)
    public final static String PROP_GROW_SKIP_INTERVAL = "growSkipInterval";


    protected Linguist linguist; // Provides grammar/language info
    private Pruner pruner; // used to prune the active list
    private AcousticScorer scorer; // used to score the active list
    protected int currentFrameNumber; // the current frame number
    protected ActiveList activeList; // the list of active tokens
    
    protected ActiveList activeList2; // the list of active tokens

    protected List<Token> resultList; // the current set of results
    protected LogMath logMath;

    private Logger logger;
    private String name;

    // ------------------------------------
    // monitoring data
    // ------------------------------------

    private Timer scoreTimer; // TODO move these timers out
    private Timer pruneTimer;
    protected Timer growTimer;
    private StatisticsVariable totalTokensScored;
    private StatisticsVariable tokensPerSecond;
    private StatisticsVariable curTokensScored;
    private StatisticsVariable tokensCreated;
    private StatisticsVariable viterbiPruned;
    private StatisticsVariable beamPruned;

    // ------------------------------------
    // Working data
    // ------------------------------------

    protected boolean showTokenCount;
    private boolean wantEntryPruning;
    protected Map<SearchState, Token> bestTokenMap;
    private float logRelativeWordBeamWidth;
    private int totalHmms;
    private double startTime;
    private float threshold;
    private float wordThreshold;
    private int growSkipInterval;
    protected ActiveListFactory activeListFactory;
    protected boolean streamEnd;
    
    PrintWriter pw_in = null; 
    PrintWriter pw_out = null;    
    PrintWriter pw_actlist = null;    

    public SimpleBreadthFirstSearchManager() {
        
    }

    /**
     * 
     * @param linguist
     * @param pruner
     * @param scorer
     * @param activeListFactory
     * @param showTokenCount
     * @param relativeWordBeamWidth
     * @param growSkipInterval
     * @param wantEntryPruning
     */
    public SimpleBreadthFirstSearchManager(Linguist linguist, Pruner pruner,
                                           AcousticScorer scorer, ActiveListFactory activeListFactory,
                                           boolean showTokenCount, double relativeWordBeamWidth,
                                           int growSkipInterval, boolean wantEntryPruning) {
        this.name = getClass().getName();
        this.logger = Logger.getLogger(name);
        this.logMath = LogMath.getInstance();
        this.linguist = linguist;
        this.pruner = pruner;
        this.scorer = scorer;
        this.activeListFactory = activeListFactory;
        this.showTokenCount = showTokenCount;
        this.growSkipInterval = growSkipInterval;
        this.wantEntryPruning = wantEntryPruning;
        this.logRelativeWordBeamWidth = logMath.linearToLog(relativeWordBeamWidth);
        this.keepAllTokens = true;
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        
        logMath = LogMath.getInstance();
        logger = ps.getLogger();
        name = ps.getInstanceName();

        linguist = (Linguist) ps.getComponent(PROP_LINGUIST);
        pruner = (Pruner) ps.getComponent(PROP_PRUNER);
        scorer = (AcousticScorer) ps.getComponent(PROP_SCORER);
        activeListFactory = (ActiveListFactory) ps.getComponent(PROP_ACTIVE_LIST_FACTORY);
        showTokenCount = ps.getBoolean(PROP_SHOW_TOKEN_COUNT);

        double relativeWordBeamWidth = ps.getDouble(PROP_RELATIVE_WORD_BEAM_WIDTH);
        growSkipInterval = ps.getInt(PROP_GROW_SKIP_INTERVAL);
        wantEntryPruning = ps.getBoolean(PROP_WANT_ENTRY_PRUNING);
        logRelativeWordBeamWidth = logMath.linearToLog(relativeWordBeamWidth);
        
        this.keepAllTokens = true;      
    }


    /** Called at the start of recognition. Gets the search manager ready to recognize */
    public void startRecognition() {
        logger.finer("starting recognition");

        linguist.startRecognition();
        pruner.startRecognition();
        scorer.startRecognition();
        localStart();
        if (startTime == 0.0) {
            startTime = System.currentTimeMillis();
        }
    }


    /**
     * Performs the recognition for the given number of frames.
     *
     * @param nFrames the number of frames to recognize
     * @return the current result or null if there is no Result (due to the lack of frames to recognize)
     */
    public Result recognize(int nFrames) {
        
      //  System.out.println("nFrames: " + nFrames);
        boolean done = false;
        Result result = null;
        streamEnd = false;
 
        for (int i = 0; i < nFrames && !done; i++) {
            done = recognize();
        }
        
        try {
                pw_out = new PrintWriter(new FileOutputStream(new File("hmm_data_out.txt"),false));
            } catch (FileNotFoundException ex) {
                Logger.getLogger(SimpleBreadthFirstSearchManager.class.getName()).log(Level.SEVERE, null, ex);
        }   
        
        // print final activeList
        pw_out.printf("%d %d %d %f\n", currentFrameNumber, activeList.size(), activeList.getBestToken().hashCode(), activeList.getBestScore());
        for (Token token : activeList) {
            pw_out.printf("%d ",  token.hashCode());
        }
        pw_out.printf("\n\n");
        pw_out.printf("%d\n", resultList.size());
        // print final resultList
        for (Token token : resultList) {
            pw_out.printf("%d ",  token.hashCode());
        }
        pw_out.printf("\n");
         
        // generate a new temporary result if the current token is based on a final search state
        // remark: the first check for not null is necessary in cases that the search space does not contain scoreable tokens.
        if (activeList.getBestToken() != null) {
            // to make the current result as correct as possible we undo the last search graph expansion here
            ActiveList fixedList = undoLastGrowStep();
            
            // print final fixedList
         /*   pw_out.printf("%d\n", fixedList.size());
            for (Token token : fixedList) {
                pw_out.printf("%d ",  token.hashCode());
            }*/
            	
            // Now create the result using the fixed active-list.
            if (!streamEnd)
                result = new Result(activeList, resultList, currentFrameNumber, done);
                //result = new Result(fixedList, resultList, currentFrameNumber, done);

        }

        if (showTokenCount) {
            showTokenCount();
        }
        
        pw_out.printf("\n");    
        pw_out.close();

        return result;
    }


    /**
     * Because the growBranches() is called although no data is left after the last speech frame, the ordering of the
     * active-list might depend on the transition probabilities and (penalty-scores) only. Therefore we need to undo the last
     * grow-step up to final states or the last emitting state in order to fix the list.
     * @return newly created list
     */
    protected ActiveList undoLastGrowStep() {
        ActiveList fixedList = activeList.newInstance();

        for (Token token : activeList) {
            Token curToken = token.getPredecessor();

            // remove the final states that are not the real final ones because they're just hide prior final tokens:
            while (curToken.getPredecessor() != null && (
                    (curToken.isFinal() && curToken.getPredecessor() != null && !curToken.getPredecessor().isFinal())
                            || (curToken.isEmitting() && curToken.getData() == null) // the so long not scored tokens
                            || (!curToken.isFinal() && !curToken.isEmitting()))) {
                curToken = curToken.getPredecessor();
            }

            fixedList.add(curToken);
        }

        return fixedList;
    }


    /** Terminates a recognition */
    public void stopRecognition() {
        localStop();
        scorer.stopRecognition();
        pruner.stopRecognition();
        linguist.stopRecognition();

        System.out.println("Total time: " + getTotalTime());
                
        logger.finer("recognition stopped");
    }


    /**
     * Performs recognition for one frame. Returns true if recognition has been completed.
     *
     * @return <code>true</code> if recognition is completed.
     */
    protected boolean recognize() {
        boolean more = scoreTokens(); // score emitting tokens
        if (more) {
 //           pruneBranches(); // eliminate poor branches
            currentFrameNumber++;
            
            pw_actlist.printf("%d %d %d %f\n", currentFrameNumber, activeList.size(), activeList.getBestToken().hashCode(), activeList.getBestScore());
            for (Token token : activeList) {
                pw_actlist.printf("%d ",  token.hashCode());
            }
            pw_actlist.printf("\n");
            
            activeList2 = activeListFactory.newInstance();
                    
            for (Token token : activeList) {
                collectSuccessorTokens2(token);
            }
            
            if (growSkipInterval == 0
                    || (currentFrameNumber % growSkipInterval) != 0) {
                growBranches(); // extend remaining branches
            }
        }
        return !more;
    }


    /** Gets the initial grammar node from the linguist and creates a GrammarNodeToken */
    protected void localStart() {    
        try {
            pw_in = new PrintWriter(new FileOutputStream(new File("hmm_data_in.txt"),false));
        } catch (FileNotFoundException ex) {
            Logger.getLogger(SimpleBreadthFirstSearchManager.class.getName()).log(Level.SEVERE, null, ex);
        }
        
        try {
            pw_actlist = new PrintWriter(new FileOutputStream(new File("hmm_data_actlist.txt"),false));
        } catch (FileNotFoundException ex) {
            Logger.getLogger(SimpleBreadthFirstSearchManager.class.getName()).log(Level.SEVERE, null, ex);
        }
        
        currentFrameNumber = 0;
        curTokensScored.value = 0;
        ActiveList newActiveList = activeListFactory.newInstance();
        SearchState state = linguist.getSearchGraph().getInitialState();
        newActiveList.add(new Token(state, currentFrameNumber));
        activeList = newActiveList;
        
        activeList2 = activeListFactory.newInstance();
        
        pw_actlist.printf("%d %d %d %f\n", currentFrameNumber, activeList.size(), activeList.getBestToken().hashCode(), activeList.getBestScore());
        for (Token token : activeList) {
            pw_actlist.printf("%d ",  token.hashCode());
        }
        pw_actlist.printf("\n");
        
     //   System.out.println("Initial state: " + state.toPrettyString());

        for (Token token : activeList) {
            collectSuccessorTokens2(token);
        }
        
        Data data = scorer.calculateScores(activeList2.getTokens());        
        
        growBranches();
        
    }

    /** Local cleanup for this search manager */
    protected void localStop() {
        
        pw_in.close();
        pw_actlist.close();
            
    }


    /**
     * Goes through the active list of tokens and expands each token, finding the set of successor tokens until all the
     * successor tokens are emitting tokens.
     */
    protected void growBranches() {
        int mapSize = activeList.size() * 10;
        if (mapSize == 0) {
            mapSize = 1;
        }
        growTimer.start();
        bestTokenMap = new HashMap<SearchState, Token>(mapSize);
        ActiveList oldActiveList = activeList;
        resultList = new LinkedList<Token>();
        activeList = activeListFactory.newInstance();
        
        threshold = oldActiveList.getBeamThreshold();
        wordThreshold = oldActiveList.getBestScore() + logRelativeWordBeamWidth;
        
        //System.out.println("logRelativeWordBeamWidth: " + logRelativeWordBeamWidth);
        
        for (Token token : oldActiveList) {
            collectSuccessorTokens(token);
        }
        growTimer.stop();
        
        totalHmms += activeList.size();       
        System.out.println("Frame: " + currentFrameNumber + " Hmms: "
                    + activeList.size() + "  total " + totalHmms);
                    
        if (logger.isLoggable(Level.FINE)) {
            int hmms = activeList.size();
            totalHmms += hmms;
            logger.fine("Frame: " + currentFrameNumber + " Hmms: "
                    + hmms + "  total " + totalHmms);
        }
    }


    /**
     * Calculate the acoustic scores for the active list. The active list should contain only emitting tokens.
     *
     * @return <code>true</code> if there are more frames to score, otherwise, false
     */
    protected boolean scoreTokens() {
        boolean hasMoreFrames = false;

        scoreTimer.start();
        Data data = scorer.calculateScores(activeList.getTokens());
        scoreTimer.stop();
        
        Token bestToken = null;
        if (data instanceof Token) {
            bestToken = (Token)data;
        } else if (data == null) {
        	streamEnd = true;
    	}
        
        if (bestToken != null) {
            hasMoreFrames = true;
            activeList.setBestToken(bestToken);
        }

        // update statistics
        curTokensScored.value += activeList.size();
        totalTokensScored.value += activeList.size();
        tokensPerSecond.value = totalTokensScored.value / getTotalTime();

//        if (logger.isLoggable(Level.FINE)) {
//            logger.fine(currentFrameNumber + " " + activeList.size()
//                    + " " + curTokensScored.value + " "
//                    + (int) tokensPerSecond.value);
//        }

        return hasMoreFrames;
    }


    /**
     * Returns the total time since we start4ed
     *
     * @return the total time (in seconds)
     */
    private double getTotalTime() {
        return (System.currentTimeMillis() - startTime) / 1000.0;
    }


    /** Removes unpromising branches from the active list */
    protected void pruneBranches() {
        int startSize = activeList.size();
        pruneTimer.start();
      //  System.out.println("Size before: " + startSize);
        activeList = pruner.prune(activeList);
       // System.out.println("Size after: " + activeList.size());

        beamPruned.value += startSize - activeList.size();
        pruneTimer.stop();
    }


    /**
     * Gets the best token for this state
     *
     * @param state the state of interest
     * @return the best token
     */
    protected Token getBestToken(SearchState state) {
        Token best = bestTokenMap.get(state);
      //  System.out.println("BT " + best + " for state " + state);
        if (logger.isLoggable(Level.FINER) && best != null) {
            logger.finer("BT " + best + " for state " + state);
        }
        return best;
    }

    /**
     * Sets the best token for a given state
     *
     * @param token the best token
     * @param state the state
     * @return the previous best token for the given state, or null if no previous best token
     */
    protected Token setBestToken(Token token, SearchState state) {
        return bestTokenMap.put(state, token);
    }

    public ActiveList getActiveList() {
        return activeList;
    }

    protected void collectSuccessorTokens2(Token token) {
        SearchState state = token.getSearchState();

        SearchStateArc[] arcs = state.getSuccessors();

        pw_in.printf("%d %d %d %d %f %d %d\n", currentFrameNumber, arcs.length, state.hashCode(), token.hashCode(), token.getScore(), (token.isFinal()) ? 1 : 0, (state instanceof WordSearchState) ? 1:0);
        
        for (SearchStateArc arc : arcs) {
            
            SearchState nextState = arc.getState();
            
          //  String sig = nextState.getSignature();
            Token predecessor = getResultListPredecessor(token);
            float logEntryScore = token.getScore() + arc.getProbability();

            Token newToken = new Token(predecessor, nextState, logEntryScore,
                        arc.getInsertionProbability(),
                        arc.getLanguageProbability(), 
                        currentFrameNumber);
            
//            pw_in.printf("%d %f %d %d %f\n", newToken.hashCode(), newToken.getScore(), nextState.hashCode(), (nextState.isEmitting()) ? 1:0, arc.getProbability());
            pw_in.printf("%d %f %d %d %f %d %d\n", newToken.hashCode(), newToken.getScore(), nextState.hashCode(), (nextState.isEmitting()) ? 1:0, arc.getProbability(), (newToken.isFinal()) ? 1 : 0, (nextState instanceof WordSearchState) ? 1:0);
                
            if (!newToken.isEmitting()) {
                    // if not emitting, check to see if we've already visited
                    // this state during this frame. Expand the token only if we
                    // haven't visited it already. This prevents the search
                    // from getting stuck in a loop of states with no
                    // intervening emitting nodes. This can happen with nasty
                    // jsgf grammars such as ((foo*)*)*
                if (!isVisited(newToken)) {
                        collectSuccessorTokens2(newToken);
                }        
            }
                    
            activeList2.add(newToken);

        }

    }
    
    /**
     * Collects the next set of emitting tokens from a token and accumulates them in the active or result lists
     *
     * @param token the token to collect successors from
     */
    protected void collectSuccessorTokens(Token token) {
        SearchState state = token.getSearchState();
        // If this is a final state, add it to the final list
        if (token.isFinal()) {
            resultList.add(token);
        }
        if (token.getScore() < threshold) {
            return;
        }
        if (state instanceof WordSearchState
                && token.getScore() < wordThreshold) {
            return;
        }
        SearchStateArc[] arcs = state.getSuccessors();
        // For each successor
        // calculate the entry score for the token based upon the
        // predecessor token score and the transition probabilities
        // if the score is better than the best score encountered for
        // the SearchState and frame then create a new token, add
        // it to the lattice and the SearchState.
        // If the token is an emitting token add it to the list,
        // otherwise recursively collect the new tokens successors.
        for (SearchStateArc arc : arcs) {
            SearchState nextState = arc.getState();
            // We're actually multiplying the variables, but since
            // these come in log(), multiply gets converted to add
            float logEntryScore = token.getScore() + arc.getProbability();
            
            if (wantEntryPruning) { // false by default
                if (logEntryScore < threshold) {
                    continue;
                }
                if (nextState instanceof WordSearchState
                        && logEntryScore < wordThreshold) {
                    continue;
                }
            }
            Token predecessor = getResultListPredecessor(token);
            Token bestToken = getBestToken(nextState);
            boolean firstToken = bestToken == null;
            if (firstToken || bestToken.getScore() <= logEntryScore) {
                Token newToken = new Token(predecessor, nextState, logEntryScore,
                        arc.getInsertionProbability(),
                        arc.getLanguageProbability(), 
                        currentFrameNumber);
                tokensCreated.value++;
                setBestToken(newToken, nextState);
                if (!newToken.isEmitting()) {
                    // if not emitting, check to see if we've already visited
                    // this state during this frame. Expand the token only if we
                    // haven't visited it already. This prevents the search
                    // from getting stuck in a loop of states with no
                    // intervening emitting nodes. This can happen with nasty
                    // jsgf grammars such as ((foo*)*)*
                    if (!isVisited(newToken)) {
                        collectSuccessorTokens(newToken);
                       // System.out.println("here!!");
                    }
                } else {
                    if (firstToken) {
                        activeList.add(newToken);
                    } else {
                        activeList.replace(bestToken, newToken);
                        viterbiPruned.value++;
                    }
                }
            } else {
                viterbiPruned.value++;
            }
        }
    }


    /**
     * Determines whether or not we've visited the state associated with this token since the previous frame.
     *
     * @param t the token to check
     * @return true if we've visited the search state since the last frame
     */
    private boolean isVisited(Token t) {
        SearchState curState = t.getSearchState();

        t = t.getPredecessor();

        while (t != null && !t.isEmitting()) {
            if (curState.equals(t.getSearchState())) {
                return true;
            }
            t = t.getPredecessor();
        }
        return false;
    }


    /** Counts all the tokens in the active list (and displays them). This is an expensive operation. */
    protected void showTokenCount() {
        if (logger.isLoggable(Level.INFO)) {
            Set<Token> tokenSet = new HashSet<Token>();
            for (Token token : activeList) {
                while (token != null) {
                    tokenSet.add(token);
                    token = token.getPredecessor();
                }
            }
            logger.info("Token Lattice size: " + tokenSet.size());
            tokenSet = new HashSet<Token>();
            for (Token token : resultList) {
                while (token != null) {
                    tokenSet.add(token);
                    token = token.getPredecessor();
                }
            }
            logger.info("Result Lattice size: " + tokenSet.size());
        }
    }


    /**
     * Returns the best token map.
     *
     * @return the best token map
     */
    protected Map<SearchState, Token> getBestTokenMap() {
        return bestTokenMap;
    }


    /**
     * Sets the best token Map.
     *
     * @param bestTokenMap the new best token Map
     */
    protected void setBestTokenMap(Map<SearchState, Token> bestTokenMap) {
        this.bestTokenMap = bestTokenMap;
    }


    /**
     * Returns the result list.
     *
     * @return the result list
     */
    public List<Token> getResultList() {
        return resultList;
    }


    /**
     * Returns the current frame number.
     *
     * @return the current frame number
     */
    public int getCurrentFrameNumber() {
        return currentFrameNumber;
    }


    /**
     * Returns the Timer for growing.
     *
     * @return the Timer for growing
     */
    public Timer getGrowTimer() {
        return growTimer;
    }


    /**
     * Returns the tokensCreated StatisticsVariable.
     *
     * @return the tokensCreated StatisticsVariable.
     */
    public StatisticsVariable getTokensCreated() {
        return tokensCreated;
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.decoder.search.SearchManager#allocate()
    */
    public void allocate() {
        totalTokensScored = StatisticsVariable
                .getStatisticsVariable("totalTokensScored");
        tokensPerSecond = StatisticsVariable
                .getStatisticsVariable("tokensScoredPerSecond");
        curTokensScored = StatisticsVariable
                .getStatisticsVariable("curTokensScored");
        tokensCreated = StatisticsVariable
                .getStatisticsVariable("tokensCreated");
        viterbiPruned = StatisticsVariable
                .getStatisticsVariable("viterbiPruned");
        beamPruned = StatisticsVariable.getStatisticsVariable("beamPruned");


        try {
            linguist.allocate();
            pruner.allocate();
            scorer.allocate();
        } catch (IOException e) {
            throw new RuntimeException("Allocation of search manager resources failed", e);
        }

        scoreTimer = TimerPool.getTimer(this, "Score");
        pruneTimer = TimerPool.getTimer(this, "Prune");
        growTimer = TimerPool.getTimer(this, "Grow");
    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.decoder.search.SearchManager#deallocate()
    */
    public void deallocate() {
	try {
            scorer.deallocate();
            pruner.deallocate();
            linguist.deallocate();
        } catch (IOException e) {
            throw new RuntimeException("Deallocation of search manager resources failed", e);
        }
    }


    @Override
    public String toString() {
        return name;
    }
}

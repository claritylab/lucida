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
package edu.cmu.sphinx.decoder.scorer;

import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.BaseDataProcessor;
import edu.cmu.sphinx.frontend.DataProcessingException;
import edu.cmu.sphinx.util.CustomThreadFactory;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Boolean;
import edu.cmu.sphinx.util.props.S4Integer;

import java.util.*;
import java.util.concurrent.*;

/**
 * An acoustic scorer that breaks the scoring up into a configurable number of separate threads.
 * <p/>
 * All scores are maintained in LogMath log base
 */
public class ThreadedAcousticScorer extends SimpleAcousticScorer {

    /**
     * The property that controls the thread priority of scoring threads.
     * Must be a value between {@link Thread#MIN_PRIORITY} and {@link Thread#MAX_PRIORITY}, inclusive.
     * The default is {@link Thread#NORM_PRIORITY}.
     */
    @S4Integer(defaultValue = Thread.NORM_PRIORITY)
    public final static String PROP_THREAD_PRIORITY = "threadPriority";

    /**
     * The property that controls the number of threads that are used to score HMM states. If the isCpuRelative
     * property is false, then is is the exact number of threads that are used to score HMM states. If the isCpuRelative
     * property is true, then this value is combined with the number of available processors on the system. If you want
     * to have one thread per CPU available to score states, set the NUM_THREADS property to 0 and the isCpuRelative to
     * true. If you want exactly one thread to process scores set NUM_THREADS to 1 and isCpuRelative to false.
     * <p/>
     * If the value is 1 isCpuRelative is false no additional thread will be instantiated, and all computation will be
     * done in the calling thread itself. The default value is 0.
     */
    @S4Integer(defaultValue = 0)
    public final static String PROP_NUM_THREADS = "numThreads";

    /**
     * The property that controls whether the number of available CPUs on the system is used when determining
     * the number of threads to use for scoring. If true, the NUM_THREADS property is combined with the available number
     * of CPUS to determine the number of threads. Note that the number of threads is contained to be never lower than
     * zero. Also, if the number of threads is 0, the states are scored on the calling thread, no separate threads are
     * started. The default value is false.
     */
    @S4Boolean(defaultValue = true)
    public final static String PROP_IS_CPU_RELATIVE = "isCpuRelative";

    /**
     * The property that controls the minimum number of scoreables sent to a thread. This is used to prevent
     * over threading of the scoring that could happen if the number of threads is high compared to the size of the
     * active list. The default is 50
     */
    @S4Integer(defaultValue = 10)
    public final static String PROP_MIN_SCOREABLES_PER_THREAD = "minScoreablesPerThread";

    private final static String className = ThreadedAcousticScorer.class.getSimpleName();

    private int numThreads;         // number of threads in use
    private int threadPriority;
    private int minScoreablesPerThread; // min scoreables sent to a thread
    private ExecutorService executorService;

    /**
     * @param frontEnd
     *            the frontend to retrieve features from for scoring
     * @param scoreNormalizer
     *            optional post-processor for computed scores that will
     *            normalize scores. If not set, no normalization will applied
     *            and the token scores will be returned unchanged.
     * @param minScoreablesPerThread
     *            the number of threads that are used to score HMM states. If
     *            the isCpuRelative property is false, then is is the exact
     *            number of threads that are used to score HMM states. If the
     *            isCpuRelative property is true, then this value is combined
     *            with the number of available processors on the system. If you
     *            want to have one thread per CPU available to score states, set
     *            the NUM_THREADS property to 0 and the isCpuRelative to true.
     *            If you want exactly one thread to process scores set
     *            NUM_THREADS to 1 and isCpuRelative to false.
     *            <p/>
     *            If the value is 1 isCpuRelative is false no additional thread
     *            will be instantiated, and all computation will be done in the
     *            calling thread itself. The default value is 0.
     * @param cpuRelative
     *            controls whether the number of available CPUs on the system is
     *            used when determining the number of threads to use for
     *            scoring. If true, the NUM_THREADS property is combined with
     *            the available number of CPUS to determine the number of
     *            threads. Note that the number of threads is constrained to be
     *            never lower than zero. Also, if the number of threads is 0,
     *            the states are scored on the calling thread, no separate
     *            threads are started. The default value is false.
     * @param numThreads
     *            the minimum number of scoreables sent to a thread. This is
     *            used to prevent over threading of the scoring that could
     *            happen if the number of threads is high compared to the size
     *            of the active list. The default is 50
     * @param threadPriority
     *            the thread priority of scoring threads. Must be a value between
     *            {@link Thread#MIN_PRIORITY} and {@link Thread#MAX_PRIORITY}, inclusive.
     *            The default is {@link Thread#NORM_PRIORITY}.
     */
    public ThreadedAcousticScorer(BaseDataProcessor frontEnd, ScoreNormalizer scoreNormalizer,
                                  int minScoreablesPerThread, boolean cpuRelative, int numThreads, int threadPriority) {
        super(frontEnd, scoreNormalizer);
        init(minScoreablesPerThread, cpuRelative, numThreads, threadPriority);
    }

    public ThreadedAcousticScorer() {
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        init(ps.getInt(PROP_MIN_SCOREABLES_PER_THREAD), ps.getBoolean(PROP_IS_CPU_RELATIVE),
            ps.getInt(PROP_NUM_THREADS), ps.getInt(PROP_THREAD_PRIORITY));
    }

    private void init(int minScoreablesPerThread, boolean cpuRelative, int numThreads, int threadPriority) {
        this.minScoreablesPerThread = minScoreablesPerThread;
        if (cpuRelative) {
            numThreads += Runtime.getRuntime().availableProcessors();
        }
        this.numThreads = numThreads;
        this.threadPriority = threadPriority;
    }

    @Override
    public void allocate() {
        super.allocate();
        if (executorService == null) {
            if (numThreads > 1) {
                logger.fine("# of scoring threads: " + numThreads);
                executorService = Executors.newFixedThreadPool(numThreads,
                    new CustomThreadFactory(className, true, threadPriority));
            } else {
                logger.fine("no scoring threads");
            }
        }
    }

    @Override
    public void deallocate() {
        super.deallocate();
        if (executorService != null) {
            executorService.shutdown();
            executorService = null;
        }
    }

    @Override
    protected <T extends Scoreable> T doScoring(List<T> scoreableList, final Data data) throws Exception {
        if (numThreads > 1) {
            int totalSize = scoreableList.size();
            int jobSize = Math.max((totalSize + numThreads - 1) / numThreads, minScoreablesPerThread);

            if (jobSize < totalSize) {
                List<Callable<T>> tasks = new ArrayList<Callable<T>>();
                for (int from = 0, to = jobSize; from < totalSize; from = to, to += jobSize) {
                    final List<T> scoringJob = scoreableList.subList(from, Math.min(to, totalSize));
                    tasks.add(new Callable<T>() {
                        public T call() throws Exception {
                            return ThreadedAcousticScorer.super.doScoring(scoringJob, data);
                        }
                    });
                }

                List<T> finalists = new ArrayList<T>(tasks.size());
       
                for (Future<T> result : executorService.invokeAll(tasks))
                    finalists.add(result.get());
       
                if (finalists.size() == 0) {
                    throw new DataProcessingException("No scoring jobs ended");
                }
                
                return Collections.min(finalists, Scoreable.COMPARATOR);
            }
        }
        // if no additional threads are necessary, do the scoring in the calling thread
        return super.doScoring(scoreableList, data);
    }

}

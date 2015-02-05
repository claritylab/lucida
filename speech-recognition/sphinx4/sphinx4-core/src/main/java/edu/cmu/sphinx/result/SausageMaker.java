/*
 * Copyright 1999-2004 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 *
 * Created on Aug 10, 2004
 */
package edu.cmu.sphinx.result;

import edu.cmu.sphinx.linguist.dictionary.Pronunciation;
import edu.cmu.sphinx.util.LogMath;

import java.util.*;

/**
 * <p/>
 * The SausageMaker takes word lattices as input and turns them into sausages (Confusion Networks) according to Mangu,
 * Brill and Stolcke, "Finding Consensus in Speech Recognition: word error minimization and other applications of
 * confusion networks", Computer Speech and Language, 2000. Note that the <code>getBestHypothesis</code> of the
 * ConfidenceResult object returned by the {@link #score(Result) score} method returns the path where all the words have
 * the highest posterior probability within its corresponding time slot. </p>
 *
 * @author pgorniak
 */
public class SausageMaker extends AbstractSausageMaker {

    /** Construct an empty sausage maker */
    public SausageMaker() {
    }
    
    public SausageMaker(float languageWieght) {
        this.languageWeight = languageWieght;
    }

    /**
     * Construct a sausage maker
     *
     * @param l the lattice to construct a sausage from
     */
    public SausageMaker(Lattice l) {
        lattice = l;
    }


    /**
     * Perform the inter word clustering stage of the algorithm
     *
     * @param clusters the current cluster set
     */
    protected void interWordCluster(List<Cluster> clusters) {
        while (interWordClusterStep(clusters)) ;
    }


    /**
     * Returns the latest begin time of all nodes in the given cluster.
     *
     * @param cluster the cluster to examine
     * @return the latest begin time
     */
    public int getLatestBeginTime(List<Node> cluster) {
        if (cluster.isEmpty()) {
            return -1;
        }
        int startTime = 0;
        for (Node n : cluster) {
            if (n.getBeginTime() > startTime) {
                startTime = n.getBeginTime();
            }
        }
        return startTime;
    }


    /**
     * Returns the earliest end time of all nodes in the given cluster.
     *
     * @param cluster the cluster to examine
     * @return the earliest end time
     */
    public int getEarliestEndTime(List<Node> cluster) {
        if (cluster.isEmpty()) {
            return -1;
        }
        int endTime = Integer.MAX_VALUE;
        for (Node n : cluster) {
            if (n.getEndTime() < endTime) {
                endTime = n.getEndTime();
            }
        }
        return endTime;
    }


    /**
     * Perform one inter word clustering step of the algorithm
     *
     * @param clusters the current cluster set
     */
    protected boolean interWordClusterStep(List<Cluster> clusters) {
        Cluster toBeMerged1 = null;
        Cluster toBeMerged2 = null;
        double maxSim = Double.NEGATIVE_INFINITY;
        ListIterator<Cluster> i = clusters.listIterator();
        while (i.hasNext()) {
            Cluster c1 = i.next();
            if (!i.hasNext()) {
                break;
            }
            ListIterator<Cluster> j = clusters.listIterator(i.nextIndex());
            while (j.hasNext()) {
                Cluster c2 = j.next();
                double sim = interClusterDistance(c1, c2);
                if (sim > maxSim && hasOverlap(c1, c2)) {
                    maxSim = sim;
                    toBeMerged1 = c1;
                    toBeMerged2 = c2;
                }
            }
        }
        if (toBeMerged1 != null) {
            clusters.remove(toBeMerged2);
            toBeMerged1.add(toBeMerged2);
            return true;
        }
        return false;
    }


    /**
     * Find the string edit distance between to lists of objects. 
     * Objects are compared using .equals() 
     * TODO: could be moved to a general utility class
     *
     * @param p1 the first list
     * @param p2 the second list
     * @return the string edit distance between the two lists
     */
    protected static int stringEditDistance(List<?> p1, List<?> p2) {
        if (p1.isEmpty()) {
            return p2.size();
        }
        if (p2.isEmpty()) {
            return p1.size();
        }
        int[][] distances = new int[p1.size() + 1][p2.size() + 1];
        for (int i = 0; i <= p1.size(); i++) {
            distances[i][0] = i;
        }
        for (int j = 0; j <= p2.size(); j++) {
            distances[0][j] = j;
        }
        for (int i = 1; i <= p1.size(); i++) {
            for (int j = 1; j <= p2.size(); j++) {
                int min = Math.min(distances[i - 1][j - 1]
                        + (p1.get(i - 1).equals(p2.get(j - 1)) ? 0 : 1),
                        distances[i - 1][j] + 1);
                min = Math.min(min, distances[i][j - 1] + 1);
                distances[i][j] = min;
            }
        }
        return distances[p1.size()][p2.size()];
    }


    /**
     * Compute the phonetic similarity of two lattice nodes, based on the string edit distance between their most likely
     * pronunciations. TODO: maybe move to Node.java?
     *
     * @param n1 the first node
     * @param n2 the second node
     * @return the phonetic similarity, between 0 and 1
     */
    protected double computePhoneticSimilarity(Node n1, Node n2) {
        Pronunciation p1 = n1.getWord().getMostLikelyPronunciation();
        Pronunciation p2 = n2.getWord().getMostLikelyPronunciation();
        double sim = stringEditDistance(Arrays.asList(p1.getUnits()),
                Arrays.asList(p2.getUnits()));
        sim /= (p1.getUnits().length + p2.getUnits().length);
        return 1 - sim;
    }


    /**
     * Calculate the distance between two clusters
     *
     * @param c1 the first cluster
     * @param c2 the second cluster
     * @return the inter cluster similarity, or Double.NEGATIVE_INFINITY if these clusters should never be clustered
     *         together.
     */
    protected double interClusterDistance(Cluster c1, Cluster c2) {
        if (areClustersInRelation(c1, c2)) {
            return Double.NEGATIVE_INFINITY;
        }
        float totalSim = LogMath.LOG_ZERO;
        float wordPairCount = (float) 0.0;
        HashSet<String> wordsSeen1 = new HashSet<String>();
        LogMath logMath = LogMath.getInstance();

        for (Node node1 : c1.getElements()) {
            String word1 = node1.getWord().getSpelling();
            if (wordsSeen1.contains(word1)) {
                continue;
            }
            wordsSeen1.add(word1);
            HashSet<String> wordsSeen2 = new HashSet<String>();
            for (Node node2 : c2.getElements()) {
                String word2 = node2.getWord().getSpelling();
                if (wordsSeen2.contains(word2)) {
                    continue;
                }
                wordsSeen2.add(word2);
                float sim = (float) computePhoneticSimilarity(node1, node2);
                sim = logMath.linearToLog(sim);
                sim += wordSubClusterProbability(c1, word1);
                sim += wordSubClusterProbability(c2, word2);
                totalSim = logMath.addAsLinear(totalSim, sim);
                wordPairCount++;
            }
        }
        return totalSim - logMath.logToLinear(wordPairCount);
    }


    /**
     * Check whether these to clusters stand in a relation to each other. Two clusters are related if a member of one is
     * an ancestor of a member of the other cluster.
     *
     * @param cluster1 the first cluster
     * @param cluster2 the second cluster
     * @return true if the clusters are related
     */
    protected boolean areClustersInRelation(Cluster cluster1, Cluster cluster2) {
        for (Node n1: cluster1.getElements()) {
            for (Node n2: cluster2.getElements()) {
                if (n1.hasAncestralRelationship(n2)) {
                    return true;
                }
            }
        }
        return false;
    }


    /**
     * Calculate the distance between two clusters, forcing them to have the same words in them, and to not be related
     * to each other.
     *
     * @param cluster1 the first cluster
     * @param cluster2 the second cluster
     * @return The intra-cluster distance, or Double.NEGATIVE_INFINITY if the clusters should never be clustered
     *         together.
     */
    protected double intraClusterDistance(Cluster cluster1, Cluster cluster2) {
        LogMath logMath = LogMath.getInstance();
        double maxSim = Double.NEGATIVE_INFINITY;

        for (Node node1 : cluster1.getElements()) {
            for (Node node2 : cluster2.getElements()) {
                if (!node1.getWord().getSpelling().equals(
                            node2.getWord().getSpelling()))
                    return Double.NEGATIVE_INFINITY;

                if (node1.hasAncestralRelationship(node2))
                    return Double.NEGATIVE_INFINITY;

                double overlap = getOverlap(node1, node2);
                if (overlap > 0.0) {
                    overlap = logMath.logToLinear((float) overlap);
                    overlap += node1.getPosterior() + node2.getPosterior();
                    if (overlap > maxSim) {
                        maxSim = overlap;
                    }
                }
            }
        }
        return maxSim;
    }


    /**
     * Perform the intra word clustering stage of the algorithm
     *
     * @param clusters the current list of clusters
     */
    protected void intraWordCluster(List<Cluster> clusters) {
        while (intraWordClusterStep(clusters)) ;
    }


    /**
     * Perform a step of the intra word clustering stage
     *
     * @param clusters the current list of clusters
     * @return did two clusters get merged?
     */
    protected boolean intraWordClusterStep(List<Cluster> clusters) {
        Cluster toBeMerged1 = null;
        Cluster toBeMerged2 = null;
        double maxSim = Double.NEGATIVE_INFINITY;
        for (ListIterator<Cluster> i = clusters.listIterator(); i.hasNext();) {
            Cluster c1 = i.next();
            if (!i.hasNext()) {
                break;
            }
            for (ListIterator<Cluster> j = clusters.listIterator(i.nextIndex()); j.hasNext();) {
                Cluster c2 = j.next();
                double sim = intraClusterDistance(c1, c2);
                if (sim > maxSim) {
                    maxSim = sim;
                    toBeMerged1 = c1;
                    toBeMerged2 = c2;
                }
            }
        }
        if (toBeMerged1 != null) {
            clusters.remove(toBeMerged2);
            toBeMerged1.add(toBeMerged2);
            return true;
        }
        return false;
    }


    /**
     * Turn the lattice contained in this sausage maker into a sausage object.
     *
     * @return the sausage producing by collapsing the lattice.
     */
    public Sausage makeSausage() {
        List<Cluster> clusters = new ArrayList<Cluster>(lattice.nodes.size());
        for (Node n : lattice.nodes.values()) {
            n.cacheDescendants();
            Cluster bucket = new Cluster(n);
            clusters.add(bucket);
        }
        intraWordCluster(clusters);
        interWordCluster(clusters);
        clusters = topologicalSort(clusters);
        return sausageFromClusters(clusters);
    }


    /** @see edu.cmu.sphinx.result.ConfidenceScorer#score(edu.cmu.sphinx.result.Result) */
    public ConfidenceResult score(Result result) {
        lattice = new Lattice(result);
        LatticeOptimizer lop = new LatticeOptimizer(lattice);
        lop.optimize();
        lattice.computeNodePosteriors(languageWeight);
        return makeSausage();
    }


    /**
     * Topologically sort the clusters. Note that this is a brute force sort by removing the min cluster from the list
     * of clusters, since Collections.sort() does not work in all cases.
     *
     * @param clusters the list of clusters to be topologically sorted
     * @return a topologically sorted list of clusters
     */
    private List<Cluster> topologicalSort(List<Cluster> clusters) {
        Comparator<Cluster> comparator = new ClusterComparator();
        List<Cluster> sorted = new ArrayList<Cluster>(clusters.size());
        while (!clusters.isEmpty()) {
            Cluster cluster = Collections.min(clusters, comparator);
            clusters.remove(cluster);
            sorted.add(cluster);
        }
        return sorted;
    }
}

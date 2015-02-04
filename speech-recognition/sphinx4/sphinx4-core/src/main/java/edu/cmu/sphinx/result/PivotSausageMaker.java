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
 * Created on Nov 23, 2004
 */
package edu.cmu.sphinx.result;

import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;


/**
 * This is an implementation of an alternative sausage making algorithm as described in the following paper. Runs in
 * less time than the SausageMaker, but will also build different sausages. A general algorithm for word graph matrix
 * decomposition, Hakkani-Tur, D. Riccardi, G., AT&T Labs.-Res., USA;, This paper appears in: 2003 IEEE International
 * Conference on Acoustics, Speech, and Signal Processing, 2003. Proceedings. (ICASSP '03).
 *
 * @author P. Gorniak
 */
public class PivotSausageMaker extends AbstractSausageMaker {

    protected List<Node> sortedNodes;
    protected List<Cluster> clusters;

    /**
     * Turn the lattice contained in this sausage maker into a sausage object.
     *
     * @return the sausage producing by collapsing the lattice.
     */
    public Sausage makeSausage() {
        for (Node node : sortedNodes) {
            int slot = findMostOverlappingSlot(node);
            Cluster targetCluster = getCluster(slot);
            if (!containsAncestor(targetCluster, node)) {
                targetCluster.add(node);
            } else {
                Cluster newCluster = new Cluster(node);
                int newTime = (targetCluster.startTime + targetCluster.endTime) / 2;
                targetCluster.endTime = newTime;
                newCluster.startTime = newTime;
                clusters.add(slot + 1, newCluster);
            }
        }
        return sausageFromClusters(clusters);
    }


    /**
     * Get a cluster by index
     *
     * @param i the index into the list of clusters
     * @return the requested cluster
     */
    protected Cluster getCluster(int i) {
        return clusters.get(i);
    }


    /**
     * Check whether this given cluster contains and ancestor of the given node
     *
     * @param c the cluster to check
     * @param n the node to check for ancestors
     * @return whether and ancestor was found
     */
    protected boolean containsAncestor(Cluster c, Node n) {
        for (Node cNode : c.getElements()) {
            if (cNode.isAncestorOf(n)) {
                return true;
            }
        }
        return false;
    }


    /**
     * Find the most overlapping time slot in the list of clusters
     *
     * @param n the node whose times to check for
     * @return the index of the most overlapping slot
     */
    protected int findMostOverlappingSlot(Node n) {
        int maxOverlap = Integer.MIN_VALUE;
        int bestCluster = -1;
        ListIterator<Cluster> i = clusters.listIterator();
        i.next(); //never cluster anything with the <s> node
        while (i.hasNext()) {
            int index = i.nextIndex();
            if (!i.hasNext()) {
                //never cluster anything with the </s> node
                return bestCluster;
            }
            Cluster c = i.next();
            int overlap = getOverlap(n, c.startTime, c.endTime);
            if (overlap > maxOverlap) {
                maxOverlap = overlap;
                bestCluster = index;
            }
        }
        return bestCluster;
    }


    /** @see edu.cmu.sphinx.result.ConfidenceScorer#score(edu.cmu.sphinx.result.Result) */
    public ConfidenceResult score(Result result) {
        lattice = new Lattice(result);
        LatticeOptimizer lop = new LatticeOptimizer(lattice);
        lop.optimize();
        lattice.computeNodePosteriors(languageWeight);
        List<Node> seedPath = lattice.getViterbiPath();
        sortedNodes = lattice.sortNodes();
        sortedNodes.removeAll(seedPath);
        clusters = new LinkedList<Cluster>();
        for (Node n : seedPath) {
            clusters.add(new Cluster(n));
        }
        return makeSausage();
    }

}

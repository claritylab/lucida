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
 * Created on Nov 27, 2004
 *
 */
package edu.cmu.sphinx.result;

import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4Double;

import java.util.*;

/**
 * Parent to all sausage makers.
 *
 * @author P. Gorniak
 */
public abstract class AbstractSausageMaker implements ConfidenceScorer, Configurable {

    /**
     * A Cluster is a set of Nodes together with their earliest start time and latest end time. A SausageMaker builds up
     * a sequence of such clusters that then gets turned into a Sausage.
     *
     * @see Node
     * @see Sausage
     * @see SausageMaker
     */
    class Cluster implements Iterable<Node> {

        public int startTime;
        public int endTime;
        private List<Node> elements = new LinkedList<Node>();


        public Cluster(Node n) {
            startTime = n.getBeginTime();
            endTime = n.getEndTime();
            elements.add(n);
        }


        public Cluster(int start, int end) {
            startTime = start;
            endTime = end;
        }


        public void add(Node n) {
            if (n.getBeginTime() < startTime) {
                startTime = n.getBeginTime();
            }
            if (n.getEndTime() > endTime) {
                endTime = n.getEndTime();
            }
            elements.add(n);
        }


        public void add(Cluster c) {
            if (c.startTime < startTime) {
                startTime = c.startTime;
            }
            if (c.endTime > endTime) {
                endTime = c.endTime;
            }
            elements.addAll(c.getElements());
        }

        public Iterator<Node> iterator() {
            return elements.iterator();
        }


        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("s: ").append(startTime).append(" e: ").append(endTime).append('[');
            for (Node node : elements)
                sb.append(node).append(',');
            if (!elements.isEmpty())
                sb.setLength(sb.length() - 1);
            sb.append(']');
            return sb.toString();
        }


        /** @return Returns the elements. */
        public List<Node> getElements() {
            return elements;
        }


        /** @param elements The elements to set. */
        public void setElements(List<Node> elements) {
            this.elements = elements;
        }
    }

    class ClusterComparator implements Comparator<Cluster> {

        /**
         * Compares to clusters according to their topological relationship. Relies on strong assumptions about the
         * possible constituents of clusters which will only be valid during the sausage creation process.
         *
         * @param cluster1 the first cluster
         * @param cluster2 the second cluster
         */
        public int compare(Cluster cluster1, Cluster cluster2) {
            for (Node n1 : cluster1) {
                for (Node n2 : cluster2) {
                    if (n1.isAncestorOf(n2)) {
                        return -1;
                    } else if (n2.isAncestorOf(n1)) {
                        return 1;
                    }
                }
            }
            return 0;
        }
    }

    /** The property that defines the language model weight. */
    @S4Double(defaultValue = 1.0)
    public final static String PROP_LANGUAGE_WEIGHT = "languageWeight";

    protected float languageWeight;

    protected Lattice lattice;


    public AbstractSausageMaker() {
    }
   
    /** @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet) */
    public void newProperties(PropertySheet ps) throws PropertyException {
        languageWeight = ps.getFloat(PROP_LANGUAGE_WEIGHT);
    }


    protected static int getOverlap(Node n, int startTime, int endTime) {
        return Math.min(n.getEndTime(), endTime) -
                Math.max(n.getBeginTime(), startTime);
    }


    protected static int getOverlap(Node n1, Node n2) {
        return Math.min(n1.getEndTime(), n2.getEndTime()) -
                Math.max(n1.getBeginTime(), n2.getBeginTime());
    }


    /**
     * Returns true if the two given clusters has time overlaps.
     *
     * @param cluster1 the first cluster to examine
     * @param cluster2 the second cluster to examine
     * @return true if the clusters has overlap, false if they don't
     */
    protected boolean hasOverlap(Cluster cluster1, Cluster cluster2) {
        return (cluster1.startTime < cluster2.endTime &&
                cluster2.startTime < cluster1.endTime);
    }


    /**
     * Return the total probability mass of the subcluster of nodes of the given cluster that all have the given word as
     * their word.
     *
     * @param cluster the cluster to subcluster from
     * @param word    the word to subcluster by
     * @return the log probability mass of the subcluster formed by the word
     */
    protected double wordSubClusterProbability(List<Node> cluster, String word) {
        return clusterProbability(makeWordSubCluster(cluster, word));
    }


    /**
     * Return the total probability mass of the subcluster of nodes of the given cluster that all have the given word as
     * their word.
     *
     * @param cluster the cluster to subcluster from
     * @param word    the word to subcluster by
     * @return the log probability mass of the subcluster formed by the word
     */
    protected double wordSubClusterProbability(Cluster cluster, String word) {
        return clusterProbability(makeWordSubCluster(cluster, word));
    }


    /**
     * Calculate the sum of posteriors in this cluster.
     *
     * @param cluster the cluster to sum over
     * @return the probability sum
     */
    protected double clusterProbability(List<Node> cluster) {
        float p = LogMath.LOG_ZERO;
        LogMath logMath = LogMath.getInstance();

        for (Node node : cluster)
            p = logMath.addAsLinear(p, (float)node.getPosterior());

        return p;
    }


    /**
     * Calculate the sum of posteriors in this cluster.
     *
     * @param cluster the cluster to sum over
     * @return the probability sum
     */
    protected double clusterProbability(Cluster cluster) {
        return clusterProbability(cluster.elements);
    }


    /**
     * Form a subcluster by extracting all nodes corresponding to a given word.
     *
     * @param cluster the parent cluster
     * @param word    the word to cluster by
     * @return the subcluster.
     */
    protected List<Node> makeWordSubCluster(List<Node> cluster, String word) {
        List<Node> sub = new ArrayList<Node>();
        for (Node n : cluster) {
            if (n.getWord().getSpelling().equals(word)) {
                sub.add(n);
            }
        }
        return sub;
    }


    /**
     * Form a subcluster by extracting all nodes corresponding to a given word.
     *
     * @param cluster the parent cluster
     * @param word    the word to cluster by
     * @return the subcluster.
     */
    protected Cluster makeWordSubCluster(Cluster cluster, String word) {
        List<Node> l = makeWordSubCluster(cluster.elements, word);
        Cluster c = new Cluster(cluster.startTime, cluster.endTime);
        c.elements = l;
        return c;
    }


    /**
     * print out a list of clusters for debugging
     *
     * @param clusters
     */
    protected void printClusters(List<Cluster> clusters) {
        ListIterator<Cluster> i = clusters.listIterator();
        while (i.hasNext()) {
            System.out.print("----cluster " + i.nextIndex() + " : ");
            System.out.println(i.next());
        }
        System.out.println("----");
    }


    /**
     * Turn a list of lattice node clusters into a Sausage object.
     *
     * @param clusters the list of node clusters in topologically correct order
     * @return the Sausage corresponding to the cluster list
     */
    protected Sausage sausageFromClusters(List<Cluster> clusters) {
        Sausage sausage = new Sausage(clusters.size());
        int index = 0;
        for (Cluster cluster : clusters) {
            HashSet<String> seenWords = new HashSet<String>();
            for (Node node : cluster) {
                Word word = node.getWord();
                if (seenWords.contains(word.getSpelling())) {
                    continue;
                }
                seenWords.add(word.getSpelling());
                WordResult swr =
                    new WordResult(
                            node,
                            wordSubClusterProbability(
                                cluster, word.getSpelling()));
                sausage.addWordHypothesis(index, swr);
            }
            index++;
        }
        sausage.fillInBlanks();
        return sausage;
    }
}

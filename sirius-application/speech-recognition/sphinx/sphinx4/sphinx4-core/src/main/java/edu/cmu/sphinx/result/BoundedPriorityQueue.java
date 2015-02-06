/*
 * Copyright 1999-2013 Carnegie Mellon University.
 * Portions Copyright 2002 Sun Microsystems, Inc.
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 */
package edu.cmu.sphinx.result;

import java.util.Iterator;
import java.util.TreeSet;

public class BoundedPriorityQueue<T> implements Iterable<T> {

    TreeSet<T> items;
    int maxSize;

    public BoundedPriorityQueue(int maxSize) {
        items = new TreeSet<T>();
        this.maxSize = maxSize;
    }

    public void add(T item) {
        items.add(item);
        if (items.size() > maxSize)
            items.pollFirst();
    }

    public int size() {
        return items.size();
    }

    public T poll() {
        return items.pollLast();
    }

    public Iterator<T> iterator() {
        return items.iterator();
    }
}

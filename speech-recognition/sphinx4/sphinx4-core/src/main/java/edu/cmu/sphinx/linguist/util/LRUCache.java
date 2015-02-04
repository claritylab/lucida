package edu.cmu.sphinx.linguist.util;

import java.util.LinkedHashMap;
import java.util.Map;

/** An LRU cache */
@SuppressWarnings("serial")
public class LRUCache<K, V> extends LinkedHashMap<K, V> {

    final int maxSize;


    /**
     * Creates an LRU cache with the given maximum size
     *
     * @param maxSize the maximum size of the cache
     */
    public LRUCache(int maxSize) {
        this.maxSize = maxSize;
    }


    /**
     * Determines if the eldest entry in the map should be removed.
     *
     * @param eldest the eldest entry
     * @return true if the eldest entry should be removed
     */
    @Override
    protected boolean removeEldestEntry(Map.Entry<K,V> eldest) {
        return size() > maxSize;
    }
}

package edu.cmu.sphinx.util;

import java.util.HashMap;
import java.util.Map;

/**
 * Provides a simple object cache.
 *
 * <p>Object stored in cache must properly implement {@link Object#hashCode hashCode} and {@link Object#equals equals}.
 *
 * <p><strong>Note that this class is not synchronized.</strong>
 * If multiple threads access a cache concurrently, and at least one of
 * the threads modifies the cache, it <i>must</i> be synchronized externally.
 * This is typically accomplished by synchronizing on some object that
 * naturally encapsulates the cache.
 */
public class Cache<T> {

    private Map<T, T> map = new HashMap<T, T>();

    private int hits = 0;

    /**
     * Puts the given object in the cache if it is not already present.
     *
     * <p>If the object is already cached, than the instance that exists in the cached is returned.
     * Otherwise, it is placed in the cache and null is returned.
     *
     * @param object object to cache
     * @return the cached object or null if the given object was not already cached
     */
    public T cache(T object) {
        T result = map.get(object);
        if (result == null) {
            map.put(object, object);
        } else {
            hits++;
        }
        return result;
    }

    /**
     * Returns the number of cache hits, which is the number of times {@link #cache} was called
     * and returned an object that already existed in the cache.
     *
     * @return the number of cache hits
     */
    public int getHits() {
        return hits;
    }

    /**
     * Returns the number of cache misses, which is the number of times {@link #cache} was called
     * and returned null (after caching the object), effectively representing the size of the cache. 
     *
     * @return the number of cache misses
     */
    public int getMisses() {
        return map.size();
    }

}

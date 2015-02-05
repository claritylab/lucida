package edu.cmu.sphinx.util;

import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * A custom thread factory, able to create threads with custom name prefix, daemon-ness and priority.
 * Based on java.util.concurrent.Executors.DefaultThreadFactory.
 *
 * @author Yaniv Kunda
 * @see java.util.concurrent.Executors.DefaultThreadFactory
 */
public class CustomThreadFactory implements ThreadFactory {
    static final AtomicInteger poolNumber = new AtomicInteger(1);
    final ThreadGroup group;
    final AtomicInteger threadNumber = new AtomicInteger(1);
    final String namePrefix;
    final boolean daemon;
    final int priority;

    public CustomThreadFactory(String namePrefix, boolean daemon, int priority) {
        if (priority > Thread.MAX_PRIORITY || priority < Thread.MIN_PRIORITY)
            throw new IllegalArgumentException("illegal thread priority");
        SecurityManager s = System.getSecurityManager();
        this.group = s != null ? s.getThreadGroup() : Thread.currentThread().getThreadGroup();
        this.namePrefix = namePrefix + "-" + poolNumber.getAndIncrement() + "-thread-";
        this.daemon = daemon;
        this.priority = priority;
    }

    public Thread newThread(Runnable r) {
        Thread t = new Thread(group, r, namePrefix + threadNumber.getAndIncrement(), 0);
        if (t.isDaemon() != daemon)
            t.setDaemon(daemon);
        if (t.getPriority() != priority)
            t.setPriority(priority);
        return t;
    }
}

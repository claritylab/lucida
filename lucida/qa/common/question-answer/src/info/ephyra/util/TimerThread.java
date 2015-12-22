package info.ephyra.util;

/**
 * A <code>TimerThread</code> interrupts another thread after a timeout has
 * occurred.
 * 
 * @author Nico Schlaefer
 * @version 2007-02-03
 */
public class TimerThread extends Thread {
	/** Timeout in milliseconds. */
	private long timeout;
	/** Threat to be interrupted. */
	private Thread threadToTimeout;
	
	/**
	 * Sets the timeout and starts the timer.
	 * 
	 * @param timeout the timeout in milliseconds
	 * @param threadToTimeout the thread to be interrupted
	 */
	public TimerThread(long timeout, Thread threadToTimeout) {
		this.timeout = timeout;
		this.threadToTimeout = threadToTimeout;
		
		start();
	}
	
	/**
	 * Waits for the time specified in <code>timeout</code> and then interrupts
	 * <code>threadToTimeout</code>.
	 */
	public void run() {
		// wait for timeout or until notification
		try {
			synchronized (threadToTimeout) {
				threadToTimeout.wait(timeout);
			}
		} catch (InterruptedException e) {}
		// stop thread if it is still running
		threadToTimeout.interrupt();
	}
}

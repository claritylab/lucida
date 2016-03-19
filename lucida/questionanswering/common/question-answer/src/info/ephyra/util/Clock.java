package info.ephyra.util;

/**
 * Implementation of a stop watch.
 * 
 * @author Nico Schlaefer
 * @version 2004-11-04
 */
public class Clock {
	/** Is the clock running? */
	private static boolean isRunning;
	/** Starting time. */
	private static long startTime;
	/** Stop time. */
	private static long stopTime;
	
	/** Starts the clock. */
	public static void start() {
		if (!isRunning) {
			startTime = System.currentTimeMillis();
			isRunning = true;
		}
	}
	
	/** Stops the clock. */
	public static void stop() {
		if (isRunning) {
			stopTime = System.currentTimeMillis();
			isRunning = false;
		}
	}
	
	/**
	 * Returns the time that has passed.
	 * 
	 * @return time difference
	 */
	public static long getTime() {
		if (isRunning || (startTime == 0) || (stopTime == 0)) return 0;
		
		return (stopTime - startTime);
	}
}

package edu.cmu.sphinx.util;

import static java.lang.String.format;


public class Preconditions {

    /**
     * Checks conditions and throws {@link IllegalArgumentException} if it is
     * not satisfied.
     * 
     * The error message may contain the same formatting characters as
     * supported by @{link {@link String#format(String, Object...)}.
     * 
     * @param condition condition to check
     * @param message error message that is passed to exception
     * @param arguments arguments for the format string
     * 
     * @throws IllegalArgumentException if condition is false
     */
    public static void checkArgument(boolean condition,
                                     String message,
                                     Object... arguments) {
        if (!condition)
            throw new IllegalArgumentException(format(message, arguments));
    }

    /**
     * Checks conditions and throws {@link IllegalStateException} if it is
     * not satisfied.
     * 
     * The error message may contain the same formatting characters as
     * supported by @{link {@link String#format(String, Object...)}.
     * 
     * @param condition condition to check
     * @param message error message that is passed to exception
     * @param arguments arguments for the format string
     * 
     * @throws IllegalStateException if condition is false
     */
    public static void checkState(boolean condition,
                                  String message,
                                  Object... arguments) {
        if (!condition)
            throw new IllegalStateException(format(message, arguments));
    }
}
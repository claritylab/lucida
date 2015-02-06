package edu.cmu.sphinx.util;

import org.testng.annotations.Test;


public class PreconditionsTest {

    @Test(expectedExceptions = IllegalArgumentException.class,
          expectedExceptionsMessageRegExp = "bar 42 baz")
    public void testThrowsArgumentException() {
        Preconditions.checkArgument(false, "%s %d baz", "bar", 42);
    }

    @Test(expectedExceptions = IllegalStateException.class,
          expectedExceptionsMessageRegExp = "foo bar 42")
    public void testThrowsStateException() {
        Preconditions.checkState(false, "foo %s %d", "bar", 42);
    }
}

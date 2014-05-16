package edu.cmu.sphinx.util.props;

import java.lang.annotation.*;

/**
 * An integer property.
 *
 * @author Holger Brandl
 * @see ConfigurationManager
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
@S4Property
public @interface S4Integer {

    /**
     * Default value to return
     */
    public static final int NOT_DEFINED = -918273645;

    int defaultValue() default NOT_DEFINED;

    int[] range() default {-Integer.MAX_VALUE, Integer.MAX_VALUE};


    boolean mandatory() default true;
}

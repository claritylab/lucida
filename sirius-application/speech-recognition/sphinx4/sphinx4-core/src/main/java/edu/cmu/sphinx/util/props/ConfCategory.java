package edu.cmu.sphinx.util.props;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * An annotation which can be used to tag classes. Based on these tags classes will be sorted into module categories
 * within the graphical system designer.
 *
 * @author Holger Brandl
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
public @interface ConfCategory {

    String[] value();
}

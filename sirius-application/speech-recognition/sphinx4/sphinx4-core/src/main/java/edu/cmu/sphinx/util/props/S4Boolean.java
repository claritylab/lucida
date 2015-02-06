package edu.cmu.sphinx.util.props;

import java.lang.annotation.*;

/**
 * A logical property.
 *
 * @author Holger Brandl
 * @see ConfigurationManager
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
@S4Property
public @interface S4Boolean {

    boolean defaultValue();

}

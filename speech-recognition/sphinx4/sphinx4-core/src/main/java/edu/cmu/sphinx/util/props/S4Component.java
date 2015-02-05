package edu.cmu.sphinx.util.props;

import java.lang.annotation.*;

/**
 * A component property.
 *
 * @author Holger Brandl
 * @see ConfigurationManager
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
@S4Property
public @interface S4Component {

    Class<? extends Configurable> type();


    Class<? extends Configurable> defaultClass() default Configurable.class;


    boolean mandatory() default true;
}

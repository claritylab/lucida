package edu.cmu.sphinx.util.props;

import java.lang.annotation.*;

/**
 * A list property.
 *
 * @author Holger Brandl
 * @see ConfigurationManager
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
@S4Property
public @interface S4ComponentList {

    Class<? extends Configurable> type();


    /**
     * A default list of <code>Configurable</code>s used to configure this component list given the case that no
     * component list was defined (via xml or during runtime).
     */
    Class<? extends Configurable>[] defaultList() default {};


    /**
     * If this flag is set the <code>ConfigurationManager</code> will not fail if some elements of the list couldn't be
     * instantitated.
     */
    boolean beTolerant() default false;
}

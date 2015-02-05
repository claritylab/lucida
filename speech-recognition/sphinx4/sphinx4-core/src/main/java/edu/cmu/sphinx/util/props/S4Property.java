package edu.cmu.sphinx.util.props;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * A tag which superclasses all sphinx property annotations. Because there is no real inheritance for annotations all
 * child classes are annotated by this general property annotation.
 *
 * @author Holger Brandl
 * @see S4Component
 * @see S4Integer
 * @see S4ComponentList
 * @see S4Double
 * @see S4Boolean
 * @see S4String
 */
@Retention(RetentionPolicy.RUNTIME)
public @interface S4Property {

}

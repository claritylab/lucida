package edu.cmu.sphinx.util.props;

import java.lang.annotation.Annotation;

/**
 * Wraps annotations
 *
 * @author Holger Brandl
 */
public class S4PropWrapper {

    private final Annotation annotation;


    public S4PropWrapper(Annotation annotation) {
        this.annotation = annotation;
    }


    public Annotation getAnnotation() {
        return annotation;
    }
}

/*
 * Created on Mar 4, 2005
 */
package edu.cmu.sphinx.linguist.language.classes;

/**
 * Represents a probability of a word belonging to class.
 *
 * @author Tanel Alumae
 */
class ClassProbability {

    private final String className;
    private final float logProbability;

    /**
     * @param className      Name of the class
     * @param logProbability Log probability
     */
    public ClassProbability(String className, float logProbability) {
        this.className = className;
        this.logProbability = logProbability;
    }

    public String getClassName() {
        return className;
    }

    public float getLogProbability() {
        return logProbability;
    }
}
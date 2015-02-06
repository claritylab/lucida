package edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi;


/**
 * Decision tree.
 */
public interface EventMap {

    /**
     * Maps speech unit to probability distribution function.
     *
     * @param pdfClass pdf-class
     * @param context  context
     *
     * @return identifier of probability distribution function
     */
    public int map(int pdfClass, int[] context);
}

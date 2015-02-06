package edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi;


/**
 * Stores a single answer.
 *
 * This is always a leaf node in decision tree.
 */
public class ConstantEventMap implements EventMap {

    private final int value;

    /**
     * Constructs a constant event map that returns one value.
     * 
     * @param   value returned value
     */
    public ConstantEventMap(int value) {
        this.value = value;
    }

    /**
     * @param  pdfClass is not used
     * @param  context  is not used
     *
     * @return stored value
     */
    public int map(int pdfClass, int[] context) {
        return value;
    }
}

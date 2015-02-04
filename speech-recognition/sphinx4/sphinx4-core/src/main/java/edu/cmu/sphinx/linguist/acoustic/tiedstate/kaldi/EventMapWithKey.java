package edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi;


/**
 * Event map that splits on a key.
 */
public abstract class EventMapWithKey implements EventMap {

    protected final int key;

    /**
     * Constructs new event map.
     *
     * @param key key to split on
     */
    protected EventMapWithKey(int key) {
        this.key = key;
    }

    /**
     * Returns value of the given context for the key.
     *
     * Convenient method to retrieve value for the key.
     *
     * @param pdfClass pdf-class
     * @param context  context
     *
     * @return phone ID for non-negative values of the key and pdf-class if the
     *         key equals -1
     */
    protected int getKeyValue(int pdfClass, int[] context) {
        return -1 == key ? pdfClass : context[key];
    }
}

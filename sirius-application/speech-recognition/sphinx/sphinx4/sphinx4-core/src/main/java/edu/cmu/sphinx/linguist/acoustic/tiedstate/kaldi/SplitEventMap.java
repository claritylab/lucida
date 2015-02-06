package edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;


/**
 * Binary decision tree.
 *
 * Splits on a certain key and goes to the "yes" or "no" child node depending
 * on the answer. Its Map function calls the Map function of the appropriate
 * child node. It stores a set of integers of type kAnswerType that correspond
 * to the "yes" child (everything else goes to "no").
 */
public class SplitEventMap extends EventMapWithKey {

    private final Set<Integer> values;
    private final EventMap yesMap;
    private final EventMap noMap;

    /**
     * Constructs new event map.
     *
     * @param key     key to split on
     * @param values yes values
     * @param yesMap  event map for "yes" answer
     * @param noMap   eventMap for no answer
     */
    public SplitEventMap(int key,
                         Collection<Integer> values,
                         EventMap yesMap, EventMap noMap)
    {
        super(key);
        this.values = new HashSet<Integer>(values);
        this.yesMap = yesMap;
        this.noMap = noMap;
    }

    /**
     * Maps given context to probability distribution function.
     *
     * @param context phonetic context
     *
     * @return identifier of probability distribution function.
     */
    public int map(int pdfClass, int[] context) {
        return values.contains(getKeyValue(pdfClass, context)) ?
               yesMap.map(pdfClass, context) : noMap.map(pdfClass, context);
    }
}


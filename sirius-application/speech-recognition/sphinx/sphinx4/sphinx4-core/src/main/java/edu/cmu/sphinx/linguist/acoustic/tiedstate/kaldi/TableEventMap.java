package edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi;

import java.util.ArrayList;
import java.util.List;


public class TableEventMap extends EventMapWithKey {

    private final List<EventMap> table;

    /**
     * Constructs new table event map.
     *
     * @param key key to split on
     */
    public TableEventMap(int key, List<EventMap> table) {
        super(key);
        this.table = new ArrayList<EventMap>(table);
    }

    /**
     *
     */
    public int map(int pdfClass, int[] context) {
        EventMap eventMap = table.get(getKeyValue(pdfClass, context));
        return eventMap.map(pdfClass, context);
    }
}

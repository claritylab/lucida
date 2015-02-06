package edu.cmu.sphinx.frontend.databranch;

import edu.cmu.sphinx.frontend.BaseDataProcessor;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DataProcessingException;
import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4ComponentList;

import java.util.ArrayList;
import java.util.List;

/**
 * Creates push-branches out of a Frontend. This might be used for for push-decoding or to create new pull-streams
 *
 * @see edu.cmu.sphinx.decoder.FrameDecoder
 * @see edu.cmu.sphinx.frontend.databranch.DataBufferProcessor
 */
public class FrontEndSplitter extends BaseDataProcessor implements DataProducer {


    @S4ComponentList(type = Configurable.class, beTolerant = true)
    public static final String PROP_DATA_LISTENERS = "dataListeners";
    private List<DataListener> listeners = new ArrayList<DataListener>();

    public FrontEndSplitter() {
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        listeners = ps.getComponentList(PROP_DATA_LISTENERS, DataListener.class);
    }


    /**
     * Reads and returns the next Data frame or return <code>null</code> if no data is available.
     *
     * @return the next Data or <code>null</code> if none is available
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     *          if there is a data processing error
     */
    @Override
    public Data getData() throws DataProcessingException {
        Data input = getPredecessor().getData();

        for (DataListener l : listeners)
            l.processDataFrame(input);

        return input;
    }


    public void addDataListener(DataListener l) {
        if (l == null) {
            return;
        }
        listeners.add(l);
    }


    public void removeDataListener(DataListener l) {
        if (l == null) {
            return;
        }
        listeners.remove(l);
    }
}


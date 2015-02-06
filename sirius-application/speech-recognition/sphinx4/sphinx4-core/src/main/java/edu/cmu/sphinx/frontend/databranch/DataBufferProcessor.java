package edu.cmu.sphinx.frontend.databranch;

import edu.cmu.sphinx.frontend.BaseDataProcessor;
import edu.cmu.sphinx.frontend.Data;
import edu.cmu.sphinx.frontend.DataProcessingException;
import edu.cmu.sphinx.util.props.*;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;

/**
 * A FIFO-buffer for <code>Data</code>-elements.
 * <p/>
 * <code>Data</code>s are inserted to the buffer using the <code>processDataFrame</code>-method.
 */
public class DataBufferProcessor extends BaseDataProcessor implements DataListener {

    /** The FIFO- data buffer. */
    private final List<Data> featureBuffer = new LinkedList<Data>();

    /**
     * If this property is set <code>true</code> the buffer will wait for new data until it returns from a
     * <code>getData</code>-call. Enable this flag if the buffer should serve as starting point for a new
     * feature-pull-chain..
     */
    @S4Boolean(defaultValue = false)
    public static final String PROP_WAIT_IF_EMPTY = "waitIfEmpty";
    private boolean waitIfEmpty;

    /**
     * The time in milliseconds which will be waited between two attempts to read a data element from the buffer when
     * being in <code>waitIfEmpty</code>-mode
     */
    @S4Integer(defaultValue = 10)
    public static final String PROP_WAIT_TIME_MS = "waitTimeMs";
    private long waitTime;


    /** The maximal size of the buffer in frames. The oldest frames will be removed if the buffer grows out of bounds. */
    @S4Integer(defaultValue = 50000)
    public static final String PROP_BUFFER_SIZE = "maxBufferSize";
    private int maxBufferSize;


    @S4ComponentList(type = Configurable.class, beTolerant = true)
    public static final String DATA_LISTENERS = "dataListeners";
    private List<DataListener> dataListeners = new ArrayList<DataListener>();

    /**
     * @param maxBufferSize The maximal size of the buffer in frames. The oldest frames will be removed if the buffer grows out of bounds.
     * @param waitIfEmpty If this property is set <code>true</code> the buffer will wait for new data until it returns from a
     * <code>getData</code>-call. Enable this flag if the buffer should serve as starting point for a new
     * feature-pull-chain.
     * @param waitTime The time in milliseconds which will be waited between two attempts to read a data element from the buffer when
     * being in <code>waitIfEmpty</code>-mode
     * @param listeners
     */
    public DataBufferProcessor(int maxBufferSize, boolean waitIfEmpty, int waitTime, List<? extends Configurable> listeners) {
        initLogger();
        
        this.maxBufferSize = maxBufferSize;
        this.waitIfEmpty = waitIfEmpty;

        if (waitIfEmpty) // if false we don't need the value
            this.waitTime = waitTime;

        for (Configurable configurable : listeners) {
            assert configurable instanceof DataListener;
            addDataListener((DataListener) configurable);
        }
    }

    public DataBufferProcessor() {
    }

    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        maxBufferSize = ps.getInt(PROP_BUFFER_SIZE);
        waitIfEmpty = ps.getBoolean(PROP_WAIT_IF_EMPTY);

        if (waitIfEmpty) // if false we don't need the value
            waitTime = ps.getInt(PROP_WAIT_TIME_MS);

        dataListeners = ps.getComponentList(DATA_LISTENERS, DataListener.class);
    }


    public void processDataFrame(Data data) {
        featureBuffer.add(data);

        // inform data-listeners if necessary
        for (DataListener dataListener : dataListeners) {
            dataListener.processDataFrame(data);
        }

        //reduce the buffer-size if necessary
        while (featureBuffer.size() > maxBufferSize) {
            featureBuffer.remove(0);
        }
    }


    /**
     * Returns the processed Data output.
     *
     * @return an Data object that has been processed by this DataProcessor
     * @throws edu.cmu.sphinx.frontend.DataProcessingException
     *          if a data processor error occurs
     */
    @Override
    public Data getData() throws DataProcessingException {
        Data data = null;

        while (waitIfEmpty && featureBuffer.isEmpty()) {
            try {
                Thread.sleep(waitTime);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        if (!featureBuffer.isEmpty()) {
            data = featureBuffer.remove(0);
        } else {
            assert !waitIfEmpty;
        }

        return data;
    }


    public int getBufferSize() {
        return featureBuffer.size();
    }


    public void clearBuffer() {
        featureBuffer.clear();
    }


    public List<Data> getBuffer() {
        return Collections.unmodifiableList(featureBuffer);
    }


    /** Adds a new listener.
     * @param l*/
    public void addDataListener(DataListener l) {
        if (l == null)
            return;

        dataListeners.add(l);
    }


    /** Removes a listener.
     * @param l*/
    public void removeDataListener(DataListener l) {
        if (l == null)
            return;

        dataListeners.remove(l);
    }

}

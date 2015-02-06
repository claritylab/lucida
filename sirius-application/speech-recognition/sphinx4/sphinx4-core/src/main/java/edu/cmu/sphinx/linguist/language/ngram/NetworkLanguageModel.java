package edu.cmu.sphinx.linguist.language.ngram;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.URL;
import java.util.HashSet;
import java.util.Set;

import edu.cmu.sphinx.linguist.WordSequence;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.linguist.util.LRUCache;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.ConfigurationManagerUtils;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4String;
import edu.cmu.sphinx.util.props.S4Integer;

/*
 * The client of the SRILM language model server. It needs to
 * read the vocabulary from a vocabulary file though.
 */
public class NetworkLanguageModel implements LanguageModel {

    /** The property specifying the host of the language model server. */
    @S4String(defaultValue = "localhost")
    public final static String PROP_HOST = "host";

    /** The property specifying the port of the language model server. */
    @S4Integer(defaultValue = 2525)
    public final static String PROP_PORT = "port";

    LogMath logMath;

    private String host;
    private int port;
    private URL location;
    int maxDepth;

    Socket socket;
    private BufferedReader inReader;
    private PrintWriter outWriter;
    LRUCache<WordSequence, Float> cache;

    private boolean allocated;

    /**
     * Creates network language model client
     * 
     * @param host
     *            server host
     * @param port
     *            server port
     * @param location
     *            URL of the file with vocabulary (only needed for 1-stage
     *            model)
     * @param maxDepth
     *            depth of the model
     * @param logMath
     *            logMath
     */
    public NetworkLanguageModel(String host, int port, URL location, int maxDepth) {
        this.host = host;
        this.port = port;
        this.maxDepth = maxDepth;
        this.location = location;
        logMath = LogMath.getInstance();
    }

    public NetworkLanguageModel() {
    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util
     * .props.PropertySheet)
     */
    public void newProperties(PropertySheet ps) throws PropertyException {

        if (allocated) {
            throw new RuntimeException("Can't change properties after allocation");
        }
        host = ps.getString(PROP_HOST);
        port = ps.getInt(PROP_PORT);
        location = ConfigurationManagerUtils.getResource(PROP_LOCATION, ps);

        maxDepth = ps.getInt(PROP_MAX_DEPTH);
        if (maxDepth == -1)
            maxDepth = 3;
    }

    public void allocate() throws IOException {
        allocated = true;

        socket = new Socket(host, port);
        inReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        outWriter = new PrintWriter(socket.getOutputStream(), true);
        String greeting = inReader.readLine();
        if (!greeting.equals("probserver ready")) {
            throw new IOException("Incorrect input");
        }
        cache = new LRUCache<WordSequence, Float>(1000);
    }

    public void deallocate() {
        allocated = false;
        try {
            socket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public int getMaxDepth() {
        return maxDepth;
    }

    public float getProbability(WordSequence wordSequence) {

        Float probability = cache.get(wordSequence);

        if (probability != null) {
            return probability.floatValue();
        }

        StringBuilder builder = new StringBuilder();
        if (wordSequence.size() == 0)
            return 0.0f;
        for (Word w : wordSequence.getWords()) {
            builder.append(w.toString());
            builder.append(' ');
        }
        outWriter.println(builder.toString());
        String result = "0";
        try {
            result = inReader.readLine();
            if (result.charAt(0) == 0)
                result = result.substring(1);
        } catch (IOException e) {
            e.printStackTrace();
        }

        if (!result.equals("-inf"))
            probability = logMath.log10ToLog(Float.parseFloat(result));
        else
            probability = LogMath.LOG_ZERO;

        cache.put(wordSequence, probability);
        return probability.floatValue();
    }

    public float getSmear(WordSequence wordSequence) {
        return 0.0f;
    }

    public Set<String> getVocabulary() {
        Set<String> result = new HashSet<String>();
        try {
            BufferedReader reader = new BufferedReader(new InputStreamReader(location.openStream()));
            String line;
            while (true) {
                line = reader.readLine();
                if (line == null)
                    break;
                if (line.length() == 0)
                    continue;
                result.add(line.trim());
            }
            reader.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return result;
    }

    public void start() {
    }

    public void stop() {
    }
}

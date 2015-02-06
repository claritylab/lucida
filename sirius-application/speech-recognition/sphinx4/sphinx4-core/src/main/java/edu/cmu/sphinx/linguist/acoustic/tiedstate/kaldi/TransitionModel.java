package edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import edu.cmu.sphinx.util.LogMath;

final class HmmState {

    private final int id;
    private final int pdfClass;
    private final List<Integer> transitions;

    public HmmState(int id, int pdfClass, Collection<Integer> transitions) {
        this.id = id;
        this.pdfClass = pdfClass;
        this.transitions = new ArrayList<Integer>(transitions);
    }

    public int getId() {
        return id;
    }

    public int getPdfClass() {
        return pdfClass;
    }

    public List<Integer> getTransitions() {
        return transitions;
    }

    public int size() {
        return transitions.size();
    }

    @Override
    public String toString() {
        return String.format("HmmSate {%d, %d, %s}",
                             id, pdfClass, transitions);
    }
}

final class Triple {

    private int phone;
    private int hmmState;
    private int pdf;

    public Triple(int phone, int hmmState, int pdf) {
        this.phone = phone;
        this.hmmState = hmmState;
        this.pdf = pdf;
    }

    @Override
    public boolean equals(Object object) {
        if (!(object instanceof Triple))
            return false;

        Triple other = (Triple) object;

        return phone    == other.phone &&
               hmmState == other.hmmState &&
               pdf      == other.pdf;
    }

    @Override
    public int hashCode() {
        return 31 * (31 * phone + hmmState) + pdf;
    }

    @Override
    public String toString() {
        return String.format("Triple {%d, %d, %d}", phone, hmmState, pdf);
    }
}

/**
 * Represents transition model of a Kaldi acoustic model.
 */
public class TransitionModel {

    private Map<Integer, List<HmmState>> phoneStates;
    private Map<Triple, Integer> transitionStates;
    private float[] logProbabilities;

    /**
     * Loads transition model using provided parser.
     *
     * @param parser parser
     */
    public TransitionModel(KaldiTextParser parser) {
        parser.expectToken("<TransitionModel>");
        parseTopology(parser);

        parser.expectToken("<Triples>");
        transitionStates = new HashMap<Triple, Integer>();
        int numTriples = parser.getInt();
        int transitionId = 1;

        for (int i = 0; i < numTriples; ++i) {
            int phone = parser.getInt();
            int hmmState = parser.getInt();
            int pdf = parser.getInt();
            Triple triple = new Triple(phone, hmmState, pdf);
            transitionStates.put(triple, transitionId);
            transitionId +=
                phoneStates.get(phone).get(hmmState).getTransitions().size();
        }

        parser.expectToken("</Triples>");
        parser.expectToken("<LogProbs>");
        logProbabilities = parser.getFloatArray();
        parser.expectToken("</LogProbs>");
        parser.expectToken("</TransitionModel>");

        LogMath logMath = LogMath.getInstance();
        for (int i = 0; i < logProbabilities.length; ++i)
            logProbabilities[i] = logMath.lnToLog(logProbabilities[i]);
    }

    private void parseTopology(KaldiTextParser parser) {
        parser.expectToken("<Topology>");

        phoneStates = new HashMap<Integer, List<HmmState>>();
        String token;

        while ("<TopologyEntry>".equals(token = parser.getToken())) {
            parser.assertToken("<TopologyEntry>", token);
            parser.expectToken("<ForPhones>");

            List<Integer> phones = new ArrayList<Integer>();
            while (!"</ForPhones>".equals(token = parser.getToken()))
                phones.add(Integer.parseInt(token));

            List<HmmState> states = new ArrayList<HmmState>(3);
            while ("<State>".equals(token = parser.getToken())) {
                // Skip state number.
                int id = parser.getInt();
                token = parser.getToken();

                if ("<PdfClass>".equals(token)) {
                    int pdfClass = parser.getInt();
                    List<Integer> transitions = new ArrayList<Integer>();
                    while ("<Transition>".equals(token = parser.getToken())) {
                        transitions.add(parser.getInt());
                        // Skip initial probability.
                        parser.getToken();
                    }

                    parser.assertToken("</State>", token);
                    states.add(new HmmState(id, pdfClass, transitions));
                }
            }

            for (Integer id : phones)
                phoneStates.put(id, states);
        }

        parser.assertToken("</Topology>", token);
    }

    /**
     * Returns transition matrix for the given context.
     *
     * @param phone central phone in the context
     * @param pdfs  array of pdf identifiers of the context units
     *
     * @return
     * 4 by 4 matrix where cell i,j contains probability in {@link LogMath}
     * domain of transition from state i to state j
     */
    public float[][] getTransitionMatrix(int phone, int[] pdfs) {
        // TODO: use variable size
        float[][] transitionMatrix = new float[4][4];
        Arrays.fill(transitionMatrix[3], LogMath.LOG_ZERO);

        for (HmmState state : phoneStates.get(phone)) {
            int stateId = state.getId();
            Arrays.fill(transitionMatrix[stateId], LogMath.LOG_ZERO);
            Triple triple = new Triple(phone, stateId, pdfs[stateId]);
            int i = transitionStates.get(triple);

            for (Integer j : state.getTransitions())
                transitionMatrix[stateId][j] = logProbabilities[i++];
        }

        return transitionMatrix;
    }
}

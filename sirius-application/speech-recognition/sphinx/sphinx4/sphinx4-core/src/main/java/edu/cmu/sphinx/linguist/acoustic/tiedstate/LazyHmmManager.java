package edu.cmu.sphinx.linguist.acoustic.tiedstate;

import java.util.ArrayList;
import java.util.Collection;
import java.util.InputMismatchException;
import java.util.List;
import java.util.Map;

import edu.cmu.sphinx.linguist.acoustic.HMM;
import edu.cmu.sphinx.linguist.acoustic.HMMPosition;
import edu.cmu.sphinx.linguist.acoustic.LeftRightContext;
import edu.cmu.sphinx.linguist.acoustic.Unit;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi.ConstantEventMap;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi.EventMap;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi.KaldiTextParser;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi.SplitEventMap;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi.TableEventMap;
import edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi.TransitionModel;

/**
 * {@link HMMManager} extension to load HMMs from Kaldi model.
 *
 * Initially empty this class creates HMMs on request.
 */
public class LazyHmmManager extends HMMManager {

    private final EventMap eventMap;
    private final Pool<Senone> senonePool;
    private final Map<String, Integer> symbolTable;
    private final TransitionModel transitionModel;

    public LazyHmmManager(KaldiTextParser parser,
                          TransitionModel transitionModel,
                          Pool<Senone> senonePool,
                          Map<String, Integer> symbolTable)
    {
        this.transitionModel = transitionModel;
        this.senonePool = senonePool;
        this.symbolTable = symbolTable;

        parser.expectToken("ContextDependency");
        parser.getInt();
        parser.getInt();
        parser.expectToken("ToPdf");
        eventMap = parseEventMap(parser);
        parser.expectToken("EndContextDependency");
    }

    private EventMap parseEventMap(KaldiTextParser parser) {
        String token = parser.getToken();

        if ("CE".equals(token))
            return new ConstantEventMap(parser.getInt());

        if ("SE".equals(token))
            return parseSplitEventMap(parser);

        if ("TE".equals(token))
            return parseTableEventMap(parser);

        if ("NULL".equals(token))
            return null;

        throw new InputMismatchException(token);
    }

    private EventMap parseSplitEventMap(KaldiTextParser parser) {
        int key = parser.getInt();
        Collection<Integer> values;
        values = new ArrayList<Integer>();
        for (Integer n : parser.getIntArray())
            values.add(n);

        parser.expectToken("{");
        EventMap yesMap = parseEventMap(parser);
        EventMap noMap = parseEventMap(parser);
        EventMap eventMap = new SplitEventMap(key, values, yesMap, noMap);
        parser.expectToken("}");

        return eventMap;
    }

    private EventMap parseTableEventMap(KaldiTextParser parser) {
        int key = parser.getInt();
        int size = parser.getInt();
        List<EventMap> table = new ArrayList<EventMap>(size);
        
        parser.expectToken("(");

        while (0 < size--)
            table.add(parseEventMap(parser));

        parser.expectToken(")");
        return new TableEventMap(key, table);
    }

    @Override
    public HMM get(HMMPosition position, Unit unit) {
        HMM hmm = super.get(position, unit);
        if (null != hmm) return hmm;

        int[] ids = new int[3];
        ids[1] = symbolTable.get(unit.getName());

        if (unit.isContextDependent()) {
            LeftRightContext context = (LeftRightContext) unit.getContext();
            Unit left = context.getLeftContext()[0];
            Unit right = context.getRightContext()[0];
            ids[0] = symbolTable.get(left.getName());
            ids[2] = symbolTable.get(right.getName());
        } else {
            ids[0] = symbolTable.get("SIL");
            ids[2] = symbolTable.get("SIL");
        }

        int[] pdfs = new int[3];
        pdfs[0] = eventMap.map(0, ids);
        pdfs[1] = eventMap.map(1, ids);
        pdfs[2] = eventMap.map(2, ids);

        Senone[] senones = new Senone[3];
        senones[0] = senonePool.get(pdfs[0]);
        senones[1] = senonePool.get(pdfs[1]);
        senones[2] = senonePool.get(pdfs[2]);
        SenoneSequence ss = new SenoneSequence(senones);

        float[][] transitionMatrix;
        transitionMatrix = transitionModel.getTransitionMatrix(ids[1], pdfs);
        hmm = new SenoneHMM(unit, ss, transitionMatrix, position);
        put(hmm);

        return hmm;
    }
}

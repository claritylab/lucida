package info.ephyra.questionanalysis.atype.classifier;

import info.ephyra.questionanalysis.atype.AnswerType;
import info.ephyra.questionanalysis.atype.QuestionClassifier;
import info.ephyra.util.Properties;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import edu.cmu.minorthird.classify.ClassLabel;
import edu.cmu.minorthird.classify.Classifier;
import edu.cmu.minorthird.classify.Instance;
import edu.cmu.minorthird.util.IOUtil;

/**
 * A trained model-based classifier for the answer type of a question.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class TrainedQuestionClassifier extends QuestionClassifier
{
    private Classifier classifier;

    public TrainedQuestionClassifier() {}

    public void initialize() throws Exception {
        // return if already initialized
        if (isInitialized()) return;
        if (languagePair == null)
            throw new Exception("languagePair must be set before calling initialize");
        super.initialize();
        Properties properties = Properties.loadFromClassName(this.getClass().getName());
        
        properties = properties.mapProperties().get(languagePair.getFirst()+"_"+languagePair.getSecond());
                
        String classifierFileName = properties.get("classifierFile");
        if (classifierFileName == null)
            throw new RuntimeException("Required property classifierFile is undefined'"); 
        classifier = (Classifier) IOUtil.loadSerialized(new File(
                classifierFileName));

        setInitialized(true);
    }

    public List<AnswerType> classify(Instance instance) {
        List<AnswerType> res = new ArrayList<AnswerType>();
        ClassLabel label = null;
        synchronized (classifier) {
            label = classifier.classification(instance);
        }
        String labelStr = label.bestClassName().replaceAll("-",".");
        AnswerType atype = AnswerType.constructFromString(labelStr);
        double weight = label.bestWeight();
        if (weight > 1.0) weight = (1 - (1 / weight));
        atype.setConfidence(weight);
        res.add(atype);
        return res;
    }
}

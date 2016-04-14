package info.ephyra.questionanalysis.atype.classifier; 

import info.ephyra.questionanalysis.atype.AnswerType;
import info.ephyra.questionanalysis.atype.QuestionClassifier;
import info.ephyra.util.Properties;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import edu.cmu.minorthird.classify.Instance;

/**
 * The answer type / subtype classifier for English questions.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class HybridQuestionClassifier extends QuestionClassifier
{
    Comparator<AnswerType> atypeComparator = new Comparator<AnswerType>(){
        public int compare(AnswerType o1,AnswerType o2){
            double val = o2.getConfidence() - o1.getConfidence();
            if(val == 0)
                return 0;
            if(val > 0)
                return 1;
            else
                return -1;
        }
    };
    
    //private static final Logger log = Logger.getLogger(HybridQuestionClassifier.class);
    private List<QuestionClassifier> classifiers;
    private boolean mergeResults;
    private double confidenceThreshold;

    public HybridQuestionClassifier() {}
    
    public void initialize() throws Exception
    {
        // return if already initialized
        if (isInitialized()) return;
        if (languagePair == null)
            throw new Exception("languagePair must be set before calling initialize");
        super.initialize();

        Properties properties = Properties.loadFromClassName(this.getClass().getName());
        
        properties = properties.mapProperties().get(languagePair.getFirst()+"_"+languagePair.getSecond());
        
        String classifierClassNames = properties.get("classifierTypes").trim();
        if (classifierClassNames == null)
            throw new RuntimeException("Required property classifierTypes is undefined");
        String[] classNames = classifierClassNames.split(",");
        
        classifiers = new ArrayList<QuestionClassifier>();
        for (String className : classNames) {
            QuestionClassifier classifier = (QuestionClassifier)Class.forName(className).newInstance();
            classifier.setLanguagePair(languagePair);
            classifier.initialize();
            classifiers.add(classifier);
        }

        String mergeResultsStr = properties.get("mergeResults").trim();
        if (mergeResultsStr == null)
            throw new RuntimeException("Required property mergeResults is undefined"); 
        mergeResults = Boolean.parseBoolean(mergeResultsStr);
        
        String confidenceThresholdStr = properties.get("confidenceThreshold").trim();
        if (mergeResultsStr == null)
            throw new RuntimeException("Required property confidenceThreshold is undefined"); 
        confidenceThreshold = Double.parseDouble(confidenceThresholdStr);
        
        setInitialized(true);
    }
        
    public List<AnswerType> classify(Instance instance)
    {
        List<AnswerType> res = new ArrayList<AnswerType>();
        
        for (QuestionClassifier classifier : classifiers) {
            List<AnswerType> classifierResult = classifier.classify(instance);
            if (classifierResult != null && classifierResult.size() > 0
                 && !classifierResult.get(0).getType().equals("NOANS")) {
                if (!mergeResults) return classifierResult;
                for (AnswerType atype : classifierResult) 
                    if (!res.contains(atype)) res.add(atype);
            } 
        }
        Collections.sort(res, atypeComparator);
        for (int i = 0; i < res.size(); i++) {
            if (res.get(i).getConfidence() < confidenceThreshold) {
                res.remove(i);
                i--;
            }
        }
        if (res.size() == 0)
            res.add(AnswerType.constructFromString("NONE"));
        return res;
    }
}

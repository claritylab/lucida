package info.ephyra.questionanalysis.atype; 

import info.ephyra.questionanalysis.atype.extractor.FeatureExtractor;
import info.ephyra.questionanalysis.atype.extractor.FeatureExtractorFactory;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.apache.log4j.Logger;

import edu.cmu.lti.javelin.evaluation.GoldStandard;
import edu.cmu.lti.javelin.qa.Term;
import edu.cmu.lti.javelin.util.Language;
import edu.cmu.lti.util.Pair;
import edu.cmu.minorthird.classify.Instance;
import edu.cmu.minorthird.classify.MutableInstance;

/**
 * A class for classifying questions in terms of their expected answer type. 
 * This abstract class contains most of the accessor methods for getting a 
 * question's classification results, while the details of exactly how that 
 * classification is determined (see the 
 * {@link info.ephyra.questionanalysis.atype.QuestionClassifier#classify(Instance) classify}
 * method) is left to the subclasses.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public abstract class QuestionClassifier
{
    private static final Logger log = Logger.getLogger(QuestionClassifier.class);
    protected FeatureExtractor extractor;
    protected boolean isInitialized;
    protected Pair<Language, Language> languagePair;

    /**
     * Initializes the FeatureExtractor.
     * @throws Exception if one of the required input properties is not defined
     */
    public void initialize() throws Exception {
        if (languagePair == null)
            throw new Exception("languagePair must be set before calling the parent initialize");
        extractor = FeatureExtractorFactory.getInstance(languagePair.getFirst());
    }

    /**
     * @return the isInitialized
     */
    public boolean isInitialized() {
        return this.isInitialized;
    }

    /**
     * @param isInitialized the isInitialized to set
     */
    public void setInitialized(boolean isInitialized) {
        this.isInitialized = isInitialized;
    }

    /**
     * @param languagePair the languagePair to set
     */
    public void setLanguagePair(Pair<Language,Language> languagePair) {
        this.languagePair = languagePair;
    }

    /**
     * @return the languagePair
     */
    public Pair<Language,Language> getLanguagePair() {
        return this.languagePair;
    }

    /**
     * Classifies the question represented by the given List of Terms and parse tree 
     * as having a particular answer type and possibly subtype. 
     * 
     * @param terms the Terms that make up the question to classify
     * @param parseTreeStr the syntactic parse tree of the question, in String format
     * 
     * @return the candidate answer type / subtypes.
     * @throws Exception
     */
    public List<AnswerType> getAnswerTypes(List<Term> terms, String parseTreeStr) throws Exception {
        if(!isInitialized())
            throw new Exception("getAnswerTypes called while not initialized");

        String question = "";
        for (Term term : terms) question += term.getText()+" ";

        // create the instance
        Instance instance = new MutableInstance(question);
        if (extractor != null)
            instance = extractor.createInstance(terms,parseTreeStr);

        return classify(instance);
    }    

    /**
     * Classifies the question represented by the given String and parse tree
     * as having a particular answer type and possibly subtype. 
     * 
     * @param question the question to classify
     * @param parseTreeStr the syntactic parse tree of the question, in String format
     * 
     * @return the candidate answer type / subtypes.
     * @throws Exception
     */
    public List<AnswerType> getAnswerTypes(String question, String parseTreeStr) throws Exception {
        if(!isInitialized())
            throw new Exception("getAnswerTypes called while not initialized");

        String[] tokens = question.split("\\s+");
        List<Term> terms = new ArrayList<Term>();
        for (String token : tokens) {
            terms.add(new Term(0,0,token));
        }
        return getAnswerTypes(terms, parseTreeStr);
    }

    /**
     * Classifies the question represented by the given String
     * as having a particular answer type and possibly subtype. 
     * 
     * @param question the question to classify
     * 
     * @return the candidate answer type / subtypes.
     * @throws Exception
     */
    public List<AnswerType> getAnswerTypes(String question) throws Exception {
        if(!isInitialized())
            throw new Exception("getAnswerTypes called while not initialized");

        // create the instance
        Instance instance = new MutableInstance(question);
        if (extractor != null)
            instance = extractor.createInstance(question);

        return classify(instance);
    }

    /**
     * Classifies a question, represented by an Instance, in terms of its
     * expected answer type.  Multiple AnswerTypes may be returned, ranked
     * by their associated score, or probability of being ccrrect.
     * 
     * The source object associated with the Instance that is returned by 
     * Instance.getSource() must be the original question, in String form.  
     * 
     * @param instance the Instance to be classified.
     * @return a list of AnswerTypes, ranked by their associated 
     * score, or probability of being ccrrect.
     */
    public abstract List<AnswerType> classify(Instance instance);

    /**
     * Evaluates classification accuracy on a given test set.
     * 
     * @param testSetFileName the name of the file containing the test set to evaluate against
     * @throws Exception
     */
    public void evaluate (String testSetFileName) throws Exception {
        DecimalFormat format=new DecimalFormat("#0.00");
        int correct = 0;


        List<GoldStandard> goldStandards;
        goldStandards = GoldStandard.loadFile( testSetFileName );

        int size = goldStandards.size();
        
        //System.out.println(String.format("%18s / %-18s","Actual","Predicted"));
        for (GoldStandard gs : goldStandards) {

            String question = gs.getQuestion( languagePair.getFirst().name().split("_")[0].toUpperCase() );
            String actual = gs.getAnswerType();
            Set<AnswerType> actuals = new HashSet<AnswerType>();
            Set<AnswerType> predicted = new HashSet<AnswerType>();
            for (String atypeStr : actual.split("\\|")) {
                actuals.add(AnswerType.constructFromString(atypeStr));
            }
            List<AnswerType> atypes = getAnswerTypes( question );
            if (atypes.size() > 0) {
                double topConf = atypes.get(0).getConfidence();
                for (int i = 0; i < atypes.size(); i++) {
                    if (atypes.get(i).getConfidence() == topConf) {
                        predicted.add(atypes.get(i));
                        atypes.remove(i);
                        i--;
                    }
                }
            }
            boolean corr = false;
            for (AnswerType a : predicted) 
                if (actuals.contains(a)) corr = true;
            if (corr) correct++;
            else {
                log.debug("A: " + actuals + ", P: " + predicted + ", "+ question );
                if (atypes.size() > 0)
                    log.debug("        2nd: "+atypes.get(0));
            }
        }
        if (size > 0)
            System.out.println("Test set accuracy: " + correct + "/" + size + " (" +
                    format.format((double)correct/(double)size * 100) + "%)");
        else 
            System.out.println("No examples classified."); 
    }
    
    public void classifySet(String filename) {
        try {
            BufferedReader in = new BufferedReader( new FileReader( filename ) );
            String question = null;
            while ((question = in.readLine()) != null) {
                List<AnswerType> types = classify(extractor.createInstance(question));
                System.out.print(types + " ");
                System.out.println(question);
            }
            in.close();
        } catch (IOException e) {
            System.err.println("Error accessing file: " + filename);
        }
    }
    
}

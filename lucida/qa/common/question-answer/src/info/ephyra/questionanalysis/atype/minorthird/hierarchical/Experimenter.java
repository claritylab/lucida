package info.ephyra.questionanalysis.atype.minorthird.hierarchical;

import info.ephyra.util.Properties;

import java.io.FileInputStream;
import java.text.DecimalFormat;

import org.apache.log4j.Logger;

import edu.cmu.lti.javelin.util.FileUtil;
import edu.cmu.lti.javelin.util.Language;
import edu.cmu.lti.util.Pair;
import edu.cmu.minorthird.classify.experiments.Evaluation;

/**
 * A utility class for running experiments with different combinations of the 
 * property values required by {@link info.ephyra.questionanalysis.atype.minorthird.hierarchical.HierarchicalClassifierTrainer HierarchicalClassifierTrainer}.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class Experimenter{

    private static Logger log = Logger.getLogger(Experimenter.class);
    private String[] learningCombos;
    private String[] featureTypeCombos;
    private Properties properties;
    private Pair<Language,Language> languagePair;
    private boolean isInitialized;

 

    public Experimenter(Pair<Language,Language> languagePair) {
        this.languagePair = languagePair;
    }

    /**
     * The input properties that must be defined are:
     * <ul>
     *   <li> <code>learningCombos</code>  : &nbsp; A "|"-separated list of comma-separated 
     *   lists of learning algorithms.  The outer, "|"-separated list specifies the different hierarchical
     *   classifiers to experiment with.  The inner, comma-separated list of algorithms 
     *   specifies the structure of the hierarchical classifier to use for one 
     *   experiment.    
     *   The set of valid algorithm names is: 
     *   <ul>
     *      <li> <code>KNN</code>
     *      <li> <code>KWAY_MIX</code>
     *      <li> <code>MAX_ENT</code>
     *      <li> <code>BWINNOW_OVA</code>
     *      <li> <code>MPERCEPTRON_OVA </code>
     *      <li> <code>NBAYES_OVA</code>
     *      <li> <code>VPERCEPTRON_OVA</code>
     *      <li> <code>ADABOOST_OVA</code>
     *      <li> <code>ADABOOST_CB</code>
     *      <li> <code>ADABOOST_MFF</code>
     *      <li> <code>ADABOOSTL_OVA</code>
     *      <li> <code>ADABOOSTL_CB</code>
     *      <li> <code>ADABOOSTL_MFF</code>
     *      <li> <code>DTREE_OVA</code>
     *      <li> <code>DTREE_CB</code>
     *      <li> <code>DTREE_MFF</code>
     *      <li> <code>NEGBI_OVA</code>
     *      <li> <code>NEGBI_CB</code>
     *      <li> <code>NEGBI_MFF</code>
     *      <li> <code>SVM_OVA</code>
     *      <li> <code>SVM_CB</code>
     *      <li> <code>SVM_MFF</code>
     *   </ul>
     *   <li> <code>featureTypeCombos</code> : &nbsp; A "|"-separated list of comma-separated 
     *   lists of feature types. 
     * </ul>
     */
    public void initialize() throws Exception {
        if (isInitialized()) return;
        if (languagePair == null)
            throw new Exception("languagePair must be set before calling initialize");
        
        String propertiesFileName = System.getProperty("ephyra.home", ".")+"/conf/"+Experimenter.class.getName()+".properties";
        properties.load(new FileInputStream(propertiesFileName));
        properties = properties.mapProperties().get(languagePair.getFirst()+"_"+languagePair.getSecond());
        
        learningCombos = properties.getProperty("learningCombos").split("\\|");
        featureTypeCombos = properties.getProperty("featureTypeCombos").split("\\|");
        
        setInitialized(true);
    }

    /**
     * @return the isInitialized
     */
    public boolean isInitialized() {
        return isInitialized;
    }

    /**
     * @param isInitialized the isInitialized to set
     */
    public void setInitialized(boolean isInitialized) {
        this.isInitialized = isInitialized;
    }
    
    /**
     * Runs the experiments specified in the input properties file.  The set of 
     * experiments run is the cross-product of the sets of algorithm configurations, training/testing
     * dataset configurations, and feature type combinations.
     * 
     */
    public void runExperiments() {
        DecimalFormat format=new DecimalFormat("#0.00");
        HierarchicalClassifierTrainer qc = new HierarchicalClassifierTrainer(languagePair);
        StringBuilder sb = new StringBuilder();

        for (String alg : learningCombos) {
            properties.setProperty("learners",alg);
            for (String featureTypes : featureTypeCombos) {
                properties.setProperty("featureTypes",featureTypes);
                qc.setProperties(properties);
                Evaluation eval = qc.runExperiment();
                sb.append(alg.replaceAll(",","-")+"-"+
                        featureTypes.replaceAll(",","-")+
                        format.format((1.0-eval.errorRate())*100)+"\n");
                String report = qc.createReport();
                log.debug("Report:\n"+report);
                FileUtil.writeFile(report,"reports/report-"+
                        alg.replaceAll(",","-")+"-"+
                        featureTypes.replaceAll(",","-")+
                        ".txt","UTF-8");
            }
        }
        FileUtil.writeFile(sb.toString(),"reports/results-"+System.currentTimeMillis()+".txt","UTF-8");
    }
    

    /**
     * Calls <code>runExperiment()</code> or <code>trainAndSave()</code>, depending
     * on whether the <code>--train</code> command-line argument is specified.  Also
     * requires two properties files as input arguments: one for specifying the experiments
     * to run and one for configuring  {@link info.ephyra.questionanalysis.atype.extractor.EnglishFeatureExtractor
     *   EnglishFeatureExtractor}
     * 
     * @param args input arguments: "[--train] &lt;Experimenter-properties&gt; &lt;qa.properties&gt;"
     * @throws Exception
     */
    public static void main(String[] args) throws Exception {

        if (args.length != 1) {
            System.err.println("USAGE: Experimenter <questionLang> <corpusLang>");
            System.exit(0);
        }
        Pair<Language,Language> languagePair = new Pair<Language,Language>(
                Language.valueOf(args[0]),
                Language.valueOf(args[1]));
        Experimenter er = new Experimenter(languagePair);
        er.runExperiments();
    }
}

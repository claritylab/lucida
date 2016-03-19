package info.ephyra.questionanalysis.atype.minorthird.hierarchical;

import info.ephyra.questionanalysis.atype.extractor.FeatureExtractor;
import info.ephyra.questionanalysis.atype.extractor.FeatureExtractorFactory;
import info.ephyra.util.Properties;

import java.io.File;
import java.io.Serializable;
import java.text.DecimalFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.Formatter;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Locale;

import libsvm.svm_parameter;

import org.apache.log4j.Logger;

import edu.cmu.lti.javelin.util.FileUtil;
import edu.cmu.lti.javelin.util.Language;
import edu.cmu.lti.javelin.util.MLToolkit;
import edu.cmu.lti.util.Pair;
import edu.cmu.minorthird.classify.BasicDataset;
import edu.cmu.minorthird.classify.CascadingBinaryLearner;
import edu.cmu.minorthird.classify.Classifier;
import edu.cmu.minorthird.classify.ClassifierLearner;
import edu.cmu.minorthird.classify.Dataset;
import edu.cmu.minorthird.classify.DatasetClassifierTeacher;
import edu.cmu.minorthird.classify.Example;
import edu.cmu.minorthird.classify.Feature;
import edu.cmu.minorthird.classify.MostFrequentFirstLearner;
import edu.cmu.minorthird.classify.MutableInstance;
import edu.cmu.minorthird.classify.OneVsAllLearner;
import edu.cmu.minorthird.classify.Splitter;
import edu.cmu.minorthird.classify.algorithms.knn.KnnLearner;
import edu.cmu.minorthird.classify.algorithms.linear.BalancedWinnow;
import edu.cmu.minorthird.classify.algorithms.linear.KWayMixtureLearner;
import edu.cmu.minorthird.classify.algorithms.linear.MarginPerceptron;
import edu.cmu.minorthird.classify.algorithms.linear.MaxEntLearner;
import edu.cmu.minorthird.classify.algorithms.linear.NaiveBayes;
import edu.cmu.minorthird.classify.algorithms.linear.NegativeBinomialLearner;
import edu.cmu.minorthird.classify.algorithms.linear.VotedPerceptron;
import edu.cmu.minorthird.classify.algorithms.random.RandomElement;
import edu.cmu.minorthird.classify.algorithms.svm.SVMLearner;
import edu.cmu.minorthird.classify.algorithms.trees.AdaBoost;
import edu.cmu.minorthird.classify.algorithms.trees.DecisionTreeLearner;
import edu.cmu.minorthird.classify.experiments.CrossValSplitter;
import edu.cmu.minorthird.classify.experiments.CrossValidatedDataset;
import edu.cmu.minorthird.classify.experiments.Evaluation;
import edu.cmu.minorthird.classify.experiments.Tester;
import edu.cmu.minorthird.util.IOUtil;
import edu.cmu.minorthird.util.gui.ViewerFrame;

/**
 * Tool for training and evaluating hierarchical classifiers.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class HierarchicalClassifierTrainer{

    private static Logger log = Logger.getLogger(HierarchicalClassifierTrainer.class);
	private FeatureExtractor extractor;
	private String trainingFile;
	private String testingFile;
	private int crossValidationFolds;
	private String[] learnerNames;
	private boolean useClassLevels;
	private HashSet<String> classLabels;
	private HashSet<String> trainingLabels;
	private HashSet<String> featureTypes;
    private boolean loadTraining;
	private String classifierDir;

	//private ExampleSchema schema;
	private Dataset trainingSet;
	private Dataset testingSet;

	private Classifier classifier;
    private Pair<Language,Language> languagePair;
    private Properties properties;
	private CrossValidatedDataset cvDataset;
	private Evaluation evaluation;
	private long runTime;

	public HierarchicalClassifierTrainer(Pair<Language,Language> languagePair){
        this.languagePair = languagePair;
	}
    
    /**
     * Overrides default properties with those given.
     * 
     * @param properties
     */
    public void setProperties(Properties properties) {
        for (String property : properties.keySet()) {
            this.properties.put(property,properties.get(property));
        }
        try {
        initialize();
        } catch (Exception e) {
            log.error("Error re-initializing: ",e);
        }
    }
    
    public void initialize() throws Exception {
        if (languagePair == null) 
            throw new Exception("Langauage pair must be set before calling initialize");
        
        if (properties == null) {
            properties = Properties.loadFromClassName(this.getClass().getName());
            properties = properties.mapProperties().get(languagePair.getFirst()+"_"+languagePair.getSecond());
            extractor=FeatureExtractorFactory.getInstance(languagePair.getFirst());
        }        
        
        trainingFile=properties.getProperty("trainingFile");
        testingFile=properties.getProperty("testingFile");
        crossValidationFolds=Integer.parseInt(properties.getProperty("crossValidationFolds"));
        learnerNames=properties.getProperty("learners").split(",");
        for(int i=0;i<learnerNames.length;i++){
            learnerNames[i]=learnerNames[i].trim();
        }
        useClassLevels=Boolean.parseBoolean(properties.getProperty("useClassLevels"));
        if (!useClassLevels && learnerNames.length > 1) { 
            String[] newArr = new String[1];
            learnerNames = Arrays.asList(learnerNames).subList(0,1).toArray(newArr);
        }
        classLabels=new HashSet<String>();
        String[] labels=properties.getProperty("classLabels").split(",");
        for(int i=0;i<labels.length;i++){
            labels[i]=HierarchicalClassifier.getHierarchicalClassName(labels[i],learnerNames.length,useClassLevels);
            classLabels.add(labels[i]);
        }
        //schema=new ExampleSchema(labels);
        featureTypes=new HashSet<String>();
        String[] types=properties.getProperty("featureTypes").split(",");
        for(int i=0;i<types.length;i++){
            featureTypes.add(types[i].trim());
        }
        classifierDir=properties.getProperty("classifierDir");
        trainingSet=makeDataset(trainingFile);
        if(crossValidationFolds<0){
            testingSet=makeDataset(testingFile);
        }
    }

	private Dataset makeDataset(String fileName){
        if (trainingLabels == null) {
            loadTraining = true;
            trainingLabels = new HashSet<String>();
        }
		Dataset set=new BasicDataset();
		extractor.setUseClassLevels(useClassLevels);
		extractor.setClassLevels(learnerNames.length);
		Example[] examples=extractor.loadFile(fileName);
		for(int i=0;i<examples.length;i++){
            String label = examples[i].getLabel().bestClassName();
			if(classLabels.contains(label)){
				MutableInstance instance=new MutableInstance(examples[i].getSource(),examples[i].getSubpopulationId());
				Feature.Looper bLooper=examples[i].binaryFeatureIterator();
				while(bLooper.hasNext()){
					Feature f=bLooper.nextFeature();
					if(featureTypes.contains(f.getPart(0))){
						instance.addBinary(f);
					}
				}
				Feature.Looper nLooper=examples[i].numericFeatureIterator();
				while(nLooper.hasNext()){
					Feature f=nLooper.nextFeature();
					if(featureTypes.contains(f.getPart(0))){
						instance.addNumeric(f,examples[i].getWeight(f));
					}
				}
				Example example=new Example(instance,examples[i].getLabel());
				MLToolkit.println(example);
                if (loadTraining) {
                    trainingLabels.add(label);
                    set.add(example);
                }
                else {
                    if (!trainingLabels.contains(label))
                        MLToolkit.println("Label of test example not found in training set (discarding): "+label);
                    else set.add(example);
                }
			}
			else{
				MLToolkit.println("Discarding example for Class: "+label);
			}
		}
        if (loadTraining) loadTraining = false;
		MLToolkit.println("Loaded "+set.size()+" examples for experiment from "+fileName);
		return set;
	}

	public HierarchicalClassifierLearner createHierarchicalClassifierLearner(String[] learners){
		ClassifierLearner[] prototypes=new ClassifierLearner[learners.length];
		for(int i=0;i<prototypes.length;i++){
			prototypes[i]=createLearnerByName(learners[i]);
		}
		return new HierarchicalClassifierLearner(prototypes);
	}

	public ClassifierLearner createLearnerByName(String name){
		ClassifierLearner learner;
		//K-Nearest-Neighbor learner, using m3rd recommended parameters
		if(name.equalsIgnoreCase("KNN")){
			learner=new KnnLearner();
		}
		//K-Way Mixture learner, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("KWAY_MIX")){
			learner=new KWayMixtureLearner();
		}
		//Maximum Entropy learner, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("MAX_ENT")){
			learner=new MaxEntLearner();
		}
		//Balanced Winnow learner with One vs All binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("BWINNOW_OVA")){
			learner=new OneVsAllLearner(new BalancedWinnow());
		}
		//Margin Perceptron learner with One vs All binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("MPERCEPTRON_OVA")){
			learner=new OneVsAllLearner(new MarginPerceptron());
		}
		//Naive Bayes learner with One vs All binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("NBAYES_OVA")){
			learner=new OneVsAllLearner(new NaiveBayes());
		}
		//Voted Perceptron learner with One vs All binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("VPERCEPTRON_OVA")){
			learner=new OneVsAllLearner(new VotedPerceptron());
		}
		//Ada Boost learner with One vs All binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("ADABOOST_OVA")){
			learner=new OneVsAllLearner(new AdaBoost());
		}
		//Ada Boost learner with Cascading binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("ADABOOST_CB")){
			learner=new CascadingBinaryLearner(new AdaBoost());
		}
		//Ada Boost learner with Most Frequent First binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("ADABOOST_MFF")){
			learner=new MostFrequentFirstLearner(new AdaBoost());
		}
		//Ada Boost learner (Logistic Regression version) with One vs All binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("ADABOOSTL_OVA")){
			learner=new OneVsAllLearner(new AdaBoost.L());
		}
		//Ada Boost learner (Logistic Regression version) with Cascading binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("ADABOOSTL_CB")){
			learner=new CascadingBinaryLearner(new AdaBoost.L());
		}
		//Ada Boost learner (Logistic Regression version) with Most Frequent First binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("ADABOOSTL_MFF")){
			learner=new MostFrequentFirstLearner(new AdaBoost.L());
		}
		//Decision Tree learner with One vs All binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("DTREE_OVA")){
			learner=new OneVsAllLearner(new DecisionTreeLearner());
		}
		//Decision Tree learner with Cascading binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("DTREE_CB")){
			learner=new CascadingBinaryLearner(new DecisionTreeLearner());
		}
		//Decision Tree learner with Most Frequent First binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("DTREE_MFF")){
			learner=new MostFrequentFirstLearner(new DecisionTreeLearner());
		}
		//Negative Binomial learner with One vs All binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("NEGBI_OVA")){
			learner=new OneVsAllLearner(new NegativeBinomialLearner());
		}
		//Negative Binomial learner with Cascading binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("NEGBI_CB")){
			learner=new CascadingBinaryLearner(new NegativeBinomialLearner());
		}
		//Negative Binomial learner with Most Frequent First binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("NEGBI_MFF")){
			learner=new MostFrequentFirstLearner(new NegativeBinomialLearner());
		}
		//SVM learner with One vs All binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("SVM_OVA")){
			learner=new OneVsAllLearner(new SVMLearner());
		}
		//SVM learner with One vs All binary transformer, using testing parameters
		else if(name.equalsIgnoreCase("SVM_OVA_CONF1")){
			svm_parameter param=new svm_parameter();
			param.svm_type=svm_parameter.C_SVC;
			param.kernel_type=svm_parameter.POLY;
			param.degree=2;
			param.gamma=1; // 1/k
			param.coef0=0;
			param.nu=0.5;
			param.cache_size=40;
			param.C=1;
			param.eps=1e-3;
			param.p=0.1;
			param.shrinking=1;
			param.nr_weight=0;
			param.weight_label=new int[0];
			param.weight=new double[0];
			learner=new OneVsAllLearner(new SVMLearner(param));
		}
		//SVM learner with Cascading binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("SVM_CB")){
			learner=new CascadingBinaryLearner(new SVMLearner());
		}
		//SVM learner with Most Frequent First binary transformer, using m3rd recommended parameters
		else if(name.equalsIgnoreCase("SVM_MFF")){
			learner=new MostFrequentFirstLearner(new SVMLearner());
		}
		else{
			System.err.println("Unrecognized learner name: "+name);
			learner=null;
		}
		return learner;
	}

	public Evaluation runExperiment(){

		runTime=System.currentTimeMillis();

		ClassifierLearner learner=createHierarchicalClassifierLearner(learnerNames);

		if(crossValidationFolds<0){
			evaluation=Tester.evaluate(learner,trainingSet,testingSet);
		}
		else{
			Splitter splitter=new CrossValSplitter(new RandomElement(System.currentTimeMillis()),crossValidationFolds);
			cvDataset=new CrossValidatedDataset(learner,trainingSet,splitter,true);
			evaluation=cvDataset.getEvaluation();
			//remove later
			//ViewerFrame frame=new ViewerFrame(trainingFile,cvDataset.toGUI());
			//frame.setVisible(true);
			//evaluation=Tester.evaluate(learner,trainingSet,splitter);
		}

		runTime=System.currentTimeMillis()-runTime;

		return evaluation;
	}

	public void trainClassifier(){
		runTime=System.currentTimeMillis();
		ClassifierLearner learner=createHierarchicalClassifierLearner(learnerNames);
		classifier=new DatasetClassifierTeacher(trainingSet).train(learner);
		runTime=System.currentTimeMillis()-runTime;
	}
	
	public void saveClassifier(String fileName){
		try{
			IOUtil.saveSerialized((Serializable)classifier,new File(fileName));
		}
		catch(Exception e){
			e.printStackTrace(System.err);
		}
	}
	
	public void saveClassifier(){
		String fileName=classifierDir+System.currentTimeMillis()/1000;
		for(int i=0;i<learnerNames.length;i++){
			fileName+="-"+learnerNames[i];
		}
		if(useClassLevels){
			fileName+="-HC";
		}
		fileName+="-"+(new File(trainingFile)).getName();
		saveClassifier(fileName);
	}
	
	public void loadClassifier(String fileName){
		try{
			classifier=(Classifier)IOUtil.loadSerialized(new File(fileName));
		}
		catch(Exception e){
			e.printStackTrace(System.err);
		}
	}
	
	public Classifier getClassifier(){
		return classifier;
	}

	public String createReport(){
		DecimalFormat format=new DecimalFormat("#0.00");
		StringBuffer b=new StringBuffer();
		b.append("Question Answer Type Classification Report\n");
		b.append((new Date())+"\n");
		b.append("\n");
		b.append("Training Data File: "+trainingFile+"\n");
		if(crossValidationFolds<0){
			b.append("Testing Data File: "+testingFile+"\n");
		}
		else{
			b.append("Testing using "+crossValidationFolds+"-fold cross validation"+"\n");
		}
		b.append("\n");
		b.append("Valid Class Labels:");
		for(Iterator it=classLabels.iterator();it.hasNext();b.append(" "+(String)it.next()));
		b.append("\n");
		b.append("\n");
		if(useClassLevels){
			b.append("Using Hierarchical Classifier Learners:\n");
			for(int i=0;i<learnerNames.length;i++){
				b.append("\t"+learnerNames[i]+"\n");
			}
		}
		else{
			b.append("Using Simple Classifier Learner: "+learnerNames[0]+"\n");
		}
		b.append("\n");
		b.append("Feature Selection:\n");
		for(Iterator it=featureTypes.iterator();it.hasNext();b.append("\t"+(String)it.next()+"\n"));
		b.append("\n");
		b.append("Experiment Results:\n");
		b.append("\n");
		b.append("\tAccuracy: "+format.format((1.0-evaluation.errorRate())*100)+"% ["+evaluation.numExamples()+" example(s)]\n");
		b.append("\n");
		b.append("\tAccuracy by Class:\n");
		b.append("\n");
		String[] classNames=evaluation.getClasses();
		double[] numExamples=evaluation.numberOfExamplesByClass();
		double[] errorRates=evaluation.errorRateByClass();
		double total=0;
		for(int i=0;i<classNames.length;i++){
			double accuracy=(1.0-errorRates[i])*100;
			b.append("\t\t"+classNames[i]+" "+format.format(accuracy)+"% ["+(int)numExamples[i]+" example(s)]\n");
			total+=accuracy;
		}
		b.append("\n");
		b.append("\tAverage Class Accuracy: "+format.format(total/classNames.length)+"%\n");
		b.append("\n");
		b.append("Run Time: "+runTime+" ms\n");
		b.append("\n");
        b.append("Confusion Matrix:\n");
        b.append("\n");
        b.append(prettyPrintCM(evaluation.confusionMatrix(),evaluation.getClasses()));
        b.append("\n");
		return b.toString();
	}

    private String prettyPrintCM (Evaluation.Matrix matrix, String[] classes ) {
        double[][] values = matrix.values;
        String[] classAbb = new String[classes.length];
        StringBuilder res = new StringBuilder();
        Formatter formatter = new Formatter(res,Locale.US);
        int max = 0;
        for (int i = 0; i < classes.length; i++) {
            classAbb[i] = classes[i].replaceAll("\\B(.{1,2}).*?(.)\\b","$1$2");
            if (classAbb[i].length() > max) max = classAbb[i].length();
        }
        max++;
        String formatStr = "%-"+max+"s";
        formatter.format(formatStr,"");
        for (int i = 0; i < classes.length; i++) {
            formatter.format(formatStr,classAbb[i]);
        }
        res.append("\n\n");
        for (int i = 0; i < classes.length; i++ ) {
            formatter.format(formatStr,classAbb[i]);
            for (int j = 0; j < classes.length; j++) {
                formatter.format(formatStr,Double.toString(values[i][j]));
            }
            res.append("\n\n");
        }
        return res.toString();
    }
    
    public static void main(String[] args)throws Exception{

        if (args.length > 3 || args.length < 2 || (args.length == 3 && !args[0].equals("--train"))) {
            System.err.println("Usage:");
            System.err.println("java HierarchicalClassifierTrainer [--train] <questionLang> <corpusLang>\n");
            System.err.println(" - <questionLang> and <corpusLang> must be one of the following:");
            System.err.println("     en_US, ja_JP, jp_JP, zh_TW, zh_CN");
            System.err.println(" - Outputs a trained model in the current directory if --train is used.");
            System.err.println(" - Otherwise, performs an evaluation using the configuration in the");
            System.err.println("     properties file and outputs a report describing the results.");
            System.exit(0);
        }
        
        boolean train = false;
        int langPairInd = 0;
        if (args[0].equals("--train")) {
            train = true;
            langPairInd++;
        }
        
        Pair<Language,Language> languagePair = new Pair<Language,Language>(
                Language.valueOf(args[langPairInd]),
                Language.valueOf(args[langPairInd+1]));
        
        HierarchicalClassifierTrainer qct=new HierarchicalClassifierTrainer(languagePair);
        qct.initialize();
        
        if (train) {
            System.out.println("Training classifier...");
            qct.trainClassifier();
            qct.saveClassifier();
            System.out.println("Classifier saved.");
        } else {
            System.out.println("Running experiment...");
            Evaluation eval=qct.runExperiment();
            FileUtil.writeFile(qct.createReport(),args[0]+".report"+System.currentTimeMillis()+".txt","UTF-8");

            ViewerFrame frame=new ViewerFrame(args[0],eval.toGUI());
            frame.setVisible(true);
        }

    }
    
}

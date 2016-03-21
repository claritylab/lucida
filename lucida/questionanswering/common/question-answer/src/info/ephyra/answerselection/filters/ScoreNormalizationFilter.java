package info.ephyra.answerselection.filters;

import info.ephyra.io.MsgPrinter;
import info.ephyra.questionanalysis.AnalyzedQuestion;
import info.ephyra.search.Result;
import info.ephyra.util.ArrayUtils;
import info.ephyra.util.FileUtils;
import info.ephyra.util.StringUtils;

import java.io.EOFException;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.List;
import java.util.Set;

import edu.cmu.minorthird.classify.BasicDataset;
import edu.cmu.minorthird.classify.ClassLabel;
import edu.cmu.minorthird.classify.Classifier;
import edu.cmu.minorthird.classify.ClassifierLearner;
import edu.cmu.minorthird.classify.Dataset;
import edu.cmu.minorthird.classify.DatasetClassifierTeacher;
import edu.cmu.minorthird.classify.Example;
import edu.cmu.minorthird.classify.ExampleSchema;
import edu.cmu.minorthird.classify.Feature;
import edu.cmu.minorthird.classify.Instance;
import edu.cmu.minorthird.classify.MutableInstance;
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
import edu.cmu.minorthird.ui.Recommended.BoostedStumpLearner;

/**
 * <p>A filter that normalizes the scores of the answer candidates by applying a
 * trained classifier. The weight of the positive class ("answer correct") is
 * used as the normalized score.</p>
 * 
 * <p>The main method can be used to evaluate different combinations of features
 * and models and to train a classifier with the best combination.</p>
 * 
 * <p>The filter is applied to factoid answers only.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Nico Schlaefer
 * @version 2008-01-26
 */
public class ScoreNormalizationFilter extends Filter {
	/** Identifier for the score feature. */
	private static final String SCORE_F = "Score";
	/** Identifier for the extractor features. */
	private static final String EXTRACTORS_F = "Extractors";
	/** Identifier for the answer type features. */
	private static final String ANSWER_TYPES_F = "AnswerTypes";
	/** Identifier for the number of answers feature. */
	private static final String NUM_ANSWERS_F = "NumAnswers";
	/** Identifier for the mean score feature. */
	private static final String MEAN_SCORE_F = "MeanScore";
	/** Identifier for the maximum score feature. */
	private static final String MAX_SCORE_F = "MaxScore";
	/** Identifier for the minimum score feature. */
	private static final String MIN_SCORE_F = "MinScore";
	/** All feature identifiers. */
	private static final String[] ALL_FEATURES = {
		SCORE_F,
		EXTRACTORS_F,
		ANSWER_TYPES_F,
		NUM_ANSWERS_F,
		MEAN_SCORE_F,
		MAX_SCORE_F,
		MIN_SCORE_F
	};
	/** Subset of the features used to train the classifier. */
	private static final String[] SELECTED_FEATURES = {
		SCORE_F,
		EXTRACTORS_F,
//		ANSWER_TYPES_F,
//		NUM_ANSWERS_F,
//		MEAN_SCORE_F,
//		MAX_SCORE_F,
//		MIN_SCORE_F
	};
	
	/** Identifier for the Ada Boost model (boosts a decision tree learner 10 times). */
	private static final String ADA_BOOST_10_M = "AdaBoost10";
	/** Identifier for the Ada Boost model (boosts a decision tree learner 100 times). */
	private static final String ADA_BOOST_100_M = "AdaBoost100";
	/** The <code>N</code> in <code>ADA_BOOST_N_M</code>. */
	private static int NUM_BOOSTS = 70;
	/** Identifier for the Ada Boost model (boosts a decision tree learner <code>NUM_BOOSTS</code> times). */
	private static String ADA_BOOST_N_M = "AdaBoost" + NUM_BOOSTS;
	/** Identifier for the Ada Boost model (Logistic Regression version). */
	private static final String ADA_BOOST_L_M = "AdaBoostL";
	/** Identifier for the Balanced Winnow model. */
	private static final String BALANCED_WINNOW_M = "BalancedWinnow";
	/** Identifier for the Decision Tree model. */
	private static final String DECISION_TREE_M = "DecisionTree";
	/** Identifier for the K-Nearest-Neighbor model. */
	private static final String KNN_M = "KNN";
	/** Identifier for the K-Way Mixture model. */
	private static final String KWAY_MIXTURE_M = "KWayMixture";
	/** Identifier for the Margin Perceptron model. */
	private static final String MARGIN_PERCEPTRON_M = "MarginPerceptron";
	/** Identifier for the Maximum Entropy model. */
	private static final String MAX_ENT_M = "MaxEnt";
	/** Identifier for the Naive Bayes model. */
	private static final String NAIVE_BAYES_M = "NaiveBayes";
	/** Identifier for the Negative Binomial model. */
	private static final String NEGATIVE_BINOMIAL_M = "NegativeBinomial";
	/** Identifier for the SVM model. */
	private static final String SVM_M = "SVM";
	/** Identifier for the Voted Perceptron model. */
	private static final String VOTED_PERCEPTRON_M = "VotedPerceptron";
	/** All model identifiers. */
	private static final String[] ALL_MODELS = {
		ADA_BOOST_10_M,
		ADA_BOOST_100_M,
		ADA_BOOST_N_M,
		ADA_BOOST_L_M,
		BALANCED_WINNOW_M,
		DECISION_TREE_M,
//		KNN_M,  // time-intensive training
//		KWAY_MIXTURE_M,  // only for non-binary data
		MARGIN_PERCEPTRON_M,
		MAX_ENT_M,
		NAIVE_BAYES_M,
		NEGATIVE_BINOMIAL_M,
//		SVM_M,  // time-intensive training
		VOTED_PERCEPTRON_M
	};
	/** Model used for the classifier. */
	private static final String SELECTED_MODEL = ADA_BOOST_N_M;
	
	/** Number of folds for cross validation. */
	private static final int NUM_FOLDS = 3;
	
	/** Classifier for score normalization. */
	private static Classifier classifier;
	
	/**
	 * Reads serialized results from a file.
	 * 
	 * @param input input file
	 * @return result objects
	 */
	private static Result[] readSerializedResults(File input) {
		ArrayList<Result> results = new ArrayList<Result>();
		
		try {
			FileInputStream fis = new FileInputStream(input);
			ObjectInputStream ois = new ObjectInputStream(fis);
			
			// make sure that first serialized object is an AnalyzedQuestion,
			// then discard it
			if (!(ois.readObject() instanceof AnalyzedQuestion)) {
				MsgPrinter.printErrorMsg("First serialized object is not an" +
						"AnalyzedQuestion.");
				System.exit(1);
			}
			try {
				while (true) results.add((Result) ois.readObject());
			} catch (EOFException e) {/* end of file reached */}
			
			ois.close();
		} catch (Exception e) {
			MsgPrinter.printErrorMsg("Could not read serialized results:");
			MsgPrinter.printErrorMsg(e.toString());
			System.exit(1);
		}
		
		return results.toArray(new Result[results.size()]);
	}
	
	/**
	 * Adds the score of the answer candidate as a feature to the instance.
	 */
	private static void addScoreFeature(MutableInstance instance,
			Result result) {
		float score = result.getScore();
		
		Feature feature = new Feature(SCORE_F);
		instance.addNumeric(feature, score);
	}
	
	/**
	 * Adds the extractor used to obtain the answer candidate as a feature to
	 * the instance.
	 */
	private static void addExtractorFeature(MutableInstance instance,
			Result result) {
		String extractor = result.getExtractionTechniques()[0];
		
		Feature feature = new Feature(extractor);
		instance.addBinary(feature);
	}
	
	/**
	 * Adds the answer types of the question as features to the instance.
	 */
	private static void addAnswerTypeFeatures(MutableInstance instance,
			Result result) {
		String[] answerTypes =
			result.getQuery().getAnalyzedQuestion().getAnswerTypes();
		
		for (String answerType : answerTypes) {
			Feature feature = new Feature(answerType.split("->"));
			instance.addBinary(feature);
		}
	}
	
	/**
	 * Adds the number of factoid answers from the same extractor as a feature
	 * to the instance.
	 */
	private static void addNumAnswersFeature(MutableInstance instance,
			Result result, Result[] results) {
		// get number of factoid answers
		int numFactoid = 0;
//		String extractor = result.getExtractionTechniques()[0];
		for (Result r : results)
			if (r.getScore() > 0 && r.getScore() < Float.POSITIVE_INFINITY)
//				if (r.extractedWith(extractor))
					numFactoid++;
		
		Feature feature = new Feature(NUM_ANSWERS_F);
		instance.addNumeric(feature, numFactoid);
	}
	
	/**
	 * Adds the mean score of all factoid answers from the same extractor as a
	 * feature to the instance.
	 */
	private static void addMeanScoreFeature(MutableInstance instance,
			Result result, Result[] results) {
		// calculate mean score
		double meanScore = 0;
		int numFactoid = 0;
//		String extractor = result.getExtractionTechniques()[0];
		for (Result r : results)
			if (r.getScore() > 0 && r.getScore() < Float.POSITIVE_INFINITY) {
//				if (r.extractedWith(extractor)) {
					meanScore += r.getScore();
					numFactoid++;
//				}
			}
		meanScore /= numFactoid;
		
		Feature feature = new Feature(MEAN_SCORE_F);
		instance.addNumeric(feature, meanScore);
	}
	
	/**
	 * Adds the maximum score of all factoid answers from the same extractor as
	 * a feature to the instance.
	 */
	private static void addMaxScoreFeature(MutableInstance instance,
			Result result, Result[] results) {
		// calculate maximum score
		double maxScore = 0;
//		String extractor = result.getExtractionTechniques()[0];
		for (Result r : results)
			if (r.getScore() > 0 && r.getScore() < Float.POSITIVE_INFINITY)
//				if (r.extractedWith(extractor))
					maxScore = Math.max(r.getScore(), maxScore);
		
		Feature feature = new Feature(MAX_SCORE_F);
		instance.addNumeric(feature, maxScore);
	}
	
	/**
	 * Adds the minimum score of all factoid answers from the same extractor as
	 * a feature to the instance.
	 */
	private static void addMinScoreFeature(MutableInstance instance,
			Result result, Result[] results) {
		// calculate minimum score
		double minScore = Double.POSITIVE_INFINITY;
//		String extractor = result.getExtractionTechniques()[0];
		for (Result r : results)
			if (r.getScore() > 0 && r.getScore() < Float.POSITIVE_INFINITY)
//				if (r.extractedWith(extractor))
					minScore = Math.min(r.getScore(), minScore);
		
		Feature feature = new Feature(MIN_SCORE_F);
		instance.addNumeric(feature, minScore);
	}
	
	/**
	 * Adds the selected features to the instance.
	 */
	private static void addSelectedFeatures(MutableInstance instance,
			String[] features, Result result, Result[] results) {
		Set<String> featureSet = new HashSet<String>();
		for (String feature : features) featureSet.add(feature);
		
		if (featureSet.contains(SCORE_F))
			addScoreFeature(instance, result);
		if (featureSet.contains(EXTRACTORS_F))
			addExtractorFeature(instance, result);
		if (featureSet.contains(ANSWER_TYPES_F))
			addAnswerTypeFeatures(instance, result);
		if (featureSet.contains(NUM_ANSWERS_F))
			addNumAnswersFeature(instance, result, results);
		if (featureSet.contains(MEAN_SCORE_F))
			addMeanScoreFeature(instance, result, results);
		if (featureSet.contains(MAX_SCORE_F))
			addMaxScoreFeature(instance, result, results);
		if (featureSet.contains(MIN_SCORE_F))
			addMinScoreFeature(instance, result, results);
	}
	
	/**
	 * Creates an instance for training/evaluation or classification from an
	 * answer candidate.
	 * 
	 * @param features selected features
	 * @param result answer candidate
	 * @param results all answers to the question
	 * @return instance for training/evaluation or classification
	 */
	private static Instance createInstance(String[] features, Result result,
			Result[] results) {
		// create instance from source object
		MutableInstance instance = new MutableInstance(result);
		
		// add selected features to the instance
		addSelectedFeatures(instance, features, result, results);
		return instance;
	}
	
	/**
	 * Creates an instance for training/evaluation or classification from an
	 * answer candidate, using the question ID as a subpopulation ID.
	 * 
	 * @param features selected features
	 * @param result answer candidate
	 * @param results all answers to the question
	 * @param qid question ID
	 * @return instance for training/evaluation or classification
	 */
	private static Instance createInstance(String[] features, Result result,
			Result[] results, String qid) {
		// create instance from source object and subpopulation ID
		MutableInstance instance = new MutableInstance(result, qid);
		
		// add selected features to the instance
		addSelectedFeatures(instance, features, result, results);
		return instance;
	}
	
	/**
	 * Creates a training/evaluation example from a judged answer candidate.
	 * 
	 * @param features selected features
	 * @param result judged answer candidate
	 * @param results all answers to the question
	 * @param qid question ID
	 * @return training/evaluation example
	 */
	private static Example createExample(String[] features, Result result,
			Result[] results, String qid) {
		// create instance with selected features
		Instance instance = createInstance(features, result, results, qid);
		
		// create example from the instance and its class label
		String label = result.isCorrect()
				? ExampleSchema.POS_CLASS_NAME
				: ExampleSchema.NEG_CLASS_NAME;
		Example example = new Example(instance, new ClassLabel(label));
		
		return example;
	}
	
	/**
	 * Creates a training/evaluation set from serialized judged
	 * <code>Result</code> objects.
	 * 
	 * @param features selected features
	 * @param serializedDir directory containing serialized results
	 * @return training/evaluation set
	 */
	private static Dataset createDataset(String[] features,
			String serializedDir) {
		Dataset set = new BasicDataset();
		
		File[] files = FileUtils.getFilesRec(serializedDir);
		for (File file : files) {  // one file per question
			String filename = file.getName();
			if (!filename.endsWith(".serialized")) continue;
			
			// get question ID and results
			String qid = filename.replace(".serialized", "");
			Result[] results = readSerializedResults(file);
			
			// create examples and add to data set
			for (Result result : results) {
				// only factoid answers with 1 extraction technique
				if (result.getScore() <= 0 ||
						result.getScore() == Float.POSITIVE_INFINITY ||
						result.getExtractionTechniques() == null ||
						result.getExtractionTechniques().length != 1)
					continue;
				
				Example example = createExample(features, result, results, qid);
				set.add(example);
			}
		}
		
		return set;
	}
	
	/**
	 * Creates a classifier learner for the given model.
	 * 
	 * @param model selected model
	 * @return classifier learner
	 */
	private static ClassifierLearner createLearner(String model) {
		ClassifierLearner learner = null;
		
		// Ada Boost learner, (boosts a decision tree learner 10 times), using m3rd recommended parameters
		if (model.equals(ADA_BOOST_10_M)) {
			learner = new AdaBoost();
		}
		// Ada Boost learner, (boosts a decision tree learner 100 times), using m3rd recommended parameters
		else if (model.equals(ADA_BOOST_100_M)) {
			learner = new BoostedStumpLearner();
		}
		// Ada Boost learner, (boosts a decision tree learner NUM_BOOSTS times), using m3rd recommended parameters
		else if (model.equals(ADA_BOOST_N_M)) {
			learner = new AdaBoost(new DecisionTreeLearner(), NUM_BOOSTS);
		}
		// Ada Boost learner (Logistic Regression version), using m3rd recommended parameters
		else if (model.equals(ADA_BOOST_L_M)) {
			learner = new AdaBoost.L();
		}
		// Balanced Winnow learner, using m3rd recommended parameters
		else if (model.equals(BALANCED_WINNOW_M)) {
			learner = new BalancedWinnow();
		}
		// Decision Tree learner, using m3rd recommended parameters
		else if (model.equals(DECISION_TREE_M)) {
			learner = new DecisionTreeLearner();
		}
		// K-Nearest-Neighbor learner, using m3rd recommended parameters
		else if (model.equals(KNN_M)) {
			learner = new KnnLearner();
		}
		// K-Way Mixture learner, using m3rd recommended parameters
		else if (model.equals(KWAY_MIXTURE_M)) {
			learner = new KWayMixtureLearner();
		}
		// Margin Perceptron learner, using m3rd recommended parameters
		else if (model.equals(MARGIN_PERCEPTRON_M)) {
			learner = new MarginPerceptron();
		}
		// Maximum Entropy learner, using m3rd recommended parameters
		else if (model.equals(MAX_ENT_M)) {
			learner = new MaxEntLearner();
		}
		// Naive Bayes learner, using m3rd recommended parameters
		else if (model.equals(NAIVE_BAYES_M)) {
			learner = new NaiveBayes();
		}
		// Negative Binomial learner, using m3rd recommended parameters
		else if (model.equals(NEGATIVE_BINOMIAL_M)) {
			learner = new NegativeBinomialLearner();
		}
		// SVM learner, using m3rd recommended parameters
		else if (model.equals(SVM_M)) {
			learner = new SVMLearner();
		}
		// Voted Perceptron learner, using m3rd recommended parameters
		else if (model.equals(VOTED_PERCEPTRON_M)) {
			learner = new VotedPerceptron();
		}
		// Unknown model
		else {
			MsgPrinter.printErrorMsg("Unknown model: " + model);
			System.exit(1);
		}
		
		return learner;
	}
	
	/**
	 * Builds a report comprising the selected parameters (data sets, features
	 * and model) and evaluation statistics.
	 * 
	 * @param dataSets used data sets
	 * @param features selected features
	 * @param model selected model
	 * @param eval evaluation statistics
	 * @param runTime run time of the evaluation
	 * @return report
	 */
	private static String createReport(String[] dataSets, String[] features,
			String model, Evaluation eval, long runTime) {
		String report = "";
		
		// parameters section
		report += "Parameters:\n";
		report += "-----------\n";
		report += "Data set: " + StringUtils.concat(dataSets, ", ") +
				  " (" + eval.numExamples() + " examples)\n";
		report += "Features: " + StringUtils.concat(features, ", ") + "\n";
		report += "Model:    " + model + "\n";
		report += "\n";
		
		// statistics section
		report += "Statistics:\n";
		report += "-----------\n";
		double[] stats = eval.summaryStatistics();
		String[] statNames = eval.summaryStatisticNames();
		int maxLength = 0;
		for (int i = 0; i < statNames.length; i++)
			maxLength = Math.max(statNames[i].length(), maxLength);
		for (int i = 0; i < statNames.length; i++) {
			report += statNames[i] + ": ";
			report += StringUtils.repeat(" ", maxLength - statNames[i].length());
			report += stats[i] + "\n";
		}
		report += "Runtime: ";
		report += StringUtils.repeat(" ", maxLength - 7);
		report += runTime + " ms\n";
		
		return report;
	}
	
	/**
	 * Trains a classifier using the given training data, the features specified
	 * in <code>SELECTED_FEATURES</code> and the model specified in
	 * <code>SELECTED_MODEL</code>.
	 * 
	 * @param serializedDir directory containing serialized results
	 * @return trained classifier
	 */
	public static Classifier train(String serializedDir) {
		return train(serializedDir, SELECTED_FEATURES, SELECTED_MODEL);
	}
	
	/**
	 * Trains a classifier using the given training data, features and model.
	 * 
	 * @param serializedDir directory containing serialized results
	 * @param features selected features
	 * @param model selected model
	 * @return trained classifier
	 */
	public static Classifier train(String serializedDir, String[] features,
			String model) {
		// create training set with given features from serialized results
		Dataset trainingSet = createDataset(features, serializedDir);
		
		// create learner for given model
		ClassifierLearner learner = createLearner(model);
		
		// train classifier
		Classifier classifier = new DatasetClassifierTeacher(trainingSet).train(learner);
		
		return classifier;
	}
	
	/**
	 * Performs a cross-validation on the given data set for the given features
	 * and model.
	 * 
	 * @param serializedDir directory containing serialized results
	 * @param features selected features
	 * @param model selected model
	 * @return evaluation statistics
	 */
	public static Evaluation evaluate(String serializedDir, String[] features,
			String model) {
		// create data set with selected features from serialized results
		Dataset dataSet = createDataset(features, serializedDir);
		
		// create learner for selected model
		ClassifierLearner learner = createLearner(model);
		
		// cross-validate model on data set
		RandomElement r = new RandomElement(System.currentTimeMillis());
		Splitter splitter = new CrossValSplitter(r, NUM_FOLDS);
		CrossValidatedDataset cvDataset = new CrossValidatedDataset(learner,
				dataSet, splitter, true);
		Evaluation eval = cvDataset.getEvaluation();
		
		return eval;
	}
	
	/**
	 * Performs a cross-validation on the given data set for all combinations of
	 * features and models and writes a report for each evaluation. Determines
	 * the best combination according to the F1 measure.
	 * 
	 * @param serializedDir directory containing serialized results
	 * @param reportDir output directory for evaluation reports
	 * @return best combination of features and model
	 */
	public static String[][] evaluateAll(String serializedDir,
			String reportDir) {
		// get all subsets of features
		Object[][] subsets = ArrayUtils.getNonemptySubsets(ALL_FEATURES);
		String[][] featureSets = new String[subsets.length][];
		for (int i = 0; i < subsets.length; i++) {
			featureSets[i] = new String[subsets[i].length];
			for (int j = 0; j < subsets[i].length; j++)
				featureSets[i][j] = (String) subsets[i][j];
		}
//		// evaluate selected subsets of features only
//		String[][] featureSets = {
//			// all features
//			{SCORE_F, EXTRACTORS_F, ANSWER_TYPES_F, NUM_ANSWERS_F, MEAN_SCORE_F, MAX_SCORE_F, MIN_SCORE_F},
//			// ignore extractors
//			{SCORE_F, ANSWER_TYPES_F, NUM_ANSWERS_F, MEAN_SCORE_F, MAX_SCORE_F, MIN_SCORE_F},
//			// ignore answer type
//			{SCORE_F, EXTRACTORS_F, NUM_ANSWERS_F, MEAN_SCORE_F, MAX_SCORE_F, MIN_SCORE_F},
//			// ignore number of answers
//			{SCORE_F, EXTRACTORS_F, ANSWER_TYPES_F, MEAN_SCORE_F, MAX_SCORE_F, MIN_SCORE_F},
//			// ignore mean score
//			{SCORE_F, EXTRACTORS_F, ANSWER_TYPES_F, NUM_ANSWERS_F, MAX_SCORE_F, MIN_SCORE_F},
//			// ignore maximum score
//			{SCORE_F, EXTRACTORS_F, ANSWER_TYPES_F, NUM_ANSWERS_F, MEAN_SCORE_F, MIN_SCORE_F},
//			// ignore minimum score
//			{SCORE_F, EXTRACTORS_F, ANSWER_TYPES_F, NUM_ANSWERS_F, MEAN_SCORE_F, MAX_SCORE_F},
//			// ignore minimum score and maximum score
//			{SCORE_F, EXTRACTORS_F, ANSWER_TYPES_F, NUM_ANSWERS_F, MEAN_SCORE_F},
//			// only score, extractors and answer types
//			{SCORE_F, EXTRACTORS_F, ANSWER_TYPES_F},
//			// only score and extractors
//			{SCORE_F, EXTRACTORS_F},
//			// only score
//			{SCORE_F}
//		};
//		// evaluate Ada Boost with different numbers of rounds
//		int[] allNumBoosts = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200};
		
		// evaluate all combinations of features and models,
		// get best combination according to F1 measure
		double maxF1 = -1;
		String[][] bestCombination = new String[2][];
		for (String[] features : featureSets)
//			// evaluate Ada Boost with different numbers of rounds
//			for (int numBoosts : allNumBoosts) {
//				NUM_BOOSTS = numBoosts;
//				ADA_BOOST_N_M = "AdaBoost" + numBoosts;
//				String model = ADA_BOOST_N_M;
			for (String model : ALL_MODELS) {
				// do not overwrite an existing report
				String[] dataSets = FileUtils.getVisibleSubDirs(serializedDir);
				String filename =
					model + "_" + StringUtils.concat(features, "+") + "_" +
					StringUtils.concat(dataSets, "+");
				File reportFile = new File(reportDir, filename);
				if (reportFile.exists()) {
					MsgPrinter.printErrorMsg("File " + reportFile +
							" already exists.");
					continue;
				}
				
				// evaluate combination
				String msg = "Evaluating model " + model + " with feature(s) " +
						StringUtils.concat(features, ", ") + " (" +
						MsgPrinter.getTimestamp() + ")...";
				MsgPrinter.printStatusMsg(StringUtils.repeat("-", msg.length()));
				MsgPrinter.printStatusMsg(msg);
				MsgPrinter.printStatusMsg(StringUtils.repeat("-", msg.length()));
				long runTime = System.currentTimeMillis();
				Evaluation eval = evaluate(serializedDir, features, model);
				runTime = System.currentTimeMillis() - runTime;
				
				// write report
				String report =
					createReport(dataSets, features, model, eval, runTime);
				try {
					FileUtils.writeString(report, reportFile, "UTF-8");
				} catch (IOException e) {
					MsgPrinter.printErrorMsg("Failed to write report to file " +
							reportFile + ":");
					MsgPrinter.printErrorMsg(e.toString());
					System.exit(1);
				}
				
				// remember combination that yields highest F1 score
				double thisF1 = eval.f1();
				if (thisF1 > maxF1) {
					maxF1 = thisF1;
					bestCombination[0] = features;
					bestCombination[1] = new String[] {model};
				}
			}
		
		return bestCombination;
	}
	
	/**
	 * Evaluates all combinations of features and models and trains a classifier
	 * using the best combination.
	 * 
	 * @param args {directory containing serialized results,
	 *              output directory for evaluation reports and classifier}
	 */
	public static void main(String[] args) {
		// enable output of status and error messages
		MsgPrinter.enableStatusMsgs(true);
		MsgPrinter.enableErrorMsgs(true);
		
		// get command line parameters
		if (args.length < 2) {
			MsgPrinter.printUsage("java ScoreNormalizationFilter " +
					"serialized_results_dir output_dir");
			System.exit(1);
		}
		String serializedDir = args[0];
		String outputDir = args[1];
		
//		// evaluate all combinations of features and models,
//		// get best combination according to F1 measure
//		String reportsDir = new File(outputDir, "reports").getPath();
//		String[][] combination = evaluateAll(serializedDir, reportsDir);
//		String[] features = combination[0];
//		String model = combination[1][0];
		// or simply get selected features and model
		String[] features = SELECTED_FEATURES;
		String model = SELECTED_MODEL;
		
		// train classifier using best/selected features and model
		String msg = "Training classifier using model " + model +
				" with feature(s) " + StringUtils.concat(features, ", ") +
				" (" + MsgPrinter.getTimestamp() + ")...";
		MsgPrinter.printStatusMsg(StringUtils.repeat("-", msg.length()));
		MsgPrinter.printStatusMsg(msg);
		MsgPrinter.printStatusMsg(StringUtils.repeat("-", msg.length()));
		Classifier classifier = train(serializedDir, features, model);
		
		// serialize classifier to file
		String classifiersDir = new File(outputDir, "classifiers").getPath();
		String[] dataSets = FileUtils.getVisibleSubDirs(serializedDir);
		String filename =
			model + "_" + StringUtils.concat(features, "+") + "_" +
			StringUtils.concat(dataSets, "+") + ".serialized";
		try {
			FileUtils.writeSerialized(classifier,
					new File(classifiersDir, filename));
		} catch (IOException e) {
			MsgPrinter.printErrorMsg("Failed to serialize classifier to file " +
					filename + ":");
			MsgPrinter.printErrorMsg(e.toString());
			System.exit(1);
		}
		
		MsgPrinter.printStatusMsg("...done.");
	}
	
	/**
	 * Loads a serialized classifier for score normalization from a file.
	 * 
	 * @param classifierFilename filename of a serialized classifier
	 */
	public static void loadClassifier(String classifierFilename) {
		try {
			Object o = FileUtils.readSerialized(new File(classifierFilename));
			classifier = (Classifier) o;
		} catch (Exception e) {
			MsgPrinter.printErrorMsg("Failed to load classifier:");
			MsgPrinter.printErrorMsg(e.toString());
		}
	}
	
	/**
	 * Creates the filter and loads a serialized classifier from a file.
	 * 
	 * @param classifierFilename filename of a serialized classifier
	 */
	public ScoreNormalizationFilter(String classifierFilename) {
		loadClassifier(classifierFilename);
	}
	
	/**
	 * Reassigns the normalized scores for each extraction technique to ensure
	 * that the order suggested by the original scores is preserved.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return array of <code>Result</code> objects with new normalized scores
	 */
	public Result[] preserveOrderResorting(Result[] results) {
		List<Result> allResults = new ArrayList<Result>();
		
		// get answers by extractors
		Hashtable<String, ArrayList<Result>> allExtracted =
			new Hashtable<String, ArrayList<Result>>();
		for (Result result : results) {
			// only factoid answers with 1 extraction technique
			if (result.getScore() <= 0 ||
					result.getScore() == Float.POSITIVE_INFINITY ||
					result.getExtractionTechniques() == null ||
					result.getExtractionTechniques().length != 1) {
				allResults.add(result);
				continue;
			}
			String extractor = result.getExtractionTechniques()[0];
			
			ArrayList<Result> extracted = allExtracted.get(extractor);
			if (extracted == null) {
				extracted = new ArrayList<Result>();
				allExtracted.put(extractor, extracted);
			}
			extracted.add(result);
		}

		// normalize answer scores for each extractor
		for (List<Result> extracted : allExtracted.values()) {
			// sort results by their normalized scores in descending order
			Result[] factoids = extracted.toArray(new Result[extracted.size()]);
			factoids = (new NormalizedScoreSorterFilter()).apply(factoids);
			// get sorted normalized scores
			float[] normScores = new float[factoids.length];
			for (int i = 0; i < factoids.length; i++)
				normScores[i] = factoids[i].getNormScore();
			
			// sort results by their original scores in descending order
			factoids = (new ScoreSorterFilter()).apply(factoids);
			// reassign sorted normalized scores
			for (int i = 0; i < factoids.length; i++)
				factoids[i].setNormScore(normScores[i]);
			
			// merge answers
			for (Result norm : factoids) allResults.add(norm);
		}
		
		return allResults.toArray(new Result[allResults.size()]);
	}
	
	/**
	 * Calculates the average normalization factor for each extraction technique
	 * and normalizes the scores with this factor to ensure that the order
	 * suggested by the original scores is preserved. The factor is adjusted to
	 * avoid normalized scores larger 1.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return array of <code>Result</code> objects with new normalized scores
	 */
	public Result[] preserveOrderAveraging(Result[] results) {
		// get answers by extractors
		Hashtable<String, ArrayList<Result>> allExtracted =
			new Hashtable<String, ArrayList<Result>>();
		for (Result result : results) {
			// only factoid answers with 1 extraction technique
			if (result.getScore() <= 0 ||
					result.getScore() == Float.POSITIVE_INFINITY ||
					result.getExtractionTechniques() == null ||
					result.getExtractionTechniques().length != 1)
				continue;
			String extractor = result.getExtractionTechniques()[0];
			
			ArrayList<Result> extracted = allExtracted.get(extractor);
			if (extracted == null) {
				extracted = new ArrayList<Result>();
				allExtracted.put(extractor, extracted);
			}
			extracted.add(result);
		}
		
		// normalize answer scores for each extractor
		for (List<Result> extracted : allExtracted.values()) {
			// get average normalization factor
			double sumNormFactors = 0;
			float maxScore = 0;
			for (Result factoid : extracted) {
				float score = factoid.getScore();
				float normScore = factoid.getNormScore();
				
				sumNormFactors += normScore / score;
				if (score > maxScore) maxScore = score;
			}
			double avgNormFactor = sumNormFactors / extracted.size();
			// adjust factor if it results in normalized scores > 1
//			if (maxScore * avgNormFactor > 1) avgNormFactor = 1 / maxScore;
			
			// normalize scores with average normalization factor
			for (Result factoid : extracted) {
				float norm = (float) (factoid.getScore() * avgNormFactor);
				factoid.setNormScore(norm);
			}
		}
		
		return results;
	}
	
	/**
	 * Calculates the normalization factor of the top answer for each extraction
	 * technique and normalizes the scores with this factor to ensure that the
	 * order suggested by the original scores is preserved.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return array of <code>Result</code> objects with new normalized scores
	 */
	public Result[] preserveOrderTop(Result[] results) {
		// get answers by extractors
		Hashtable<String, ArrayList<Result>> allExtracted =
			new Hashtable<String, ArrayList<Result>>();
		for (Result result : results) {
			// only factoid answers with 1 extraction technique
			if (result.getScore() <= 0 ||
					result.getScore() == Float.POSITIVE_INFINITY ||
					result.getExtractionTechniques() == null ||
					result.getExtractionTechniques().length != 1)
				continue;
			String extractor = result.getExtractionTechniques()[0];
			
			ArrayList<Result> extracted = allExtracted.get(extractor);
			if (extracted == null) {
				extracted = new ArrayList<Result>();
				allExtracted.put(extractor, extracted);
			}
			extracted.add(result);
		}

		// normalize answer scores for each extractor
		for (List<Result> extracted : allExtracted.values()) {
			// get 	normalization factor of top answer
			float maxScore = 0;
			float maxNormScore = 0;
			for (Result factoid : extracted) {
				float score = factoid.getScore();
				float normScore = factoid.getNormScore();
				
				if (score > maxScore) {
					maxScore = score;
					maxNormScore = normScore;
				}
			}
			double topNormFactor = maxNormScore / maxScore;
			
			// normalize scores with average normalization factor
			for (Result factoid : extracted) {
				float norm = (float) (factoid.getScore() * topNormFactor);
				factoid.setNormScore(norm);
			}
		}
		
		return results;
	}
	
	/**
	 * Normalizes the scores of the factoid answers, using the features
	 * specified in <code>SELECTED_FEATURES</code> and the classifier specified
	 * in <code>classifier</code>.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return array of <code>Result</code> objects with normalized scores
	 */
	public Result[] apply(Result[] results) {
		if (classifier == null) return results;  // classifier not loaded
		
		for (Result result : results) {
			// only factoid answers with 1 extraction technique
			if (result.getScore() <= 0 ||
					result.getScore() == Float.POSITIVE_INFINITY ||
					result.getExtractionTechniques() == null ||
					result.getExtractionTechniques().length != 1)
				continue;
			
			// create instance with selected features
	        Instance instance = createInstance(SELECTED_FEATURES, result,
	        		results);
	        // classify instance
	        ClassLabel label = classifier.classification(instance);
	        // get weight of positive class as result score
	        double weight = label.posProbability();
	        result.setNormScore((float) weight);
		}
		
		// preserve original order of results
//		results = preserveOrderResorting(results);
//		results = preserveOrderAveraging(results);
		results = preserveOrderTop(results);
		
//		for (Result result : results) result.setScore(result.getNormScore());
		
		return results;
	}
}

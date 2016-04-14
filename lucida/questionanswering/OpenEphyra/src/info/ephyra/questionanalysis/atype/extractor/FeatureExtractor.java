package info.ephyra.questionanalysis.atype.extractor;

import info.ephyra.questionanalysis.atype.minorthird.hierarchical.HierarchicalClassifier;
import info.ephyra.util.Properties;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.log4j.Logger;

import edu.cmu.lti.javelin.qa.Term;
import edu.cmu.lti.javelin.util.FileUtil;
import edu.cmu.lti.javelin.util.IOUtil;
import edu.cmu.minorthird.classify.ClassLabel;
import edu.cmu.minorthird.classify.Example;
import edu.cmu.minorthird.classify.Feature;
import edu.cmu.minorthird.classify.Instance;

/**
 * A feature extractor for question classification.  The most important 
 * functionality is provided by the {@link #createInstance(List, String) createInstance}
 * method (a couple convenience versions also provided) which create
 * an edu.cmu.minorthird.classify.Instance object from basic question data 
 * (the original question and its syntactic parse tree). 
 * 
 * createInstance can be used to extraction features for run-time classification 
 * feature extraction.  It is also used 
 * when loading edu.cmu.minorthird.classify.Example objects from a dataset
 * file at training time (see  {@link #loadFile(String) loadFile} and 
 * {@link #createExample(String) createExample}).  Thus, feature extraction for classification
 * is accomplished by the same code for both training and run-time classification.
 * 
 * An important thing for subclassing classes to note is that the Instance returned 
 * by a createInstance(...) method must have the original question, as 
 * a String, as it's source.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public abstract class FeatureExtractor{

    private static final Logger log = Logger.getLogger(FeatureExtractor.class);

    protected static String SPACE_PTRN = "\\s+";
    
    /**
     * Regular expression describing the format of a line in a question classification dataset.  
     * The answer type label, the actual question, and the syntactic parse tree are the fields
     * that must be captured by groups, with the group indices specified by {@link #labelPosition},
     * {@link #questionPosition}, and {@link #parsePosition}, respectively. 
     */
    protected Pattern datasetExamplePattern = Pattern.compile(
            "^(\\S+)\\s+"+ // answer type
            "\\S+:\\s+"+ // question id
            "(?:.+?;\\s+)?"+ // topic (optional)
            "(.+)[\\?\\.]?\\s+"+ // question
            "(\\(ROOT.+\\))\\s*$"+ // parse tree
            ""
            ,Pattern.MULTILINE);

    /**
     * The captured group index of the answer type label in the dataset line Pattern.
     */
    protected int labelPosition = 1;
    /**
     * The captured group index of the question in the dataset line Pattern.
     */
    protected int questionPosition = 2;
    /**
     * The captured group index of the syntactic parse tree in the dataset line Pattern.
     */
    protected int parsePosition = 3;

    protected int classLevels;
    protected boolean useClassLevels;
    protected int numLoaded = 0;
    protected boolean isInitialized = false;

    /**
     * Reads in properties from this class's properties file and sets class data members.
     * 
     * @throws Exception
     */
    public void initialize() throws Exception {
        Properties properties = Properties.loadFromClassName(FeatureExtractor.class.getName());

        String classLevelsProp = properties.getProperty("classLevels");
        if (classLevelsProp == null)
            throw new RuntimeException("Required parameter classLevels is undefined");
        classLevels = Integer.parseInt(classLevelsProp);
        
        String useClassLevelsProp = properties.getProperty("useClassLevels");
        if (useClassLevelsProp == null)
            throw new RuntimeException("Required parameter useClassLevels is undefined");
        useClassLevels = Boolean.parseBoolean(useClassLevelsProp);
    }

    /**
     * Given a question as a list of Terms and it's syntactic parse tree, 
     * creates a Instance for question classification by extracting the 
     * appropriate features.
     * 
     * @param terms the Terms of the question
     * @param parseTree the syntactic parse tree of the question
     * @return an Instance which can be used for question classification
     */
    public abstract Instance createInstance(List<Term> terms, String parseTree);

    /**
     * Convenience method that tokenizes the given question by whitespace, creates
     * Terms, and calls {@link #createInstance(List, String)}.
     * 
     * @param question the question to create an Instance from
     * @param parseTree the syntactic parse tree of the question
     */
    public Instance createInstance(String question, String parseTree){
        String[] tokens = question.split("\\s+");
        List<Term> terms = new ArrayList<Term>();
        for (String token : tokens) {
            terms.add(new Term(0,0,token));
        }
        return createInstance(terms,parseTree);
    }

    /**
     * Creates an Instance for question classification when nothing but the
     * original question is available for feature extraction. Assumes words 
     * in the input question are separated by white-space.
     * 
     * @param question the input question
     * @return the Instance object 
     */
    public abstract Instance createInstance(String question);

    /**
     * Creates an edu.cmu.minorthird.classify.Example object from one line
     * of a dataset file using {@link #createInstance(String, String)}.
     * 
     * @param datasetLine the line from the dataset file from which to create
     * the Example 
     * @return the Example created
     * @throws Exception
     */
    public Example[] createExample(String datasetLine) throws Exception {
        Matcher m=datasetExamplePattern.matcher(datasetLine);

        if (!m.matches()) 
            throw new Exception("Malformed dataset line:\n"+datasetLine);

        String[] aTypes = null;

        aTypes = m.group(labelPosition)
                    .replaceAll(",$", "")   
                    .replaceAll(",", ".")
                    .split("\\|");
        String question = m.group(questionPosition);
        String sentParse = null;
        if (parsePosition > -1) sentParse = m.group(parsePosition);

        Instance instance = createInstance(question,sentParse);

        Example[] result = new Example[aTypes.length];

        //create example(s) and add it to list
        for(int i=0;i<aTypes.length;i++){
            String newATypeName=HierarchicalClassifier.getHierarchicalClassName(aTypes[i],classLevels,useClassLevels);
            result[i] = new Example(instance,new ClassLabel(newATypeName));
        }
        return result;
    }

    /**
     * Loads an array of edu.cmu.minorthird.classify.Example objects from the file
     * at the given location, using {@link #datasetExamplePattern} and
     * {@link #createExample(String) createExample}.
     * 
     * @param fileName the name of the dataset file
     */
    public Example[] loadFile(String fileName) {
        List<Example> examples = new ArrayList<Example>();
        String data=FileUtil.readFile(fileName,"UTF-8");
        Matcher m = datasetExamplePattern.matcher(data);
        while (m.find()) {
            try {
                Example[] exampleArr = createExample(m.group());
                for (Example example : exampleArr) {
                    examples.add(example);
                    numLoaded++;
                }
            } catch (Exception e) {
                log.error("Error reading Example from file: ",e);
            }
        }
        return (Example[])examples.toArray(new Example[examples.size()]);
    }
    

    /**
     * Prints the features generated for each example in an input file.  If feature
     * types are included as command-line arguments, only those types are printed. 
     * Otherwise, all features are printed.
     * 
     * @param dataSetFileName the name of the file containing the dataset to load
     * @param features a List of the features to print
     */
    public void printFeatures (String dataSetFileName, List<String> features) {
        Example[] examples=loadFile(dataSetFileName);

        for(int i=0;i<examples.length;i++){
            String src = (String)examples[i].getSource();
            StringBuilder sb = new StringBuilder();
            if (features.size() > 0) {
                for(Iterator it = examples[i].binaryFeatureIterator(); it.hasNext();) {
                    Feature feat = (Feature)it.next();
                    String name = "";
                    for (String s : feat.getName()) name += "."+s;
                    name = name.replaceFirst(".","");
                    if (features.contains(feat.getName()[0]))
                        sb.append(name+"  ");
                }
                System.out.println(sb.toString() + " " + src);
            }
            else System.out.println(examples[i] + " " + src);
        }

        System.out.println("Loaded: "+getNumLoaded());
    }

    /**
     * Prints the features generated for each example in an input file.  If feature
     * types are included as command-line arguments, only those types are printed. 
     * Otherwise, all features are printed.
     * 
     * @param questionSetFileName the name of the file containing the dataset to load
     * @param features a List of the features to print
     */
    public void printFeaturesFromQuestions (String questionSetFileName, List<String> features) {
        String questions = IOUtil.readFile(questionSetFileName);
        
        for (String question : questions.split("[\\n\\r\\f]")) {
            Instance instance = createInstance(question);
            StringBuilder sb = new StringBuilder();
            if (features.size() > 0) {
                for(Iterator it = instance.binaryFeatureIterator(); it.hasNext();) {
                    Feature feat = (Feature)it.next();
                    String name = "";
                    for (String s : feat.getName()) name += "."+s;
                    name = name.replaceFirst(".","");
                    if (features.contains(feat.getName()[0]))
                        sb.append(name+"  ");
                }
                System.out.println(sb.toString() + " " + question);
            }
            else System.out.println(instance + " " + question);
        }
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
     * @return the number of examples loaded
     */
    public int getNumLoaded(){
        return numLoaded;
    }


    public void setClassLevels(int classLevels){
        this.classLevels=classLevels;
    }

    public int getClassLevels(){
        return classLevels;
    }

    public void setUseClassLevels(boolean useClassLevels){
        this.useClassLevels=useClassLevels;
    }

    public boolean isUsingClassLevels(){
        return useClassLevels;
    }

    /**
     * @return the datasetExamplePattern
     */
    public Pattern getDatasetExamplePattern() {
        return datasetExamplePattern;
    }

    /**
     * @param datasetExamplePattern the datasetExamplePattern to set
     */
    public void setDatasetExamplePattern(Pattern datasetExamplePattern) {
        this.datasetExamplePattern = datasetExamplePattern;
    }

    /**
     * @return the labelPosition
     */
    public int getLabelPosition() {
        return labelPosition;
    }

    /**
     * @param labelPosition the labelPosition to set
     */
    public void setLabelPosition(int labelPosition) {
        this.labelPosition = labelPosition;
    }

    /**
     * @return the parsePosition
     */
    public int getParsePosition() {
        return parsePosition;
    }

    /**
     * @param parsePosition the parsePosition to set
     */
    public void setParsePosition(int parsePosition) {
        this.parsePosition = parsePosition;
    }

    /**
     * @return the questionPosition
     */
    public int getQuestionPosition() {
        return questionPosition;
    }

    /**
     * @param questionPosition the questionPosition to set
     */
    public void setQuestionPosition(int questionPosition) {
        this.questionPosition = questionPosition;
    }
}


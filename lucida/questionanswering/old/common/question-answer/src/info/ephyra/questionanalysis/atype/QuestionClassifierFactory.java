package info.ephyra.questionanalysis.atype;

import info.ephyra.util.Properties;

import java.util.Map;

import edu.cmu.lti.javelin.util.Language;
import edu.cmu.lti.util.Pair;

/**
 * Factory class for generating a question classifier. See {@link #getInstance}.
 * A main method for evaluation a classifier against a test set is also provided.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class QuestionClassifierFactory {
    
//  private static final Logger log = Logger.getLogger(QuestionClassifierFactory.class);
    private static Map<String,Properties> propertyMap;
    
    static {
        Properties properties = Properties.loadFromClassName(QuestionClassifierFactory.class.getName());
        propertyMap = properties.mapProperties();
    }
    
    /**
     * Returns a QuestionClassifier for the specified Language Pair. Configure which
     * QuestionClassifier is associated with each Language Pair in this class's properties 
     * file.
     */
    public static QuestionClassifier getInstance(Pair<Language, Language> languagePair) throws Exception {
        QuestionClassifier classifier = null;
        Properties properties = propertyMap.get(languagePair.getFirst()+"_"+languagePair.getSecond());
        String classifierType = properties.getProperty("classifierType");
        if (classifierType == null) 
            throw new Exception("Required property classifierType is undefined for language pair "+languagePair);
        classifier = (QuestionClassifier)Class.forName(classifierType).newInstance();
        classifier.setLanguagePair(languagePair);
        classifier.initialize();
        return classifier;
    }
    
    /**
     * Given a language pair and a test set filename, evaluates the classifier
     * for that language pair on the test set.
     * 
     * @param args command-line args: &lt;languagePair&gt; &lt;testSetFileName&gt;
     * @throws Exception
     */
    public static void main (String[] args) throws Exception {
        if (args.length < 3 || args.length > 4 || (args.length == 3 && args[2].equals("--eval"))) {
            System.err.println("USAGE: QuestionClassifierFactory <questionLang> <corpusLang> (<questionFile> | --eval <goldStandardXmlFile>)\n");
            System.err.println(" - <questionLang> and <corpusLang> must be one of the following:");
            System.err.println("      en_US, ja_JP, jp_JP, zh_TW, zh_CN");
            System.err.println(" - Writes the result of classification for each input question to STDOUT");
            System.err.println("      if only a <questionFile> is given");
            System.err.println(" - Otherwise, writes classification accuracy to STDOUT if --eval and ");
            System.err.println("      a <goldStandardXmlFile> are given");
            System.err.println("   -- in this case, classification errors can be seen if logging ");
            System.err.println("        is set to DEBUG in conf/log4j.properties");
            System.exit(0);
        }
        
        QuestionClassifier classifier = QuestionClassifierFactory.getInstance(
                new Pair<Language,Language>(
                        Language.valueOf(args[0]),
                        Language.valueOf(args[1])));
        
        if (args.length == 3 && !args[2].equals("--eval"))
                classifier.classifySet(args[2]);
        else if (args.length == 4 && args[2].equals("--eval"))  
            classifier.evaluate(args[3]);
    }
}

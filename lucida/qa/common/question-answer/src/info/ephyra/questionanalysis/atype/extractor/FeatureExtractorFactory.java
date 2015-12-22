package info.ephyra.questionanalysis.atype.extractor;

import info.ephyra.util.Properties;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import edu.cmu.lti.javelin.util.Language;

/**
 * Creates a feature extractor for a specific language.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class FeatureExtractorFactory {
    private static Map<String,Properties> propertyMap;
    
    static {
        Properties properties = Properties.loadFromClassName(FeatureExtractorFactory.class.getName());
        propertyMap = properties.mapProperties();
    }
    
    public static FeatureExtractor getInstance(Language language) throws Exception {
        FeatureExtractor extractor = null;
        Properties properties = propertyMap.get(language.toString());
        String extractorType = properties.getProperty("extractorType");
        if (extractorType == null) 
            throw new Exception("Required property extractorType is undefined for language"+language);
        extractor = (FeatureExtractor)Class.forName(extractorType).newInstance();
        extractor.initialize();
        return extractor;
    }

    /**
     * Invokes {@link info.ephyra.questionanalysis.atype.extractor.FeatureExtractor#printFeatures} with feature names specified on the command line.
     * 
     * @param args command-line arguments: "&lt;Language&gt; &lt;datasetFile&gt; [&lt;feature1&gt; &lt;feature2&gt; ...]" 
     * @throws Exception
     */
    public static void main(String[] args) throws Exception {
        if (args.length < 2) {
            System.out.println("USAGE: FeatureExtractorFactory <language> [--parses] <datasetFile> [<feature1> <feature2> ...]");
            System.out.println("<language> can be one of: en_US, ja_JP, zh_CH, zh_TW");
            System.out.println("Output to System.out");
            System.exit(0);
        }

        FeatureExtractor extractor = FeatureExtractorFactory.getInstance(Language.valueOf(args[0]));
        extractor.initialize();

        extractor.setClassLevels(1);
        extractor.setUseClassLevels(false);

        List<String> features = new ArrayList<String>();
        int fileInd = 1;
        boolean parses = false;
        if (args[1].equals("--parses")) {
            parses = true;
            fileInd = 2;
        }
        
        for (int i=fileInd+1; i <= args.length-1; i++) 
            features.add(args[i]);
        
        if (parses)
            extractor.printFeatures(args[fileInd], features);
        else 
            extractor.printFeaturesFromQuestions(args[fileInd], features);
    }

}

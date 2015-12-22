package info.ephyra.questionanalysis.atype.classifier;

import info.ephyra.questionanalysis.atype.AnswerType;
import info.ephyra.questionanalysis.atype.QuestionClassifier;
import info.ephyra.util.Properties;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.apache.log4j.Logger;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import edu.cmu.minorthird.classify.Feature;
import edu.cmu.minorthird.classify.Instance;

/**
 * A rule-based classifier for the answer type of a question.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class RuleBasedQuestionClassifier extends QuestionClassifier {

    private static final Logger log = Logger.getLogger(RuleBasedQuestionClassifier.class);
    private List<Rule> rules;
    private String defaultLabel;

    public RuleBasedQuestionClassifier() {}
    
    public void initialize() throws Exception {
        if (isInitialized()) return;
        if (languagePair == null)
            throw new Exception("languagePair must be set before calling initialize");
 
        super.initialize();

        Properties properties = Properties.loadFromClassName(this.getClass().getName());
        
        properties = properties.mapProperties().get(languagePair.getFirst()+"_"+languagePair.getSecond());
        
        String rulesFileName = properties.get("rulesFile");
        if (rulesFileName == null)
            throw new RuntimeException("Required property rulesFile is undefined'"); 

        rules = new ArrayList<Rule>();
        loadRulesFile(rulesFileName);
        
        setInitialized(true);
    }
    
    public List<AnswerType> classify(Instance instance) {
        List<AnswerType> res = new ArrayList<AnswerType>();
        for (Rule rule : rules) {
            AnswerType result = rule.matches(instance);
            if (result != null) {
                res.add(result);
                //return res;
            }
        }
        return res;
    }
    
    /**
     * Retrieves the value of the given feature from the given Instance.
     * 
     * @param instance the Instance to consider
     * @param featName the name of the feature
     */
    public static String getFeatureValue (Instance instance, String featName) {
        for (Iterator it = instance.binaryFeatureIterator(); it.hasNext();) {
            Feature f = (Feature)it.next();
            if (f.getPart(0).equals(featName)) {
                String val = f.getPart(1).trim();
                val = val.equals("-") ? null : val;
                return val;
            }
        }
        return null;
    }
    
    private void loadRulesFile(String fileName){
        // PARSE XML RULES FILE 
        log.debug("Parsing xml rules file");
        Document rulesDocument;
        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            factory.setValidating(false);
            factory.setIgnoringComments(true);
            factory.setIgnoringElementContentWhitespace(true);
            factory.setNamespaceAware(true);
            DocumentBuilder db = factory.newDocumentBuilder();
            rulesDocument = db.parse(fileName);
        } catch (Exception e) {
            throw new RuntimeException("Failed to parse XML patterns file", e);
        }

        // EXTRACT RULE DATA.
        log.debug("Loading rules");
        
        NodeList ruleList = rulesDocument.getElementsByTagName("RULE");
        for (int i = 0; i < ruleList.getLength(); i++) {
            Node rule = ruleList.item(i);
            if (!rule.getNodeName().equals("RULE") ||
                rule.getNodeType() != Node.ELEMENT_NODE) continue;
            rules.add(new Rule((Element)rule));
        }
    }

    /**
     * @return the defaultLabel
     */
    public String getDefaultLabel() {
        return defaultLabel;
    }

    /**
     * @param defaultLabel the defaultLabel to set
     */
    public void setDefaultLabel(String defaultLabel) {
        this.defaultLabel = defaultLabel;
    }
}



package info.ephyra.questionanalysis.atype.classifier;

import info.ephyra.questionanalysis.atype.AnswerType;

import java.io.StringReader;
import java.util.ArrayList;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

import edu.cmu.minorthird.classify.Feature;
import edu.cmu.minorthird.classify.Instance;
import edu.cmu.minorthird.classify.MutableInstance;

/**
 * A rule used for rule-based classification of answer type.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class Rule {

    private String atype;
    private double confidence;
    private List<RuleElement> elements;
    
    /**
     * Instantiates a Rule object from an XML Element.  
     * @param ruleElement the Element to construct a Rule from
     */
    public Rule (Element ruleElement) {
        NamedNodeMap ruleAttributes = ruleElement.getAttributes();
        this.atype = ruleAttributes.getNamedItem("atype").getNodeValue().toString();
        this.confidence = Double.parseDouble(ruleAttributes.getNamedItem("conf").getNodeValue().toString());
        
        this.elements = new ArrayList<RuleElement>();
        NodeList ruleElementList = ruleElement.getChildNodes();
        for (int j = 0; j < ruleElementList.getLength(); j++) {
            Node ruleElementElement = ruleElementList.item(j);
            if (!ruleElementElement.getNodeName().equals("RULE_ELEMENT")) continue;
            elements.add(new RuleElement((Element)ruleElementElement));
        }
    }
    

    /**
     * Tells whether a given Instance matches this Rule by returning
     * the appropriate answer type if it does match, and <code>null</code> 
     * if it does not.
     * 
     * @param instance the Instance to consider
     * @return the answer type and subtype formed by matching this Rule to the
     * given Instance, or <code>null</code> if this Rule does not match the Instance.
     * @throws IllegalStateException if this Rule has not been compiled yet
     */
    public AnswerType matches (Instance instance) {
        if (elements == null) 
            throw new IllegalStateException("Must compile rule before using it");
        
        String retAtype = atype;
        AnswerType res = null;
        // All elements of the rule must match in order for the rule to match
        for (RuleElement element : elements) {
            String result = element.matches(instance);
            if (result == null) {
                return null;
            } else if (element.getFeature().equals("FOCUS_TYPE")) {
                result = result.toUpperCase();
                if (!result.equals(atype) && result != "")
                    retAtype = atype + "." + result;
            }
        }
        res = AnswerType.constructFromString(retAtype);
        res.setConfidence(confidence);
        return res;
    }
        
    /**
     * @param atype the atype to set
     */
    public void setAtype(String atype) {
        this.atype = atype;
    }

    /**
     * @return the elements
     */
    public List<RuleElement> getElements() {
        return elements;
    }

    /**
     * @param elements the elements to set
     */
    public void setElements(List<RuleElement> elements) {
        this.elements = elements;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Atype: " + atype + "\n");
        if (elements == null) sb.append("null");
        else {
            for (RuleElement re : elements) {
                sb.append("Element:\n" + re.toString() + "\n");
            }
        }
        return sb.toString();
    }
    
    /**
     * Tests Rule creation, compilation and matching.
     * 
     * @param args
     */
    public static void main(String[] args) {
        String test = "<RULE atype=\"TEST_TYPE\">" +
                        "<RULE_ELEMENT feature_name=\"TEST_FEATURE1\">" +    
                            "<FEATURE_VALUE>value1</FEATURE_VALUE>" +
                            "<FEATURE_VALUE>value2</FEATURE_VALUE>" +
                            "<FEATURE_VALUE>value3</FEATURE_VALUE>" +
                        "</RULE_ELEMENT>" +
                        "<RULE_ELEMENT feature_name=\"FOCUS_TYPE\">" +    
                            "<FEATURE_VALUE>value3=</FEATURE_VALUE>" +
                            "<FEATURE_VALUE>value4=new</FEATURE_VALUE>" +
                        "</RULE_ELEMENT>" +    
                      "</RULE>";    
        Document ruleDocument;
        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            factory.setValidating(false);
            factory.setIgnoringComments(true);
            factory.setIgnoringElementContentWhitespace(true);
            factory.setNamespaceAware(true);
            DocumentBuilder db = factory.newDocumentBuilder();
            ruleDocument = db.parse(new InputSource(new StringReader(test)));
            Rule r = new Rule(ruleDocument.getDocumentElement());
            System.out.println("Test input: " + test);
            System.out.println(r.toString());
            
            MutableInstance testInstance = new MutableInstance(test);
            testInstance.addBinary(new Feature("TEST_FEATURE1.value1"));
            testInstance.addBinary(new Feature("FOCUS_TYPE.value4"));
            System.out.println("Test instance: " + testInstance);
            System.out.println("matches test rule?: " + r.matches(testInstance));
            testInstance = new MutableInstance(test);
            testInstance.addBinary(new Feature("TEST_FEATURE1.value1"));
            testInstance.addBinary(new Feature("FOCUS_TYPE.value3"));
            System.out.println("Test instance: " + testInstance);
            System.out.println("matches test rule?: " + r.matches(testInstance));
        } catch (Exception e) {
            throw new RuntimeException("Failed to parse XML string", e);
        }
    }

}

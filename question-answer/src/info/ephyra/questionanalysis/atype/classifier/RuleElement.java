package info.ephyra.questionanalysis.atype.classifier;

import java.io.StringReader;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

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

/**
 * An element in a rule used for rule-based classification of answer type.  Each
 * RuleElement corresponds to a feature with a set of values with which to match a
 * given Instance.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class RuleElement {

    private String feature;
    private Set<String> values;
    
    /**
     * Instantiates a RuleElement from an XML Element.  
     * @param ruleElementElement the Element to construct a Rule from
     */    
    public RuleElement(Element ruleElementElement) {
        NamedNodeMap ruleElementAttributes = ruleElementElement.getAttributes();
        this.feature = ruleElementAttributes.getNamedItem("feature_name").getNodeValue().toString();
        
        this.values = new HashSet<String>();
        NodeList featureValueList = ruleElementElement.getChildNodes();
        for (int j = 0; j < featureValueList.getLength(); j++) {
            Node featureValueElement = featureValueList.item(j);
            if (!featureValueElement.getNodeName().equals("FEATURE_VALUE")) continue;
            values.add(featureValueElement.getChildNodes().item(0).getNodeValue());
        }
    }

    /**
     * Tells whether a given Instance has a feature that matches this RuleElement. 
     * by returning the matching value upon a successful match or <code>null</code>
     * if this RuleElement does not match the Instance.  When one of 
     * this RuleElement's values contains the substring "->", the portion of the 
     * value before the "->" is used for matching against the input Instance, while
     * the portion of the value after the "->" is used as the matching value;
     * 
     * @param instance the Instance to consider
     * @return whether a given Instance has a feature that matches this RuleElement
     * @throws IllegalStateException if this RuleElement has not been compiled yet
     */
    public String matches (Instance instance) {
        if (values == null) 
            throw new IllegalStateException("values not initialized");
        for (Iterator it = instance.binaryFeatureIterator(); it.hasNext();) {
            Feature f = (Feature)it.next();
            if (!f.getPart(0).equals(feature)) continue;
            for (String value : values) {
                String[] parts = value.split("=",-1);
                if (parts[0].equals(f.getPart(1).trim())) {
                    return (parts.length == 2) ? parts[1] : f.getPart(1).trim();
                }
            }
        }
        return null;
    }

    /**
     * @return the feature
     */
    public String getFeature() {
        return feature;
    }

    /**
     * @param feature the feature to set
     */
    public void setFeature(String feature) {
        this.feature = feature;
    }

    /**
     * @return the values
     */
    public Set<String> getValues() {
        return values;
    }

    /**
     * @param values the values to set
     */
    public void setValues(Set<String> values) {
        this.values = values;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Feature: " + feature + "\n");
        sb.append("Values: " + "\n");
        if (values == null) sb.append("null");
        else {
            for (String v : values) {
                sb.append("  " + v + "\n");
            }
        }
        return sb.toString();
    }

    /**
     * Tests RuleElement creation, compilation and matching.
     * 
     * @param args
     */
    public static void main(String[] args) throws Exception {
        String test = "<RULE_ELEMENT feature_name=\"TEST_FEATURE123\">" +
                         "<FEATURE_VALUE>value1</FEATURE_VALUE>" +
                         "<FEATURE_VALUE>value2</FEATURE_VALUE>" +
                         "<FEATURE_VALUE>value3</FEATURE_VALUE>" +
                       "</RULE_ELEMENT>";    
        Document ruleElementDocument;
        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            factory.setValidating(false);
            factory.setIgnoringComments(true);
            factory.setIgnoringElementContentWhitespace(true);
            factory.setNamespaceAware(true);
            DocumentBuilder db = factory.newDocumentBuilder();
            ruleElementDocument = db.parse(new InputSource(new StringReader(test)));
            RuleElement re = new RuleElement(ruleElementDocument.getDocumentElement());
            System.out.println("Test input: " + test);
            System.out.println(re.toString());
        } catch (Exception e) {
            throw new RuntimeException("Failed to parse XML string", e);
        }
    }
    
}

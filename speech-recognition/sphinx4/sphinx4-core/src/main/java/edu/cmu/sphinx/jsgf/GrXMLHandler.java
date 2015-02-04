package edu.cmu.sphinx.jsgf;

import java.net.URL;
import java.util.ArrayList;
import java.util.Map;
import java.util.logging.Logger;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.DefaultHandler;

import edu.cmu.sphinx.jsgf.rule.JSGFRule;
import edu.cmu.sphinx.jsgf.rule.JSGFRuleAlternatives;
import edu.cmu.sphinx.jsgf.rule.JSGFRuleCount;
import edu.cmu.sphinx.jsgf.rule.JSGFRuleSequence;
import edu.cmu.sphinx.jsgf.rule.JSGFRuleToken;

public class GrXMLHandler extends DefaultHandler {

    protected final Map<String, JSGFRule> topRuleMap;
    Logger logger;
    JSGFRule currentRule;
    
    URL baseURL;
    public GrXMLHandler(URL baseURL, Map<String, JSGFRule> rules, Logger logger) {
        this.baseURL = baseURL;
        this.topRuleMap = rules;
        this.logger = logger;
    }
    
    @Override
    public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
        JSGFRule newRule = null;
        JSGFRule topRule = null;

        logger.fine("Starting element " + qName);
        if (qName.equals("rule")) {
            String id = attributes.getValue("id");
            if (id != null) {
                newRule = new JSGFRuleSequence(new ArrayList<JSGFRule>());
                topRuleMap.put(id, newRule);
                topRule = newRule;
            }
        }
        if (qName.equals("item")) {
            String repeat = attributes.getValue("repeat");
            if (repeat != null) {
                newRule = new JSGFRuleSequence(new ArrayList<JSGFRule>());
                JSGFRuleCount ruleCount = new JSGFRuleCount(newRule, JSGFRuleCount.ONCE_OR_MORE);
                topRule = ruleCount;
            } else {
                newRule = new JSGFRuleSequence(new ArrayList<JSGFRule>());
                topRule = newRule;
            }
        }
        if (qName.equals("one-of")) {
            newRule = new JSGFRuleAlternatives(new ArrayList<JSGFRule>());
            topRule = newRule;
        }
        addToCurrent(newRule, topRule);
    }    

    @Override
    public void characters(char buf[], int offset, int len)throws SAXException {
        String item = new String(buf, offset, len).trim();

        if (item.length() == 0)
            return;

        logger.fine ("Processing text " + item);

        JSGFRuleToken newRule = new JSGFRuleToken(item);
        addToCurrent(newRule, newRule);
        // Don't shift current
        currentRule = newRule.parent;
    }
    
    private void addToCurrent(JSGFRule newRule, JSGFRule topRule) {

        if (newRule == null)
            return;
        
        if (currentRule == null) {
            currentRule = newRule;
            return;
        }
        
        if (currentRule instanceof JSGFRuleSequence) {
            JSGFRuleSequence ruleSequence = (JSGFRuleSequence) currentRule;
            ruleSequence.append(topRule);
            newRule.parent = currentRule;
            currentRule = newRule;
        } else if (currentRule instanceof JSGFRuleAlternatives) {
            JSGFRuleAlternatives ruleAlternatives = (JSGFRuleAlternatives) currentRule;
            ruleAlternatives.append(topRule);
            newRule.parent = currentRule;
            currentRule = newRule;
        }        
    }

    
    @Override
    public void endElement(String uri, String localName, String qName) throws SAXParseException {
        logger.fine ("Ending element " + qName);
        
        if (qName.equals("item") || qName.equals("one-of") || qName.equals("rule"))
            currentRule = currentRule.parent;
    }
}

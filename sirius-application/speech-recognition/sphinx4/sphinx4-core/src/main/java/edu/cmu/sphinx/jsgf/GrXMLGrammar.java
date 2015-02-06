/*
 * Copyright 1999-2010 Carnegie Mellon University.  
 * Portions Copyright 2010 PC-NG Inc.  
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.jsgf;

import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.util.HashMap;
import java.util.Map;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;
import org.xml.sax.XMLReader;

import edu.cmu.sphinx.jsgf.rule.JSGFRule;
import edu.cmu.sphinx.linguist.language.grammar.GrammarNode;
import edu.cmu.sphinx.util.props.S4String;

/**
 * Grammar for GrXML W3C Standard
 * @author shmyrev
 *
 */
public class GrXMLGrammar extends JSGFGrammar {

    Map<String, JSGFRule> rules;
    /** The property that defines the location of the JSGF grammar file. */
    @S4String
    public final static String PROP_BASE_GRAMMAR_URL = "grammarLocation";

    protected void loadXML() throws IOException {
        try {
            SAXParserFactory factory = SAXParserFactory.newInstance();
            XMLReader xr = factory.newSAXParser().getXMLReader();
            rules = new HashMap<String, JSGFRule>();
            GrXMLHandler handler = new GrXMLHandler(baseURL, rules, logger);
            xr.setContentHandler(handler);
            xr.setErrorHandler(handler);
            InputStream is = baseURL.openStream();
            xr.parse(new InputSource(is));
            is.close();
        } catch (SAXParseException e) {
            String msg = "Error while parsing line " + e.getLineNumber() + " of " + baseURL + ": " + e.getMessage();
            throw new IOException(msg);
        } catch (SAXException e) {
            throw new IOException("Problem with XML: " + e);
        } catch (ParserConfigurationException e) {
            throw new IOException(e.getMessage());
        }

        return;
    }
    
    /**
     * Commit changes to all loaded grammars and all changes of grammar since
     * the last commitChange
     * 
     * @throws JSGFGrammarParseException
     * @throws JSGFGrammarException
     */
    @Override
    public void commitChanges() throws IOException, JSGFGrammarParseException,
            JSGFGrammarException {
        try {
            if (loadGrammar) {
                if (manager == null)
                    getGrammarManager();
                loadXML();
                loadGrammar = false;
            }

            ruleStack = new RuleStack();
            newGrammar();

            firstNode = createGrammarNode("<sil>");
            GrammarNode finalNode = createGrammarNode("<sil>");
            finalNode.setFinalNode(true);

            // go through each rule and create a network of GrammarNodes
            // for each of them

            for (Map.Entry<String, JSGFRule> entry : rules.entrySet()) {
                
                    GrammarGraph publicRuleGraph = new GrammarGraph();
                    ruleStack.push(entry.getKey(), publicRuleGraph);
                    GrammarGraph graph = processRule(entry.getValue());
                    ruleStack.pop();

                    firstNode.add(publicRuleGraph.getStartNode(), 0.0f);
                    publicRuleGraph.getEndNode().add(finalNode, 0.0f);
                    publicRuleGraph.getStartNode().add(graph.getStartNode(),
                            0.0f);
                    graph.getEndNode().add(publicRuleGraph.getEndNode(), 0.0f);
            }
            postProcessGrammar();
        } catch (MalformedURLException mue) {
            throw new IOException("bad base grammar URL " + baseURL + ' ' + mue);
        }
    }

}

/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2002 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.jsgf;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import edu.cmu.sphinx.jsgf.parser.JSGFParser;
import edu.cmu.sphinx.jsgf.rule.*;
import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.linguist.language.grammar.Grammar;
import edu.cmu.sphinx.linguist.language.grammar.GrammarNode;
import edu.cmu.sphinx.util.LogMath;
import edu.cmu.sphinx.util.props.ConfigurationManagerUtils;
import edu.cmu.sphinx.util.props.PropertyException;
import edu.cmu.sphinx.util.props.PropertySheet;
import edu.cmu.sphinx.util.props.S4String;

/**
 * <h3>Defines a BNF-style grammar based on JSGF grammar rules in a file.</h3>
 * 
 * 
 * The Java Speech Grammar Format (JSGF) is a BNF-style, platform-independent,
 * and vendor-independent textual representation of grammars for use in speech
 * recognition. It is used by the <a
 * href="http://java.sun.com/products/java-media/speech/">Java Speech API
 * (JSAPI) </a>.
 * 
 * Here we only intend to give a couple of examples of grammars written in JSGF,
 * so that you can quickly learn to write your own grammars. For more examples
 * and a complete specification of JSGF, go to
 * 
 * <a href="http://java.sun.com/products/java-media/speech/forDevelopers/JSGF/">
 * http://java.sun.com/products/java-media/speech/forDevelopers/JSGF/ </a>.
 * 
 * 
 * <h3>Example 1: "Hello World" in JSGF</h3>
 * 
 * The example below shows how a JSGF grammar that generates the sentences
 * "Hello World":
 * 
 * <pre>
 *  #JSGF V1.0
 *  public &lt;helloWorld&gt; = Hello World;
 * </pre>
 * 
 * <i>Figure 1: Hello grammar that generates the sentences "Hello World". </i>
 * <p/>
 * 
 * The above grammar is saved in a file called "hello.gram". It defines a public
 * grammar rule called "helloWorld". In order for this grammar rule to be
 * publicly accessible, we must be declared it "public". Non-public grammar
 * rules are not visible outside of the grammar file.
 * 
 * The location of the grammar file(s) is(are) defined by the
 * {@link #PROP_BASE_GRAMMAR_URL baseGrammarURL}property. Since all JSGF grammar
 * files end with ".gram", it will automatically search all such files at the
 * given URL for the grammar. The name of the grammar to search for is specified
 * by {@link #PROP_GRAMMAR_NAME grammarName}. In this example, the grammar name
 * is "helloWorld".
 * 
 * <h3>Example 2: Command Grammar in JSGF</h3>
 * 
 * This examples shows a grammar that generates basic control commands like
 * "move a menu thanks please", "close file",
 * "oh mighty computer please kindly delete menu thanks". It is the same as one
 * of the command & control examples in the <a
 * href="http://java.sun.com/products/java-media/speech/forDevelopers/JSGF/"
 * >JSGF specification </a>. It is considerably more complex than the previous
 * example. It defines the public grammar called "basicCmd".
 * 
 * <pre>
 *  #JSGF V1.0
 *  public &lt;basicCmd&gt; = &lt;startPolite&gt; &lt;command&gt; &lt;endPolite&gt;;
 *  &lt;command&gt; = &lt;action&gt; &lt;object&gt;;
 *  &lt;action&gt; = /10/ open |/2/ close |/1/ delete |/1/ move;
 *  &lt;object&gt; = [the | a] (window | file | menu);
 *  &lt;startPolite&gt; = (please | kindly | could you | oh mighty computer) *;
 *  &lt;endPolite&gt; = [ please | thanks | thank you ];
 * </pre>
 * 
 * <i>Figure 2: Command grammar that generates simple control commands. </i>
 * <p/>
 * 
 * The features of JSGF that are shown in this example includes:
 * <ul>
 * <li>using other grammar rules within a grammar rule.
 * <li>the OR "|" operator.
 * <li>the grouping "(...)" operator.
 * <li>the optional grouping "[...]" operator.
 * <li>the zero-or-many "*" (called Kleene star) operator.
 * <li>a probability (e.g., "open" is more likely than the others).
 * </ul>
 * 
 * <h3>From JSGF to Grammar Graph</h3>
 * 
 * After the JSGF grammar is read in, it is converted to a graph of words
 * representing the grammar. Lets call this the grammar graph. It is from this
 * grammar graph that the eventual search structure used for speech recognition
 * is built. Below, we show the grammar graphs created from the above JSGF
 * grammars. The nodes <code>"&lt;sil&gt;"</code> means "silence".
 * 
 * <p/>
 * <img src="doc-files/helloWorld.jpg"> <br>
 * 
 * <i>Figure 3: Grammar graph created from the Hello World grammar. </i>
 * <p/>
 * <img src="doc-files/commandGrammar.jpg"> <br>
 * 
 * <i>Figure 4: Grammar graph created from the Command grammar. </i>
 * 
 * <h3>Limitations</h3>
 * 
 * There is a known limitation with the current JSGF support. Grammars that
 * contain non-speech loops currently cause the recognizer to hang.
 * <p/>
 * For example, in the following grammar
 * 
 * <pre>
 *  #JSGF V1.0
 *  grammar jsgf.nastygram;
 *  public &lt;nasty&gt; = I saw a ((cat* | dog* | mouse*)+)+;
 * </pre>
 * 
 * the production: ((cat* | dog* | mouse*)+)+ can result in a continuous loop,
 * since (cat* | dog* | mouse*) can represent no speech (i.e. zero cats, dogs
 * and mice), this is equivalent to ()+. To avoid this problem, the grammar
 * writer should ensure that there are no rules that could possibly match no
 * speech within a plus operator or kleene star operator.
 * 
 * <h3>Dynamic grammar behavior</h3> It is possible to modify the grammar of a
 * running application. Some rules and notes:
 * <ul>
 * <li>Unlike a JSAPI recognizer, the JSGF Grammar only maintains one Rule
 * Grammar. This restriction may be relaxed in the future.
 * <li>The grammar should not be modified while a recognition is in process
 * <li>The call to JSGFGrammar.loadJSGF will load in a completely new grammar,
 * tossing any old grammars or changes. No call to commitChanges is necessary
 * (although such a call would be harmless in this situation).
 * <li>RuleGrammars can be modified via calls to RuleGrammar.setEnabled and
 * RuleGrammar.setRule). In order for these changes to take place,
 * JSGFGrammar.commitChanges must be called after all grammar changes have been
 * made.
 * </ul>
 * 
 * <h3>Implementation Notes</h3>
 * <ol>
 * <li>All internal probabilities are maintained in LogMath log base.
 * </ol>
 */
public class JSGFGrammar extends Grammar {

    /** The property that defines the location of the JSGF grammar file. */
    @S4String
    public final static String PROP_BASE_GRAMMAR_URL = "grammarLocation";

    /** The property that defines the location of the JSGF grammar file. */
    @S4String(defaultValue = "default.gram")
    public final static String PROP_GRAMMAR_NAME = "grammarName";

    // ---------------------
    // Configurable data
    // ---------------------
    private JSGFRuleGrammar ruleGrammar;
    protected JSGFRuleGrammarManager manager;
    protected RuleStack ruleStack;
    private String grammarName;
    protected URL baseURL;
    private LogMath logMath;

    protected boolean loadGrammar = true;
    protected GrammarNode firstNode;
    protected Logger logger;

    public JSGFGrammar(String location, String grammarName,
            boolean showGrammar, boolean optimizeGrammar,
            boolean addSilenceWords, boolean addFillerWords,
            Dictionary dictionary) throws MalformedURLException,
            ClassNotFoundException {
        this(ConfigurationManagerUtils.resourceToURL(location),
                grammarName, showGrammar, optimizeGrammar, addSilenceWords,
                addFillerWords, dictionary);
    }

    public JSGFGrammar(URL baseURL, String grammarName,
            boolean showGrammar, boolean optimizeGrammar,
            boolean addSilenceWords, boolean addFillerWords,
            Dictionary dictionary) {
        super(showGrammar, optimizeGrammar, addSilenceWords, addFillerWords,
                dictionary);
        logMath = LogMath.getInstance();
        this.baseURL = baseURL;
        this.grammarName = grammarName;
        loadGrammar = true;
        logger = Logger.getLogger(getClass().getName());
    }

    public JSGFGrammar() {

    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util
     * .props.PropertySheet)
     */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);
        baseURL = ConfigurationManagerUtils.getResource(PROP_BASE_GRAMMAR_URL,
                ps);
        logger = ps.getLogger();
        grammarName = ps.getString(PROP_GRAMMAR_NAME);
        loadGrammar = true;
    }

    /**
     * Returns the RuleGrammar of this JSGFGrammar.
     * 
     * @return the RuleGrammar
     */
    public JSGFRuleGrammar getRuleGrammar() {
        return ruleGrammar;
    }

    /**
     * Returns manager used to load grammars
     * 
     * @return manager with loaded grammars
     */
    public JSGFRuleGrammarManager getGrammarManager() {
        if (manager == null)
            manager = new JSGFRuleGrammarManager();
        return manager;
    }

    /**
     * Sets the URL context of the JSGF grammars.
     * 
     * @param url
     *            the URL context of the grammars
     */
    public void setBaseURL(URL url) {
        baseURL = url;
    }

    /** Returns the name of this grammar. */
    public String getGrammarName() {
        return grammarName;
    }

    /**
     * The JSGF grammar specified by grammarName will be loaded from the base
     * url (tossing out any previously loaded grammars)
     * 
     * @param grammarName
     *            the name of the grammar
     * @throws IOException
     *             if an error occurs while loading or compiling the grammar
     * @throws JSGFGrammarException
     * @throws JSGFGrammarParseException
     */
    public void loadJSGF(String grammarName) throws IOException,
            JSGFGrammarParseException, JSGFGrammarException {
        this.grammarName = grammarName;
        loadGrammar = true;
        commitChanges();
    }

    /**
     * Creates the grammar.
     * 
     * @return the initial node of the Grammar
     */
    @Override
    protected GrammarNode createGrammar() throws IOException {
        try {
            commitChanges();
        } catch (JSGFGrammarException e) {
            throw new IOException(e);
        } catch (JSGFGrammarParseException e) {
            throw new IOException(e);
        }
        return firstNode;
    }

    /**
     * Returns the initial node for the grammar
     * 
     * @return the initial grammar node
     */
    @Override
    public GrammarNode getInitialNode() {
        return firstNode;
    }

    /**
     * Parses the given Rule into a network of GrammarNodes.
     * 
     * @param rule
     *            the Rule to parse
     * @return a grammar graph
     */
    protected GrammarGraph processRule(JSGFRule rule) throws JSGFGrammarException {
        GrammarGraph result;

        if (rule != null) {
            logger.fine("parseRule: " + rule);
        }

        if (rule instanceof JSGFRuleAlternatives) {
            result = processRuleAlternatives((JSGFRuleAlternatives) rule);
        } else if (rule instanceof JSGFRuleCount) {
            result = processRuleCount((JSGFRuleCount) rule);
        } else if (rule instanceof JSGFRuleName) {
            result = processRuleName((JSGFRuleName) rule);
        } else if (rule instanceof JSGFRuleSequence) {
            result = processRuleSequence((JSGFRuleSequence) rule);
        } else if (rule instanceof JSGFRuleTag) {
            result = processRuleTag((JSGFRuleTag) rule);
        } else if (rule instanceof JSGFRuleToken) {
            result = processRuleToken((JSGFRuleToken) rule);
        } else {
            throw new IllegalArgumentException("Unsupported Rule type: " + rule);
        }
        return result;
    }

    /**
     * Parses the given RuleName into a network of GrammarNodes.
     * 
     * @param initialRuleName
     *            the RuleName rule to parse
     * @return a grammar graph
     */
    private GrammarGraph processRuleName(JSGFRuleName initialRuleName)
            throws JSGFGrammarException {
        logger.fine("parseRuleName: " + initialRuleName);
        GrammarGraph result = ruleStack.contains(initialRuleName.getRuleName());

        if (result != null) { // its a recursive call
            return result;
        } else {
            result = new GrammarGraph();
            ruleStack.push(initialRuleName.getRuleName(), result);
        }
        JSGFRuleName ruleName = ruleGrammar.resolve(initialRuleName);

        if (ruleName == JSGFRuleName.NULL) {
            result.getStartNode().add(result.getEndNode(), 0.0f);
        } else if (ruleName == JSGFRuleName.VOID) {
            // no connection for void
        } else {
            if (ruleName == null) {
                throw new JSGFGrammarException("Can't resolve "
                        + initialRuleName + " g "
                        + initialRuleName.getFullGrammarName());
            }
            JSGFRuleGrammar rg = manager.retrieveGrammar(ruleName
                    .getFullGrammarName());
            if (rg == null) {
                throw new JSGFGrammarException("Can't resolve grammar name "
                        + ruleName.getFullGrammarName());
            }

            JSGFRule rule = rg.getRule(ruleName.getSimpleRuleName());
            if (rule == null) {
                throw new JSGFGrammarException("Can't resolve rule: "
                        + ruleName.getRuleName());
            }
            GrammarGraph ruleResult = processRule(rule);
            if (result != ruleResult) {
                result.getStartNode().add(ruleResult.getStartNode(), 0.0f);
                ruleResult.getEndNode().add(result.getEndNode(), 0.0f);
            }
        }
        ruleStack.pop();
        return result;
    }

    /**
     * Parses the given RuleCount into a network of GrammarNodes.
     * 
     * @param ruleCount
     *            the RuleCount object to parse
     * @return a grammar graph
     */
    private GrammarGraph processRuleCount(JSGFRuleCount ruleCount)
            throws JSGFGrammarException {
        logger.fine("parseRuleCount: " + ruleCount);
        GrammarGraph result = new GrammarGraph();
        int count = ruleCount.getCount();
        GrammarGraph newNodes = processRule(ruleCount.getRule());

        result.getStartNode().add(newNodes.getStartNode(), 0.0f);
        newNodes.getEndNode().add(result.getEndNode(), 0.0f);

        // if this is optional, add a bypass arc

        if (count == JSGFRuleCount.ZERO_OR_MORE
                || count == JSGFRuleCount.OPTIONAL) {
            result.getStartNode().add(result.getEndNode(), 0.0f);
        }

        // if this can possibly occur more than once, add a loopback

        if (count == JSGFRuleCount.ONCE_OR_MORE
                || count == JSGFRuleCount.ZERO_OR_MORE) {
            newNodes.getEndNode().add(newNodes.getStartNode(), 0.0f);
        }
        return result;
    }

    /**
     * Parses the given RuleAlternatives into a network of GrammarNodes.
     * 
     * @param ruleAlternatives
     *            the RuleAlternatives to parse
     * @return a grammar graph
     */
    private GrammarGraph processRuleAlternatives(
            JSGFRuleAlternatives ruleAlternatives) throws JSGFGrammarException {
        logger.fine("parseRuleAlternatives: " + ruleAlternatives);
        GrammarGraph result = new GrammarGraph();

        List<JSGFRule> rules = ruleAlternatives.getRules();
        List<Float> weights = getNormalizedWeights(ruleAlternatives.getWeights());

        // expand each alternative, and connect them in parallel
        for (int i = 0; i < rules.size(); i++) {
            JSGFRule rule = rules.get(i);
            float weight = 0.0f;
            if (weights != null) {
                weight = weights.get(i);
            }
            logger.fine("Alternative: " + rule);
            GrammarGraph newNodes = processRule(rule);
            result.getStartNode().add(newNodes.getStartNode(), weight);
            newNodes.getEndNode().add(result.getEndNode(), 0.0f);
        }

        return result;
    }

    /**
     * Normalize the weights. The weights should always be zero or greater. We
     * need to convert the weights to a log probability.
     * 
     * @param weights
     *            the weights to normalize
     */
    private List<Float> getNormalizedWeights(List<Float> weights) {
        
        if (weights == null) {
            return null;
        }

        double sum = 0.0;
        for (float weight : weights) {
            if (weight < 0) {
                throw new IllegalArgumentException("Negative weight " + weight);
            }
            sum += weight;
        }

        List<Float> normalized = new LinkedList<Float>(weights);

        for (int i = 0; i < weights.size(); i++) {
            if (sum == 0.0f) {
                normalized.set(i, LogMath.LOG_ZERO);
            } else {
                normalized.set(i, logMath.linearToLog(weights.get(i) / sum));
            }
        }
        return normalized;
    }

    /**
     * Parses the given RuleSequence into a network of GrammarNodes.
     * 
     * @param ruleSequence
     *            the RuleSequence to parse
     * @return the first and last GrammarNodes of the network
     */
    private GrammarGraph processRuleSequence(JSGFRuleSequence ruleSequence)
            throws JSGFGrammarException {

        GrammarNode startNode = null;
        GrammarNode endNode = null;
        logger.fine("parseRuleSequence: " + ruleSequence);

        List<JSGFRule> rules = ruleSequence.getRules();

        GrammarNode lastGrammarNode = null;

        // expand and connect each rule in the sequence serially
        for (int i = 0; i < rules.size(); i++) {
            JSGFRule rule = rules.get(i);
            GrammarGraph newNodes = processRule(rule);

            // first node
            if (i == 0) {
                startNode = newNodes.getStartNode();
            }

            // last node
            if (i == (rules.size() - 1)) {
                endNode = newNodes.getEndNode();
            }

            if (i > 0) {
                lastGrammarNode.add(newNodes.getStartNode(), 0.0f);
            }
            lastGrammarNode = newNodes.getEndNode();
        }

        return new GrammarGraph(startNode, endNode);
    }

    /**
     * Parses the given RuleTag into a network GrammarNodes.
     * 
     * @param ruleTag
     *            the RuleTag to parse
     * @return the first and last GrammarNodes of the network
     */
    private GrammarGraph processRuleTag(JSGFRuleTag ruleTag)
            throws JSGFGrammarException {
        logger.fine("parseRuleTag: " + ruleTag);
        JSGFRule rule = ruleTag.getRule();
        return processRule(rule);
    }

    /**
     * Creates a GrammarNode with the word in the given RuleToken.
     * 
     * @param ruleToken
     *            the RuleToken that contains the word
     * @return a GrammarNode with the word in the given RuleToken
     */
    private GrammarGraph processRuleToken(JSGFRuleToken ruleToken) {

        GrammarNode node = createGrammarNode(ruleToken.getText());
        return new GrammarGraph(node, node);
    }

    // ///////////////////////////////////////////////////////////////////
    // Loading part
    // //////////////////////////////////////////////////////////////////

    private static URL grammarNameToURL(URL baseURL, String grammarName)
            throws MalformedURLException {

        // Convert each period in the grammar name to a slash "/"
        // Append a slash and the converted grammar name to the base URL
        // Append the ".gram" suffix
        grammarName = grammarName.replace('.', '/');
        StringBuilder sb = new StringBuilder();
        if (baseURL != null) {
            sb.append(baseURL);
            if (sb.charAt(sb.length() - 1) != '/')
                sb.append('/');
        }
        sb.append(grammarName).append(".gram");
        String urlstr = sb.toString();

        URL grammarURL = null;
        try {
            grammarURL = new URL(urlstr);
        } catch (MalformedURLException me) {
            grammarURL = ClassLoader.getSystemResource(urlstr);
            if (grammarURL == null)
                throw new MalformedURLException(urlstr);
        }

        return grammarURL;
    }

    /**
     * Commit changes to all loaded grammars and all changes of grammar since
     * the last commitChange
     * 
     * @throws JSGFGrammarParseException
     * @throws JSGFGrammarException
     */
    public void commitChanges() throws IOException, JSGFGrammarParseException,
            JSGFGrammarException {
        try {
            if (loadGrammar) {
                if (manager == null)
                    getGrammarManager();
                ruleGrammar = loadNamedGrammar(grammarName);
                loadImports(ruleGrammar);
                loadGrammar = false;
            }

            manager.linkGrammars();
            ruleStack = new RuleStack();
            newGrammar();

            firstNode = createGrammarNode("<sil>");
            GrammarNode finalNode = createGrammarNode("<sil>");
            finalNode.setFinalNode(true);

            // go through each rule and create a network of GrammarNodes
            // for each of them

            for (String ruleName : ruleGrammar.getRuleNames()) {
                if (ruleGrammar.isRulePublic(ruleName)) {
                    String fullName = getFullRuleName(ruleName);
                    GrammarGraph publicRuleGraph = new GrammarGraph();
                    ruleStack.push(fullName, publicRuleGraph);
                    JSGFRule rule = ruleGrammar.getRule(ruleName);
                    GrammarGraph graph = processRule(rule);
                    ruleStack.pop();

                    firstNode.add(publicRuleGraph.getStartNode(), 0.0f);
                    publicRuleGraph.getEndNode().add(finalNode, 0.0f);
                    publicRuleGraph.getStartNode().add(graph.getStartNode(),
                            0.0f);
                    graph.getEndNode().add(publicRuleGraph.getEndNode(), 0.0f);
                }
            }
            postProcessGrammar();
            if (logger.isLoggable(Level.FINEST)) {
                dumpGrammar();
            }
        } catch (MalformedURLException mue) {
            throw new IOException("bad base grammar URL " + baseURL + ' ' + mue);
        }
    }

    /**
     * Load grammars imported by the specified RuleGrammar if they are not
     * already loaded.
     * 
     * @throws JSGFGrammarParseException
     */
    private void loadImports(JSGFRuleGrammar grammar) throws IOException,
            JSGFGrammarParseException {

        for (JSGFRuleName ruleName : grammar.imports) {
            // System.out.println ("Checking import " + ruleName);
            String grammarName = ruleName.getFullGrammarName();
            JSGFRuleGrammar importedGrammar = getNamedRuleGrammar(grammarName);

            if (importedGrammar == null) {
                // System.out.println ("Grammar " + grammarName +
                // " not found. Loading.");
                importedGrammar = loadNamedGrammar(ruleName
                        .getFullGrammarName());
            }
            if (importedGrammar != null) {
                loadImports(importedGrammar);
            }
        }
        loadFullQualifiedRules(grammar);
    }

    private JSGFRuleGrammar getNamedRuleGrammar(String grammarName) {
        return manager.retrieveGrammar(grammarName);
    }

    /**
     * Load named grammar from import rule
     * 
     * @param grammarName
     * @return already loaded grammar
     * @throws JSGFGrammarParseException
     * @throws IOException
     */
    private JSGFRuleGrammar loadNamedGrammar(String grammarName)
            throws JSGFGrammarParseException, IOException {

        URL url = grammarNameToURL(baseURL, grammarName);
        JSGFRuleGrammar ruleGrammar = JSGFParser.newGrammarFromJSGF(url,
                new JSGFRuleGrammarFactory(manager));
        ruleGrammar.setEnabled(true);

        return ruleGrammar;
    }

    /**
     * Load grammars imported by a fully qualified Rule Token if they are not
     * already loaded.
     * 
     * @param grammar
     * @throws IOException
     * @throws GrammarException
     * @throws JSGFGrammarParseException
     */
    private void loadFullQualifiedRules(JSGFRuleGrammar grammar)
            throws IOException, JSGFGrammarParseException {

        // Go through every rule
        for (String ruleName : grammar.getRuleNames()) {
            String rule = grammar.getRule(ruleName).toString();
            // check for rule-Tokens
            int index = 0;
            while (index < rule.length()) {
                index = rule.indexOf('<', index);
                if (index < 0) {
                    break;
                }
                // Extract rule name
                JSGFRuleName extractedRuleName = new JSGFRuleName(rule
                        .substring(index + 1, rule.indexOf('>', index + 1))
                        .trim());
                index = rule.indexOf('>', index) + 1;

                // Check for full qualified rule name
                if (extractedRuleName.getFullGrammarName() != null) {
                    String grammarName = extractedRuleName.getFullGrammarName();
                    JSGFRuleGrammar importedGrammar = getNamedRuleGrammar(grammarName);
                    if (importedGrammar == null) {
                        importedGrammar = loadNamedGrammar(grammarName);
                    }
                    if (importedGrammar != null) {
                        loadImports(importedGrammar);
                    }
                }
            }
        }
    }

    /**
     * Gets the fully resolved rule name
     * 
     * @param ruleName
     *            the partial name
     * @return the fully resolved name
     * @throws JSGFGrammarException
     */
    private String getFullRuleName(String ruleName) throws JSGFGrammarException {
        JSGFRuleName rname = ruleGrammar.resolve(new JSGFRuleName(ruleName));
        return rname.getRuleName();
    }

    /** Dumps interesting things about this grammar */
    protected void dumpGrammar() {
        System.out.println("Imported rules { ");

        for (JSGFRuleName imp : ruleGrammar.getImports()) {
            System.out.println("  Import " + imp.getRuleName());
        }
        System.out.println("}");

        System.out.println("Rulenames { ");

        for (String name : ruleGrammar.getRuleNames()) {
            System.out.println("  Name " + name);
        }
        System.out.println("}");
    }

    /**
     * Represents a graph of grammar nodes. A grammar graph has a single
     * starting node and a single ending node
     */
    class GrammarGraph {

        private GrammarNode startNode;
        private GrammarNode endNode;

        /**
         * Creates a grammar graph with the given nodes
         * 
         * @param startNode
         *            the staring node of the graph
         * @param endNode
         *            the ending node of the graph
         */
        GrammarGraph(GrammarNode startNode, GrammarNode endNode) {
            this.startNode = startNode;
            this.endNode = endNode;
        }

        /** Creates a graph with non-word nodes for the start and ending nodes */
        GrammarGraph() {
            startNode = createGrammarNode(false);
            endNode = createGrammarNode(false);
        }

        /**
         * Gets the starting node
         * 
         * @return the starting node for the graph
         */
        GrammarNode getStartNode() {
            return startNode;
        }

        /**
         * Gets the ending node
         * 
         * @return the ending node for the graph
         */
        GrammarNode getEndNode() {
            return endNode;
        }
    }

    /** Manages a stack of grammar graphs that can be accessed by grammar name */
    class RuleStack {

        private List<String> stack;
        private HashMap<String, GrammarGraph> map;

        /** Creates a name stack */
        public RuleStack() {
            clear();
        }

        /** Pushes the grammar graph on the stack */
        public void push(String name, GrammarGraph g) {
            stack.add(0, name);
            map.put(name, g);
        }

        /** remove the top graph on the stack */
        public void pop() {
            map.remove(stack.remove(0));
        }

        /**
         * Checks to see if the stack contains a graph with the given name
         * 
         * @param name
         *            the graph name
         * @return the grammar graph associated with the name if found,
         *         otherwise null
         */
        public GrammarGraph contains(String name) {
            if (stack.contains(name)) {
                return map.get(name);
            } else {
                return null;
            }
        }

        /** Clears this name stack */
        public void clear() {
            stack = new LinkedList<String>();
            map = new HashMap<String, GrammarGraph>();
        }
    }
}

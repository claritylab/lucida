package info.ephyra.questionanalysis.atype;

import info.ephyra.nlp.StanfordParser;
import info.ephyra.util.Properties;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.ListIterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import net.didion.jwnl.JWNL;
import net.didion.jwnl.data.IndexWord;
import net.didion.jwnl.data.POS;
import net.didion.jwnl.dictionary.Dictionary;

import org.apache.log4j.Logger;

import edu.cmu.lti.chineseNLP.util.Tree;
import edu.cmu.lti.chineseNLP.util.TreeHelper;
import edu.cmu.lti.javelin.qa.Term;

/**
 * Finder of the focus word, or target word, of a question.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class FocusFinder {
    
    private static Logger log = Logger.getLogger(FocusFinder.class);
    private static boolean isInitialized;
    private static List<Tree> treeTemplates;
    private static String treeTemplatesFile;
    // WH word tags that can have siblings are: WP, WDT, WHADJP, WRB
    private static String[] whLabels = {"WP","WDT","WHADJP"};
    
    /**
     * Initializes static resources. The input properties which must be defined are:
     * <ul>
     *   <li> edu.cmu.lti.javelin.qa.english.FocusFinder.treeTemplatesFile : &nbsp; 
     *   the location of a file containing tree templates to use.  Each tree template
     *   must be specified on one line.  Blank lines and lines beginning with "#" 
     *   are ignored.  A tree template is a parenthesized syntactic parse tree which
     *   can be used as an underspecified template tree to unify with a real syntactic
     *   parse tree.  See {@link edu.cmu.lti.chineseNLP.util.TreeHelper#extractNode 
     *   TreeHelper.extractNode} for more details.
     * </ul>
     * @throws Exception if the required input property is not defined
     */
    public static void initialize() throws Exception {
        // return if already initialized
        if (isInitialized()) return;
        
        Properties properties = Properties.loadFromClassName(FocusFinder.class.getName());
        
        // initialize JWNL
        if (!JWNL.isInitialized()) {
            String file_properties = System.getProperty("jwnl.configuration");
            if (file_properties == null)
                throw new Exception("Required property 'jwnl.configuration' is undefined");
            JWNL.initialize(new FileInputStream(file_properties));
        }
        
        // load tree templates file
        treeTemplatesFile = properties.getProperty("treeTemplatesFile");
        if (treeTemplatesFile == null)
            throw new Exception("Required property treeTemplatesFile is undefined");
        BufferedReader in = new BufferedReader(new FileReader(treeTemplatesFile));
        String line;
        treeTemplates = new ArrayList<Tree>();
        while ((line = in.readLine()) != null) {
            if (line.matches("#.*") || line.matches("\\s*")) continue;
            treeTemplates.add(TreeHelper.buildTree(line, Tree.ENGLISH));
        }
        in.close();
        
        setInitialized(true);
    }
    
    /**
     * @return the isInitialized
     */
    public static boolean isInitialized() {
        return isInitialized;
    }

    /**
     * @param isInitialized the isInitialized to set
     */
    public static void setInitialized(boolean isInitialized) {
        FocusFinder.isInitialized = isInitialized;
    }
    
    /**
     * Finds the focus, or target, word of a question using the specified tree
     * templates and default rules that look for question words.</p>
     * 
     * Using the following algorithm:</p>
     * <ol>
     *   <li> Look for the head word of a phrase that has a wh-word child and 
     *   more than one sibling
     *   <ul>
     *     <li> look for the object of a modifying of-<code>PP</code> if the word
     *      is "kind" or "type"
     *   </ul>
     *   <li> Try to extract a node using the specified tree templates in the order 
     *   they appear in the file
     *   <li> Using only preterminals, look for the last consecutive <code>NN*</code> 
     *   or <code>JJ</code> that follows a <code>WDT</code> or <code>WP</code>
     *   <li> Using only preterminals, look for the last consecutive <code>NN*</code> 
     *   that follows the terminals, "how many".   
     * </ol>
     * 
     * the focus word is returned as soon as one is found.</p>
     * 
     * @param tree The syntactic parse tree of the question
     * @return the focus word, or <code>null</code> if none was found
     */
    public static Tree findFocusNode (Tree tree) {
        Tree focus = null;
        TreeHelper.markHeadNode(tree);
        // look for a WH word with siblings anywhere in the tree
        for(String whLabel : whLabels) {
            Tree whPhrase = TreeHelper.findNodeWithChild(tree, whLabel);
            if (whPhrase != null && whPhrase.numOfChildren() > 1 &&
                    !whPhrase.getLabel().equals("SBAR")) {
                Tree head = getHeadWordOrPhrase(whPhrase);
                TreeHelper.markHeadNode(head);
                String headStr = head.getHeadWord();
                if (headStr.matches("kinds?") || headStr.matches("types?") || headStr.matches("genres?")) {
                    Tree headNode = whPhrase.getHeadNode();
                    Tree parent = TreeHelper.locateParent(headNode, tree);
                    Tree pp = parent.getChild("PP");
                    if (pp == null && parent.getLabel().matches("WHNP|NP")) {
                        Tree grandParent = TreeHelper.locateParent(parent, tree);
                        pp = grandParent.getChild("PP");
                    }
                    if (pp != null && pp.getChild("IN").getHeadWord().equals("of")) {
                        focus = getHeadWordOrPhrase(pp.getChild("NP"));
                        break;
                    }
                } 
                if (head.getLabel().equals("POS")) {
                    if (whPhrase.numOfChildren() > 2)
                        return getHeadWordOrPhrase(whPhrase.getChild(whPhrase.getHeadNodeChildIndex()-1));
                }
                return head;
            }
        }
        if (focus != null) return focus;
        // use syntactic patterns for when the focus word is apart from the WH word.
        for (Tree template : treeTemplates) {
            Tree node = TreeHelper.extractNode(tree,template);
            if (node == null) continue;
            focus = getHeadWordOrPhrase(node);
            if (focus != null) return focus;
        }
        //  look for a JJ or last consecutive NN preceded by WDT
        List<Tree> nodes = TreeHelper.getPreterminalsAfter(tree, "NN.?|JJ", "WDT");
        if (nodes == null) 
            nodes = TreeHelper.getPreterminalsAfter(tree, "NN.?|JJ", "WP");
        if (nodes != null) {
            focus = Tree.newNode("NP",nodes);
            return getHeadWordOrPhrase(focus);
        }

        // look for a NN* preceded by 'how many'
        List<Tree> tags = TreeHelper.getPreterminals(tree).asList();
        nodes = new ArrayList<Tree>(); 
        for (ListIterator<Tree> it = tags.listIterator(); it.hasNext();) {
            Tree tag = it.next();
            if (tag.getHeadWord().toLowerCase().equals("how") &&
                    it.next().getHeadWord().toLowerCase().matches("many|much")) {
                for (ListIterator<Tree> it2 = tags.listIterator(it.nextIndex()); 
                    it2.hasNext() && it2.next().getLabel().matches("NN.?");) {
                    nodes.add(tags.get(it2.previousIndex()));
                }
                break;
            }
        }
        if (nodes.size() > 0) {
            focus = Tree.newNode("NP", nodes);
            return getHeadWordOrPhrase(focus);
        }
        return null;
    }
    
    /**
     * Extracts the head word or phrase from the given Tree node, which is assumed
     * to be an NP.  Whether a phrase should be the head is determined by looking up
     * in WordNet all the possible phrases that can be constructed from the immediate children 
     * of the input Tree and which include the right-most child.
     * 
     * @param tree the Tree node from which to extract the head word or phrase
     */
    public static Tree getHeadWordOrPhrase(Tree tree) {
        TreeHelper.markHeadNode(tree);
        //Tree headChild = tree.getChild(tree.getHeadNodeChildIndex()); // can return null
        Tree headChild = tree.getHeadNode();
        if (!headChild.isPreterminal())
            return getHeadWordOrPhrase(headChild);
        List<Tree> pretermChildren = new ArrayList<Tree>();
        for( Tree child : tree.getChildren() ) {
            if (child.isPreterminal() && !child.getLabel().equals("DT")) 
                pretermChildren.add(child);
        }
        
        for (ListIterator<Tree> it = pretermChildren.listIterator(); it.hasNext();) {
            Tree t = it.next();
            StringBuilder phrase = new StringBuilder();
            List<Tree> nodes = new ArrayList<Tree>();
            nodes.add(t);
            phrase.append(t.getHeadWord() + " ");
            for (ListIterator<Tree> it2 = pretermChildren.listIterator(it.nextIndex());it2.hasNext();) {
                Tree t2 = it2.next();
                phrase.append(t2.getHeadWord() + " ");
                nodes.add(t2);
            }
            String phr = phrase.toString().trim();
            int phrSpaces = 0;
            Matcher m = Pattern.compile(" ").matcher(phr);
            while (m.find()) phrSpaces++;
                
            try {
                IndexWord indexWord = Dictionary.getInstance().lookupIndexWord(POS.NOUN, phr);
                if (indexWord == null) throw new Exception("Failed to get index word");
                int wrdSpaces = 0;
                Matcher m2 = Pattern.compile(" ").matcher(indexWord.getLemma());
                while (m2.find()) wrdSpaces++;
                if (wrdSpaces != phrSpaces) continue;
            } catch (Exception e) {
                continue;
            }
            
            if (nodes.size() == 1)
                return nodes.get(0);
            else return Tree.newNode("NP", nodes);
        }
        return tree.getHeadNode();
    }
    
    /**
     * Finds the focus word, given a Tree.
     * 
     * @param tree The syntactic parse tree object
     * @return the focus word as a String or null, if one does not exist
     */
    public static String findFocusWord (Tree tree) {
        try{
            Tree t = findFocusNode(tree);
            if (t != null) return TreeHelper.getLeaves(t);
            return null;
        }
        catch(Exception e){
            e.printStackTrace();
            return null;
        }
    }
    
    /**
     * Given a list of Terms, builds a parse tree using Charniak's parser, and 
     * then uses the resulting parse tree to find the focus words.
     * 
     * @param terms The list of Terms in the question.
     * @return the focus word as a String or null, if one does not exist
     */
    public static String findFocusWord (List<Term> terms) {
        try{
            String question = "";
            for (Term term : terms) {
                question += term + " ";
            }
            Tree t = findFocusNode(
                    TreeHelper.buildTree(
                            StanfordParser.parse(question), Tree.ENGLISH));
            if (t != null) return TreeHelper.getLeaves(t);
            return null;
        }
        catch(Exception e){
            e.printStackTrace();
            return null;
        }
    }
    
    /**
     * Given a sentence, builds a parse tree using Charniak's parser, and 
     * then uses the resulting parse tree to find the focus words.
     * 
     * @param question the input question
     * @return the focus word as a String or null, if one does not exist
     */
    public static String findFocusWord (String question) {
        try{
            String treeStr = StanfordParser.parse(question);
            log.debug("Parse: " + treeStr);
            Tree t = findFocusNode(
                    TreeHelper.buildTree(treeStr, Tree.ENGLISH));
            if (t != null) {
                log.debug("Focus: " + TreeHelper.getLeaves(t));
                return TreeHelper.getLeaves(t);
            }
            return null;
        }
        catch(Exception e){
            e.printStackTrace();
            return null;
        }
    }
    
    /**
     * Given a list of Terms, builds a parse tree using Charniak's parser, and 
     * then uses the resulting parse tree to find the focus words.
     * 
     * @param terms The list of Terms in the question.
     * @return the focus word as a Term or null, if one does not exist
     */
    public static Term findFocusTerm (List<Term> terms) {
        try{
            String question = "";
            for (Term term : terms) {
                question += term + " ";
            }
            Tree t = findFocusNode(
                    TreeHelper.buildTree(
                            StanfordParser.parse(question), Tree.ENGLISH));
            if (t != null) {
                Term res = new Term(0,0,TreeHelper.getLeaves(t));
                res.setPOS(t.getLabel());
                return res;
            }
            return null;
        }
        catch(Exception e){
            e.printStackTrace();
            return null;
        }
    }
    
    /**
     * Given a sentence, builds a parse tree using Charniak's parser, and 
     * then uses the resulting parse tree to find the focus words.
     * 
     * @param question the input question
     * @return the focus word as a Term or null, if one does not exist
     */
    public static Term findFocusTerm (String question) {
        try{
            Tree t = findFocusNode(
                    TreeHelper.buildTree(
                            StanfordParser.parse(question), Tree.ENGLISH));
            if (t != null) {
                Term res = new Term(0,0,TreeHelper.getLeaves(t));
                res.setPOS(t.getLabel());
                return res;
            }
            return null;
        }
        catch(Exception e){
            e.printStackTrace();
            return null;
        }
    }
    
    /**
     * Finds the focus word, given a Tree.
     * 
     * @param tree The syntactic parse tree object
     * @return the focus word as a Term or null, if one does not exist
     */
    public static Term findFocusTerm (Tree tree) {
        try{
            Tree t = findFocusNode(tree);
            if (t != null) {
                Term res = new Term(0,0,TreeHelper.getLeaves(t));
                res.setPOS(t.getLabel());
                return res;
            }
            return null;
        }
        catch(Exception e){
            e.printStackTrace();
            return null;
        }
    }
    
    /**
     * Extracts and prints out the focus word for each question, given a file
     * of questions.  
     * 
     * @param args command-line args: "&lt;propertiesFile&gt; &lt;inputQuestionsFile&gt;"
     */
    public static void main(String[] args) throws Exception {
       if (args.length != 1) {
           System.out.println("USAGE: FocusFinder <inputQuestionsFile>");
           System.out.println("Output stored in: <inputQuestionsFile>.output");
           System.exit(0);
       }
       FocusFinder.initialize();
       StanfordParser.initialize();
           
       List<String> questions = new ArrayList<String> ();
       BufferedReader in = new BufferedReader(new FileReader(args[0]));
       BufferedWriter out = new BufferedWriter(new FileWriter(args[0]+".output"));
       String question;
       while ((question = in.readLine()) != null) {
           questions.add(question);
       }
       
       for (String q : questions) {
           Tree t = TreeHelper.buildTree(
                           StanfordParser.parse(q), Tree.ENGLISH);
           TreeHelper.markHeadNode(t);
           String focus = findFocusWord(t);
           if (focus == null) focus = "-";
           out.append("FOCUS." + focus + "  " + q + "\n");
       }
       out.close();
       in.close();
    }

}

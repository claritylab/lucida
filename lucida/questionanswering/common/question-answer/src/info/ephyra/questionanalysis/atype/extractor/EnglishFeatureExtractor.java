package info.ephyra.questionanalysis.atype.extractor;

import info.ephyra.nlp.StanfordParser;
import info.ephyra.questionanalysis.atype.FocusFinder;
import info.ephyra.questionanalysis.atype.WordNetAnswerTypeMapping;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import net.didion.jwnl.data.IndexWord;
import net.didion.jwnl.data.POS;
import net.didion.jwnl.dictionary.Dictionary;

import org.apache.log4j.Logger;

import edu.cmu.lti.chineseNLP.util.Tree;
import edu.cmu.lti.chineseNLP.util.TreeHelper;
import edu.cmu.lti.javelin.qa.Term;
import edu.cmu.minorthird.classify.Feature;
import edu.cmu.minorthird.classify.Instance;
import edu.cmu.minorthird.classify.MutableInstance;

/**
 * Feature extractor for English answer type classification.
 * <p> See {@link info.ephyra.questionanalysis.atype.extractor.EnglishFeatureExtractor#initialize initialize} 
 * for a description of the input properties required by this class.
 * <p> See {@link info.ephyra.questionanalysis.atype.extractor.EnglishFeatureExtractor#createInstance createInstance} 
 * for a description of the features extracted by this class.
 * <p> See {@link info.ephyra.questionanalysis.atype.extractor.EnglishFeatureExtractor#loadFile loadFile} 
 * for a specification of the input file format.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class EnglishFeatureExtractor extends FeatureExtractor{
    
    private static final Logger log = Logger.getLogger(EnglishFeatureExtractor.class);
 
    private static String HOW_MUCH_PTRN = "([hH]ow much)";
    private static String HOW_MUCH_OF_PTRN = "([hH]ow much of)";
    private static String HOW_MANY_PTRN = "([hH]ow many)";
    private static String WHOSE_PTRN = "([wW]hose)";
    private static String WHO_PTRN = "([wW]ho)";
    private static String WHOM_PTRN = "([wW]hom)";
    private static String WHAT_PTRN = "([wW]hat)";
    private static String WHEN_PTRN = "([wW]hen)";
    private static String WHERE_PTRN = "([wW]here)";
    private static String WHY_PTRN = "([wW]y)";
    private static String HOW_PTRN = "([hH]ow)";
    private static String WHICH_PTRN = "([wW]hich)";
    private static String WHICH_ANYWHERE_PTRN = ".*(which)";
    private static String WHAT_ANYWHERE_PTRN = ".*(what)";
    
    private static String REST_PTRN = "\\b.*";
    private static String SPACE_PTRN = "\\s+";
    
    private static String[] OF_HEAD_WORDS = {"type", "kind", "genre"};
    
    private static List<String> whPtrns = new ArrayList<String>();
    static {
        whPtrns.add(HOW_MANY_PTRN);
        whPtrns.add(HOW_MUCH_PTRN);
        whPtrns.add(HOW_MUCH_OF_PTRN);
        whPtrns.add(HOW_PTRN);
        whPtrns.add(WHO_PTRN);
        whPtrns.add(WHOM_PTRN);
        whPtrns.add(WHOSE_PTRN);
        whPtrns.add(WHAT_PTRN);
        whPtrns.add(WHEN_PTRN);
        whPtrns.add(WHERE_PTRN);
        whPtrns.add(WHY_PTRN);
        whPtrns.add(WHICH_PTRN);
        whPtrns.add(WHICH_ANYWHERE_PTRN);
        whPtrns.add(WHAT_ANYWHERE_PTRN);
    }
    
    /**
     * Initializes static resources.  
     * @throws Exception if one of the required properties is not defined.
     */
    public void initialize() throws Exception
    {
        // return if already initialized
        if (isInitialized()) return;
        super.initialize();
        
        FocusFinder.initialize();
        WordNetAnswerTypeMapping.initialize();
        StanfordParser.initialize();
        
        setInitialized(true);
    }
    
    private static void addWordLevelFeatures(MutableInstance instance, List<Term> terms, Term focus) {
        String[] words = new String[terms.size()];
        for (int i = 0; i < terms.size(); i++) {
            Term term = terms.get(i);
            if (term.getText() != null)
                words[i] = term.getText().replaceAll("\\s+","_");
            else words[i] = "-";
        }
        // UNIGRAM
        for(int i=0; i < words.length; i++){
            instance.addBinary(new Feature("UNIGRAM"+"."+words[i]));
        }
        // BIGRAM
        for(int i=0; i < words.length-1; i++){
            instance.addBinary(new Feature("BIGRAM"+"."+words[i]+"-"+words[i+1]));
        }
        // WH_WORD
        String question = "";
        for (Term term : terms) question += term.getText()+" ";
        question = question.trim();
        String whWord = null;
        // first look at sentence beginning
        for(String ptrn : whPtrns) {
            Matcher m = Pattern.compile("^"+ptrn+REST_PTRN).matcher(question);
            if (m.matches()) {
                whWord = m.group(1).toLowerCase().replaceAll("\\s+", "_");
                instance.addBinary(new Feature("WH_WORD"+"."+whWord));
                break;
            }
        }
        if (whWord == null) {
            // then look anywhere in the sentence
            for(String ptrn : whPtrns) {
                Matcher m = Pattern.compile(ptrn+REST_PTRN).matcher(question);
                if (m.find()) {
                    whWord = m.group(1).toLowerCase().replaceAll("\\s+", "_");
                    instance.addBinary(new Feature("WH_WORD"+"."+whWord));
                    break;
                }
            }
        }
        
        // OF_HEAD
        if (focus == null) return;
        for (String word : OF_HEAD_WORDS) {
            Matcher m = Pattern.compile(word+"s? of "+focus.getText()).matcher(question);
            if (m.find()) {
                instance.addBinary(new Feature("OF_HEAD"+"."+word));
                break;
            }
        }
    }
    
    private static void addSyntacticFeatures(MutableInstance instance, List<Term> terms, String parseTree, Term focusTerm) {
        if (parseTree == null) {
            log.error("Syntactic parse of the question is null.");
            return;
        }
        Tree tree = TreeHelper.buildTree(parseTree, Tree.ENGLISH);
        
        // MAIN_VERB
        TreeHelper.markHeadNode(tree);
        String mainVerb = tree.getHeadWord();
        //mainVerb = WordnetInterface.getLemma("VERB",mainVerb);
        try {
            IndexWord word = Dictionary.getInstance().lookupIndexWord(POS.VERB, mainVerb);
            String lemma = null;
            if (word != null) lemma = word.getLemma();
            if (lemma != null) mainVerb = lemma;
        } catch (Exception e) {
            log.warn("Failed to get lemma for verb '"+mainVerb+"'", e);
        }
        if (mainVerb == null) mainVerb = "-";
        instance.addBinary(new Feature("MAIN_VERB"+"."+mainVerb));

        // WH_DET
        if (focusTerm != null && focusTerm.getText() != null) {
            String focus = focusTerm.getText();
            String question = "";
            for (Term term : terms) question += term.getText()+" ";
            question = question.trim();
            for(String ptrn : whPtrns) {
                Matcher m = Pattern.compile(ptrn+SPACE_PTRN+focus+REST_PTRN).matcher(question);
                if (m.matches()) {
                    instance.addBinary(new Feature("WH_DET"+".+"));
                    break;
                }
            }
        }
        
        // FOCUS_ADJ
        Tree focusNode = TreeHelper.findFirstPreterminalWithPrecedingPreterminal(tree, "RB|JJ", "WRB");
        if (focusNode != null) 
            instance.addBinary(new Feature("FOCUS_ADJ"+"."+focusNode.getHeadWord()));
        
    }
    
    private static void addSemanticFeatures(MutableInstance instance, Term focusTerm) {
        // FOCUS_TYPE
        String focusType = WordNetAnswerTypeMapping.getAnswerType(focusTerm);
        if (focusType == null) 
            focusType = "-";
        instance.addBinary(new Feature("FOCUS_TYPE"+"."+focusType));
        return;   
    }

    /**
     * Creates and populates an Instance from a QuestionAnalysis object.  All
     * features are binary features of one of the following types:</p>
     * 
     * Word-level features:
     * <ul>
     *   <li>UNIGRAM : individual words in the question
     *   <li>BIGRAM : pairs of adjacent words in the question
     *   <li>WH_WORD : the wh-word in the question if one exists
     * </ul>
     * 
     * Syntactic features:
     * <ul>
     *   <li>MAIN_VERB: the syntactic head of the sentence, as defined in 
     *   {@link edu.cmu.lti.chineseNLP.util.TreeHelper TreeHelper}
     *   <li>FOCUS_ADJ : the adjective following a wh-word (e.g. 'long' in 'How long is it?') 
     *   <li>WH_DET : whether or not the wh-word is the determiner of a noun phrase, as in 'which printer'
     * </ul>
     * 
     * Semantic features:
     * <ul>
     *   <li>FOCUS_TYPE : the semantic type of the focus word, 
     * </ul>
     * 
     * @throws Exception
     */
    public Instance createInstance(List<Term> terms, String parseTree){
        String question = "";
        for (Term term : terms) question += term + " ";
        question = question.trim();
        
        MutableInstance instance = new MutableInstance(question);

        // find the focus word
        log.debug("Parse: " + parseTree);
        Tree tree = TreeHelper.buildTree(parseTree, Tree.ENGLISH);
        Term focus = FocusFinder.findFocusTerm(tree);
        if (focus != null) log.debug("Focus: " + focus.getText());
        
        addWordLevelFeatures(instance, terms, focus);
        addSyntacticFeatures(instance, terms, parseTree, focus);
        addSemanticFeatures(instance, focus);
        return instance;
    }
    
    public Instance createInstance(String question){
        String[] tokens = question.split("\\s+");
        List<String> words = new ArrayList<String>();
        for (String token : tokens) words.add(token);
        try {
            String parse = StanfordParser.parse(question);
            return createInstance(question,parse);
        } catch (Exception e) {
            log.error("Failed to parse question, using only word-level features.",e);
            List<Term> terms = new ArrayList<Term>();
            for (String word : words) terms.add(new Term(0,0,word));

            MutableInstance instance = new MutableInstance(question);
            addWordLevelFeatures(instance, terms, null);
            return instance;
        }
    }
}

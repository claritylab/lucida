package info.ephyra.nlp;

import info.ephyra.util.Properties;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.log4j.Logger;

import edu.cmu.lti.javelin.util.DeltaRangeMap;
import edu.cmu.lti.javelin.util.RangeMap;
import edu.stanford.nlp.ling.MapLabel;
import edu.stanford.nlp.ling.Sentence;
import edu.stanford.nlp.ling.Word;
import edu.stanford.nlp.parser.lexparser.LexicalizedParser;
import edu.stanford.nlp.process.Tokenizer;
import edu.stanford.nlp.trees.PennTreebankLanguagePack;
import edu.stanford.nlp.trees.Tree;
import edu.stanford.nlp.trees.TreebankLanguagePack;

/**
 * Wrapper for the Stanford parser.
 * 
 * @author Justin Betteridge, Nico Schlaefer
 * @version 2007-10-30
 */
public class StanfordParser
{
    protected static final Logger log = Logger.getLogger(StanfordParser.class);
    protected static final Pattern whitespace_pattern = Pattern.compile("\\s+");
    protected static final Pattern escaped_char_pattern = Pattern.compile("\\\\/");
    protected static final Pattern double_quote_lable_pattern = Pattern.compile("[`'][`']");
    protected static final Pattern bracket_label_pattern = Pattern.compile("-...-");

    public static final String BEGIN_KEY = "begin";
    public static final String END_KEY = "end";
    
    protected static class MutableInteger {
        public int value;
        public MutableInteger() { value = 0; }
        public MutableInteger(int i) { value = i; }
        public String toString() { return Integer.toString(value); }
        public int getValue() { return value; }
        public void setValue(int i) { value = i; }
    }

    protected static TreebankLanguagePack tlp = null;
    protected static LexicalizedParser parser = null;

    /**
     * Hide default ctor.
     */
    protected StanfordParser() {}

    /**
     * Initializes static resources.
     * 
     * @throws Exception
     */
    public static void initialize() throws Exception
    {
        if (parser != null) return;
        Properties properties = Properties.loadFromClassName(StanfordParser.class.getName());
        tlp = new PennTreebankLanguagePack();
        String modelFile = properties.getProperty("modelFile");
        if (modelFile == null)
            throw new Exception("Required property '" 
                + "modelFile' is undefined");
        parser = new LexicalizedParser(modelFile);
    }

    /**
     * Unloads static resources.
     * 
     * @throws Exception
     */
    public static void destroy() throws Exception
    {
        tlp = null;
        parser = null;
    }
    
    /**
     * Parses a sentence and returns a string representation of the parse tree.
     * 
     * @param sentence a sentence
     * @return Tree whose Label is a MapLabel containing correct begin and end
     * character offsets in keys BEGIN_KEY and END_KEY
     */
	@SuppressWarnings("unchecked")
    public static String parse(String sentence)
    {
        if (tlp == null || parser == null)
            throw new RuntimeException("Parser has not been initialized");
        
        // parse the sentence to produce stanford Tree
        log.debug("Parsing sentence");
        Tree tree = null;
        synchronized (parser) {
            Tokenizer tokenizer = tlp.getTokenizerFactory().getTokenizer(new StringReader(sentence));
            List<Word> words = tokenizer.tokenize();
            log.debug("Tokenization: "+words);
            parser.parse(new Sentence(words));
            tree = parser.getBestParse();
        }
        
        // label tree with character extents
        //log.debug("Setting character extents");
        //updateTreeLabels(tree, tree, new MutableInteger(), new MutableInteger(-1));
        //log.debug("Creating offset mapping");
        //List<RangeMap> mapping = createMapping(sentence);
        //log.debug(mapping.toString());
        //log.debug("Applying offset mapping");
        //mapOffsets(tree, mapping);
        
        return tree.toString().replaceAll(" \\[[\\S]+\\]","");
    }
	
	/**
	 * Parses a sentence and returns the PCFG score as a confidence measure.
	 * 
	 * @param sentence a sentence
	 * @return PCFG score
	 */
	@SuppressWarnings("unchecked")
	public static double getPCFGScore(String sentence) {
        if (tlp == null || parser == null)
            throw new RuntimeException("Parser has not been initialized");
        
        // parse the sentence to produce PCFG score
        log.debug("Parsing sentence");
        double score;
        synchronized (parser) {
            Tokenizer tokenizer = tlp.getTokenizerFactory().getTokenizer(new StringReader(sentence));
            List<Word> words = tokenizer.tokenize();
            log.debug("Tokenization: "+words);
            parser.parse(new Sentence(words));
            score = parser.getPCFGScore();
        }
        
        return score;
	}
    
    protected static void updateTreeLabels(Tree root, Tree tree, MutableInteger offset, MutableInteger leafIndex)
    {
        if (tree.isLeaf()) {
            leafIndex.value++;
            return;
        }
        String labelValue = tree.label().value().toUpperCase();
        int begin = root.leftCharEdge(tree);
        int end = root.rightCharEdge(tree);
        //System.out.println(labelValue+"("+begin+","+end+")");
        int length = end - begin;
        
        // apply offset to begin extent
        begin += offset.value;
        
        // calculate offset delta based on label
        if (double_quote_lable_pattern.matcher(labelValue).matches() && length > 1) {
            offset.value--;
            log.debug("Quotes label pattern fired: "+offset);
        } else if (bracket_label_pattern.matcher(labelValue).matches()) {
            offset.value -= 4;
            log.debug("Bracket label pattern fired: "+offset);
        } else if (tree.isPreTerminal()) {
            Tree leaf = tree.firstChild();
            String text = leaf.label().value();
            Matcher matcher = escaped_char_pattern.matcher(text);
            while (matcher.find()) {
                offset.value--;
            }
        }
        
        for (Tree child : tree.children())
            updateTreeLabels(root, child, offset, leafIndex);

        // apply offset to end extent
        end += offset.value;

        // set begin and end offsets on node
        MapLabel label = new MapLabel(tree.label());
        label.put(BEGIN_KEY, begin);
        label.put(END_KEY, end);
        label.put(MapLabel.INDEX_KEY, leafIndex.value);
        tree.setLabel(label);
    }

    /**
     * @param sentence
     * @return a list of RangeMap objects which define a mapping of character
     * offsets in a white-space depleted version of the input string back into
     * offsets in the input string.
     */
    protected static List<RangeMap> createMapping(String sentence)
    {
        List<RangeMap> mapping = new LinkedList<RangeMap>();
        Matcher whitespace_matcher = whitespace_pattern.matcher(sentence);
        DeltaRangeMap delta_rmap = null;

        // find all sequences of whitespace chars
        while (whitespace_matcher.find()) {
            int start = whitespace_matcher.start();
            int end = whitespace_matcher.end();
            int length = end - start;

            if (delta_rmap == null) {
                // create a new RangeMap object whose start begins at current
                // match start, and whose end is at the moment undefined. The
                // delta here is taken to be the length of the whitespace
                // sequence.
                delta_rmap = new DeltaRangeMap(start, 0, length);
            } else {
                // we've found the next sequence of whitespace chars, so we
                // finalize the end extent of the previous RangeMap, and make a
                // new RangeMap to describe the mapping from this point forward.
                delta_rmap.end = start - delta_rmap.delta;
                mapping.add(delta_rmap);
                delta_rmap = new DeltaRangeMap(delta_rmap.end, 0, delta_rmap.delta + length);
            }
        }

        // process trailing DeltaRangeMap if it exists
        if (delta_rmap != null) {
            delta_rmap.end = sentence.length() - delta_rmap.delta;
            mapping.add(delta_rmap);
        }

        return mapping;
    }

    /**
     * Maps Tree node offsets using provided mapping.
     * @param tree the Tree whose begin and end extents should be mapped.
     * @param mapping the list of RangeMap objects which defines the mapping.
     */
    protected static void mapOffsets(Tree tree, List<RangeMap> mapping)
    {
        // if mapping is empty, then assume 1-to-1 mapping.
        if (mapping == null || mapping.size() == 0) return;

        int begin_map_index = 0;
        RangeMap begin_rmap = mapping.get(begin_map_index);
        TREE: for (Tree t : tree) {
            if (t.isLeaf()) continue;
            MapLabel label = (MapLabel) t.label();
            int begin = (Integer) label.get(BEGIN_KEY);
            // "end" must be index of last char in range            
            int end = (Integer) label.get(END_KEY) - 1;

            // find the first rangemap whose end is greater than the
            // beginning of current annotation.
            // log.debug("Finding RangeMap whose extents include
            // annotation.begin");
            while (begin_rmap.end <= begin) {
                begin_map_index++;
                if (begin_map_index >= mapping.size()) break TREE;
                begin_rmap = mapping.get(begin_map_index);
            }

            // if beginning of current rangemap is greater than end of
            // current annotation, then skip this annotation (default
            // mapping is 1-to-1).
            if (begin_rmap.begin > end) {
                // log.debug("Skipping annotation (assuming 1-to-1 offset
                // mapping)");
                continue;
            }

            // if beginning of current annotation falls within current range
            // map, then map it back to source space.
            int new_begin = begin;
            if (begin_rmap.begin <= new_begin) {
                // log.debug("Applying RangeMap to begin offset");
                new_begin = begin_rmap.map(new_begin);
            }

            // find the first rangemap whose end is greater than the end of
            // current annotation.
            // log.debug("Finding RangeMap whose extents include
            // annotation.end");
            int end_map_index = begin_map_index;
            RangeMap end_rmap = begin_rmap;
            END_OFFSET: while (end_rmap.end <= end) {
                end_map_index++;
                if (end_map_index >= mapping.size()) break END_OFFSET;
                end_rmap = mapping.get(end_map_index);
            }

            // if end of current annotation falls within "end" range map,
            // then map it back to source space.
            int new_end = end;
            if (end_rmap.begin <= end) {
                // log.debug("Applying RangeMap to end offset");
                new_end = end_rmap.map(end);
            }

            label.put(BEGIN_KEY, new_begin);
            label.put(END_KEY, new_end + 1);
        }
    }
    
//  private static void printOffsets(String sentence, Tree tree)
//  {
//      if (tree.isLeaf()) return;
//      MapLabel label = (MapLabel) tree.label();
//      int begin = (Integer) label.get(BEGIN_KEY);
//      int end = (Integer) label.get(END_KEY);
//      int index = (Integer) label.index();
//      String str = null;
//      if (begin < 0 || begin > sentence.length() || end < begin || end > sentence.length()) {
//          str = "error";
//      } else {
//          str = sentence.substring(begin, end);
//      }
//      System.out.println(label.value()+"("+index+":"+begin+","+end+"): "+str);
//      for (Tree child : tree.children())
//          printOffsets(sentence, child);
//  }
    
    public static void main(String[] args) throws Exception
    {
        if (args.length != 1) {
            System.out.println("USAGE: StanfordParser <inputSentencesFile>");
            System.out.println("Output stored in: <inputSentencesFile>.parses");
            System.exit(0);
        }
        StanfordParser.initialize();
        List<String> sentences = new ArrayList<String> ();
        BufferedReader in = new BufferedReader(new FileReader(args[0]));
        BufferedWriter out = new BufferedWriter(new FileWriter(args[0]+".parses"));
        String sentence;
        while ((sentence = in.readLine()) != null) {
            sentences.add(sentence);
        }
        for (String s : sentences) {
            out.append(StanfordParser.parse(s)+"\n");
        }
        out.close();
        in.close();
    }
}

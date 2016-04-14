package info.ephyra.questionanalysis.atype;

import info.ephyra.util.Properties;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import net.didion.jwnl.JWNL;
import net.didion.jwnl.data.IndexWord;
import net.didion.jwnl.data.POS;
import net.didion.jwnl.data.PointerUtils;
import net.didion.jwnl.data.Synset;
import net.didion.jwnl.data.Word;
import net.didion.jwnl.data.list.PointerTargetNode;
import net.didion.jwnl.data.list.PointerTargetNodeList;
import net.didion.jwnl.dictionary.Dictionary;

import org.apache.log4j.Logger;

import edu.cmu.lti.javelin.qa.Term;

/**
 * Uses WordNet to map question focus terms to answer types.
 * 
 * @author Justin Betteridge
 * @version 2008-02-10
 */
public class WordNetAnswerTypeMapping {
    
    private static final Logger log = Logger.getLogger(WordNetAnswerTypeMapping.class);
    
    private static Map<String, String> wnAtypeMap = null;
    private static List<String> wnAtypeMapKeys = null;
    private static PointerUtils pUtils;
    private static boolean initialized = false;
    
    private static Comparator<AnswerType> atypeComparator = new Comparator<AnswerType>(){
        public int compare(AnswerType o1,AnswerType o2){
            double val = o2.getConfidence() - o1.getConfidence();
            if(val == 0)
                return 0;
            if(val > 0)
                return 1;
            else
                return -1;
        }
    };
    
    /**
     * Initializes static resources.  The input properties that must be defined are:
     * <ul>
     *   <li>jwnl.configuration : </p>&nbsp; the location of the configuration file for JWNL
     *   <li>edu.cmu.lti.javelin.qa.english.WordNetAnswerTypeMapping.mapFile :
     *   <p>&nbsp; the location of the file specifying a mapping from WordNet synsets
     *   to answer subtypes.  The one-to-many mapping must be specified  
     *   one element per line, with the domain and range values separated by a comma. 
     *   Blank lines and lines beginning with "#" are ignored.  WordNet synsets must be
     *   represented by concatenating the list of lemmas in the synset, separating them 
     *   with a dash ("-"), followed by another "-" and the database file offset of the synset.
     *   (Note: this offset value will vary with the version of WordNet used.)</p>
     *   &nbsp; Thus, an example of an element of the mapping is:</p>
     *     <code>body_of_water-water-8651117,ocean</code>
     * </ul>
     * @throws Exception if one of the required properties is not defined.
     */
    public static void initialize() throws Exception {
        if (isInitialized()) return;
        
        if (!JWNL.isInitialized()) {
            String file_properties = System.getProperty("jwnl.configuration");
            if (file_properties == null)
                throw new Exception("Required property 'jwnl.configuration' is undefined");
            JWNL.initialize(new FileInputStream(file_properties));
        }
        pUtils = PointerUtils.getInstance();
        
        Properties properties = Properties.loadFromClassName(WordNetAnswerTypeMapping.class.getName());
        
        String wnAtypeMapFile = properties.getProperty("mapFile");
        if (wnAtypeMapFile == null)
            throw new RuntimeException("Required parameter mapFile is undefined");

        BufferedReader in = new BufferedReader(new FileReader(wnAtypeMapFile));
        String line;
        wnAtypeMap = new HashMap<String, String>();
        wnAtypeMapKeys = new ArrayList<String>();
        while ((line = in.readLine()) != null) {
            if (line.matches("#.*") || line.matches("\\s*")) continue;
            String[] strs = line.split(",");
            wnAtypeMap.put(strs[0],strs[1]);
            wnAtypeMapKeys.add(strs[0]);
        }
        in.close();
        setInitialized(true);
    }
    
    public static boolean isInitialized() {
        return initialized;
    }
    
    public static void setInitialized(boolean init) {
        initialized = init;
    }
    
    public static String getAnswerType(Term focusTerm){
        if (focusTerm == null) {
            return null;
        }
        String focusText = focusTerm.getText();
        List<AnswerType> focusTypes = new ArrayList<AnswerType>();

        try {
            IndexWord indexWord = Dictionary.getInstance().lookupIndexWord(POS.NOUN, focusText);
            if (indexWord == null) throw new Exception("Failed to get index word");
            Synset[] senses = indexWord.getSenses();
            if (senses == null) throw new Exception("Failed to get synsets");
            
            for (Synset sense : senses) {
                AnswerType type = findWnMapMatch(sense,0);
                if (type != null) {
                    focusTypes.add(type);
                }
            }
        } catch (Exception e) {
            log.warn("Failed to get hypernyms for noun '"+focusText+"'");
        }
        
        if (focusTypes.size() == 0) return focusText.toLowerCase().replaceAll(" ","_");
        Collections.sort(focusTypes,atypeComparator);
        return focusTypes.get(0).getType();            
    }
    
    
    private static AnswerType findWnMapMatch(Synset synset,int level) throws Exception {
        AnswerType type = null;
        String synsetId = buildSynsetString(synset);
        String typeStr = wnAtypeMap.get(synsetId);
        if (typeStr != null) {
            type = AnswerType.constructFromString(typeStr);
            type.setConfidence( 1.0 - ((double)level / 100.0));
            return type;
        }
        PointerTargetNodeList ptNodeList = null;
        ptNodeList = pUtils.getDirectHypernyms(synset);
        for (int i = 0; i < ptNodeList.size(); i++) {
            Synset parent = (Synset)((PointerTargetNode)ptNodeList.get(i)).getPointerTarget();
            type = findWnMapMatch(parent,level+1);
            if (type != null) return type;
        }
        return type;
    }
   
    private static String buildSynsetString(Synset node) {
        StringBuilder sb = new StringBuilder();
        for (Word wd : node.getWords()) {
            sb.append(wd.getLemma().replaceAll("\\s", "_") + "-");
        }
        sb.append(node.getKey());
        return sb.toString();
    }
    
}

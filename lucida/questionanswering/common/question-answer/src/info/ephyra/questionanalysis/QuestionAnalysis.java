package info.ephyra.questionanalysis;

import info.ephyra.io.Logger;
import info.ephyra.io.MsgPrinter;
import info.ephyra.nlp.indices.WordFrequencies;
import info.ephyra.nlp.semantics.Predicate;
import info.ephyra.nlp.semantics.ontologies.Ontology;
import info.ephyra.questionanalysis.atype.AnswerType;
import info.ephyra.questionanalysis.atype.FocusFinder;
import info.ephyra.questionanalysis.atype.QuestionClassifier;
import info.ephyra.questionanalysis.atype.QuestionClassifierFactory;
import info.ephyra.util.Dictionary;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import edu.cmu.lti.javelin.util.Language;
import edu.cmu.lti.util.Pair;

/**
 * Analyzes a question string:
 * <ul>
 * <li>normalizes the question</li>
 * <li>stems verbs and nouns</li>
 * <li>resolves verb constructions with auxiliaries</li>
 * <li>extracts keywords</li>
 * <li>extracts named entities</li>
 * <li>extracts and expands terms</li>
 * <li>determines focus word and expected answer types</li>
 * <li>interprets the question using question patterns</li>
 * <li>extracts predicate-argument structures</li>
 * </ul>
 * 
 * @author Nico Schlaefer
 * @version 2008-01-23
 */
public class QuestionAnalysis {
	/** <code>Dictionaries</code> for term extraction. */
	private static ArrayList<Dictionary> dicts = new ArrayList<Dictionary>();
	/** <code>Ontologies</code> for term expansion. */
	private static ArrayList<Ontology> ontologies = new ArrayList<Ontology>();
	/** <code>Question Classifier</code> for determining the answer type. */
	private static QuestionClassifier qc;
    static {
        try {
            qc = QuestionClassifierFactory.getInstance(
            new Pair<Language,Language>(Language.valueOf("en_US"),Language.valueOf("en_US")));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
	/** String providing additional contextual information on the question. */
	private static String context = "";
	/** Predicates used instead of extracting predicates from the question. */
	private static Predicate[] predicates;
	
	/**
	 * Registers a <code>Dictionary</code>.
	 * 
	 * @param dict a dictionary
	 */
	public static void addDictionary(Dictionary dict) {
		dicts.add(dict);
	}
	
	/**
	 * Registers an <code>Ontology</code>.
	 * 
	 * @param ontology an ontology
	 */
	public static void addOntology(Ontology ontology) {
		ontologies.add(ontology);
	}
	
	/**
	 * Returns the <code>Dictionaries</code>.
	 * 
	 * @return dictionaries
	 */
	public static Dictionary[] getDictionaries() {
		return dicts.toArray(new Dictionary[dicts.size()]);
	}
	
	/**
	 * Returns the <code>Ontologies</code>.
	 * 
	 * @return ontologies
	 */
	public static Ontology[] getOntologies() {
		return ontologies.toArray(new Ontology[ontologies.size()]);
	}
	
	/**
	 * Unregisters all <code>Dictionaries</code>.
	 */
	public static void clearDictionaries() {
		dicts.clear();
	}
	
	/**
	 * Unregisters all <code>Ontologies</code>.
	 */
	public static void clearOntologies() {
		ontologies.clear();
	}
	
	/**
	 * Sets the context string.
	 * 
	 * @param context context string
	 */
	public static void setContext(String context) {
		QuestionAnalysis.context = context;
	}
	
	/**
	 * Returns the context string.
	 * 
	 * @return context string
	 */
	public static String getContext() {
		return context;
	}
	
	/**
	 * Clears the context string.
	 */
	public static void clearContext() {
		context = "";
	}
	
	/**
	 * Sets predicates that are used instead of extracting predicates from the
	 * question.
	 * 
	 * @param predicates the predicates
	 */
	public static void setPredicates(Predicate[] predicates) {
		QuestionAnalysis.predicates = predicates;
	}
	
	/**
	 * Returns the predicates.
	 * 
	 * @return the predicates
	 */
	public static Predicate[] getPredicates() {
		return predicates;
	}
	
	/**
	 * Clears the predicates.
	 */
	public static void clearPredicates() {
		predicates = null;
	}
	
    private static String[] getAtypes (String question) {
        List<AnswerType> atypes = new ArrayList<AnswerType>();
        try {
            atypes = qc.getAnswerTypes(question);
        } catch (Exception e) {
            e.printStackTrace();
        }
        Set<AnswerType> remove = new HashSet<AnswerType>();
        for (AnswerType atype : atypes) {
            if (atype.getFullType(-1).equals("NONE")) {
                remove.add(atype);
            }
        }
        for (AnswerType atype : remove) {
            atypes.remove(atype);
        }
        String[] res = new String[atypes.size()];
        for (int i=0; i<atypes.size();i++) {
            String atype = atypes.get(i).getFullType(-1)
                                            .toLowerCase()
                                            .replaceAll("\\.", "->NE")
                                            .replaceAll("^","NE");
            StringBuilder sb = new StringBuilder(atype);
            Matcher m = Pattern.compile("_(\\w)").matcher(atype);
            while (m.find()) {
                sb.replace(m.start(), m.end(), m.group(1).toUpperCase());
                m = Pattern.compile("_(\\w)").matcher(sb.toString());
            }
            res[i] = sb.toString();
        }
        return res;
    }
    
	/**
	 * Analyzes a question string.
	 * 
	 * @param question question string
	 * @return analyzed question
	 */
	public static AnalyzedQuestion analyze(String question) {
		// normalize question
		String qn = QuestionNormalizer.normalize(question);
		
		// stem verbs and nouns
		String stemmed = QuestionNormalizer.stemVerbsAndNouns(qn);
		MsgPrinter.printNormalization(stemmed);
		Logger.logNormalization(stemmed);
		
		// resolve verb constructions with auxiliaries
		String verbMod = (QuestionNormalizer.handleAuxiliaries(qn))[0];
		// TODO return only one best string
		
		// extract keywords
		String[] kws = KeywordExtractor.getKeywords(verbMod, context);
		
		// extract named entities
		String[][] nes = TermExtractor.getNes(question, context);
		
		// extract terms and set relative frequencies
		Term[] terms = TermExtractor.getTerms(verbMod, context, nes,
				dicts.toArray(new Dictionary[dicts.size()]));
		for (Term term : terms)
			term.setRelFrequency(WordFrequencies.lookupRel(term.getText()));
		
		// extract focus word
		String focus = FocusFinder.findFocusWord(question);
		
		// determine answer types
		//String[] ats = AnswerTypeTester.getAnswerTypes(qn, stemmed);
        String[] ats = getAtypes(question);
		MsgPrinter.printAnswerTypes(ats);
		Logger.logAnswerTypes(ats);
		
		// interpret question
		QuestionInterpretation[] qis =
			QuestionInterpreter.interpret(qn, stemmed);
		MsgPrinter.printInterpretations(qis);
		Logger.logInterpretations(qis);
		
		// extract predicates
		Predicate[] ps = (predicates != null) ? predicates
				: PredicateExtractor.getPredicates(qn, verbMod, ats, terms);
		MsgPrinter.printPredicates(ps);
		Logger.logPredicates(ps);
		
		// expand terms
		TermExpander.expandTerms(terms, ps,
				ontologies.toArray(new Ontology[ontologies.size()]));
		
		return new AnalyzedQuestion(question, qn, stemmed, verbMod, kws, nes,
				terms, focus, ats, qis, ps);
	}
    
    public static void main (String[] args) {
        String[] atypes = getAtypes(args[0]);
        System.out.println(args[0]);
        for (String atype : atypes) {
            System.out.println(atype);
        }
        System.out.println("Done!");
    }
}

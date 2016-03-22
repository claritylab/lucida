package info.ephyra.answerselection.filters;

import info.ephyra.nlp.NETagger;
import info.ephyra.nlp.SnowballStemmer;
import info.ephyra.nlp.indices.FunctionWords;
import info.ephyra.search.Result;
import info.ephyra.trec.TRECNugget;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.FileWriter;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

/**
 * <p>Automatically evaluates answer candidates for the 'other' questions in
 * TREC 13-16.</p>
 * 
 * <p>This class extends the class <code>Filter</code>.</p>
 * 
 * @author Guido Sautter
 * @version 2008-02-15
 */
public class NuggetEvaluationFilter extends Filter {
	private static final String DEFAULT_EXPORT_DATE_FORMAT = "yyyyMMdd-HHmm";
	private static final DateFormat EXPORT_DATE_FORMATTER = new SimpleDateFormat(DEFAULT_EXPORT_DATE_FORMAT);
	
	private static final String baseFileName = ("log/TREC15_OTHER_ASSESS_" + EXPORT_DATE_FORMATTER.format(new Date()));
	
	private String fileNameMarker = "";
	
	private String fileName = baseFileName;
	private String conciseFileName = baseFileName + "_Concise";
	
	private String lastTarget = null;
	
//	private static ArrayList<NuggetEvaluationFilter> instances = new ArrayList<NuggetEvaluationFilter>();
	private static HashMap<String, NuggetEvaluationFilter> instanceSet = new LinkedHashMap<String, NuggetEvaluationFilter>();
	
	public NuggetEvaluationFilter() {
		this(null);
	}
	
	/**
	 */
	@SuppressWarnings("unchecked")
	public NuggetEvaluationFilter(String fileNameMarker) {
		
		if (nuggetsByTargetID == null) loadNuggets();
		
		if (fileNameMarker != null) {
			this.fileName = baseFileName + fileNameMarker;
			this.fileNameMarker = fileNameMarker;
		}
		this.conciseFileName = this.fileName + "_Concise";
		
		this.targetId = targetID;
		this.nuggets = (this.targetId == null) ? new ArrayList<TRECNugget>() : ((ArrayList<TRECNugget>) nuggetsByTargetID.get(this.targetId).clone());
//		instances.add(this);
		instanceSet.put(this.fileNameMarker, this);
		System.out.println("NuggetEvaluationFilter: instance created, overall instances: " + instanceSet.size());
	}

	/**
	 * This method is not used. Instead, the method <code>apply(Result[])</code>
	 * from the superclass <code>Filter</code> is overwritten.
	 * 
	 * @param result a <code>Result</code> object
	 * @return the same <code>Result</code> object
	 */
	public Result apply(Result result) {
		return result;
	}
	
	/**
	 * Extracts NEs of particular types from the answer strings of the
	 * <code>Result</code> objects and creates a new <code>Result</code> for
	 * each extracted unique answer.
	 * 
	 * @param results array of <code>Result</code> objects
	 * @return extended array of <code>Result</code> objects
	 */
	public Result[] apply(Result[] results) {
		if ((results.length == 0) || (targetId == null)) return results;
		
		this.lastTarget = results[0].getQuery().getOriginalQueryString();
		
		int nonWhiteLength = 0;
		int notifyLength = 1000;
		
		BufferedWriter br = null;
		try {
			br = new BufferedWriter(new FileWriter(this.fileName, true));
			br.write("===== Assessing target " + targetId + " (" + results[0].getQuery().getOriginalQueryString() + ") =====");
			br.newLine();
		} catch (Exception e) {}
		
		BufferedWriter cbr = null;
		try {
			cbr = new BufferedWriter(new FileWriter(this.conciseFileName, true));
			cbr.write("===== Assessing target " + targetId + " (" + results[0].getQuery().getOriginalQueryString() + ") =====");
			cbr.newLine();
		} catch (Exception e) {}
		
		float maxScore = results[0].getScore();
		boolean maxCutWritten = false;
		
		HashSet<TRECNugget> covered = new LinkedHashSet<TRECNugget>();
		HashMap<TRECNugget, Integer> coveredWhen = new HashMap<TRECNugget, Integer>();
		
		int vital = 0;
		int ok = 0;
		
		for (int i = 0; i < 7; i++) {
			lastVital[i] = 0;
			lastOk[i] = 0;
		}
		
		for (int r = 0; r < results.length; r++) {
			Result res = results[r];
			boolean resWritten = false;
			
			String[] tok = res.getAnswer().split("\\s++");
			for (int t = 0; t < tok.length; t++) nonWhiteLength += tok[t].length();
			
			//	write all snippets for the first 7000 characters
			if ((br != null) && (nonWhiteLength < 7000)) try {
				br.write("Result " + r + " (" + res.getScore() + ") is: " + res.getAnswer());
				br.newLine();
				resWritten = true;
			} catch (Exception e) {}
			
			if (nonWhiteLength > notifyLength) {
				
				int index = ((notifyLength - 1) / 1000);
				if (index < 7) {
					lastVital[index] = vital;
					lastOk[index] = ok;
				}
				
				if (br != null) try {
					br.write("===== " + notifyLength + " non-white char cutoff ===== ");
					br.newLine();
				} catch (Exception e) {}
				
				notifyLength += 1000;
			}
			
			if ((br != null) && !maxCutWritten && ((res.getScore() * 2) < maxScore)) try {
				br.write("===== half score cutoff ===== ");
				br.newLine();
				maxCutWritten = true;
			} catch (Exception e) {}
			
			int n = 0;
			while (n < nuggets.size()) {
				TRECNugget nug = nuggets.get(n);
				String[] uncovered = covers(res.getAnswer(), nug.nugget);
				if ((uncovered.length * 2) <= nug.size) {
					
					if (br != null) try {
						if (!resWritten) {
							br.write("Result " + r + " (" + res.getScore() + ") is: " + res.getAnswer());
							br.newLine();
							resWritten = true;
						}
						br.write("  Nugget covered (" + nug.nuggetID + "," + nug.nuggetType + "): " + nug.nugget);
						br.newLine();
						if (uncovered.length != 0) {
							br.write("      Uncovered:");
							for (String u : uncovered) br.write(" " + u);
							br.newLine();
						}
					} catch (Exception e) {}
					
					res.addCoveredNuggetID(nug.nuggetID);
					
					covered.add(nug);
					if ((uncovered.length * 4) <= nug.size) nuggets.remove(n);
					else n++;
					
					if (!coveredWhen.containsKey(nug)) {
						if ("vital".equals(nug.nuggetType)) vital++;
						else ok++;
						coveredWhen.put(nug, new Integer(nonWhiteLength));
					}
					
				} else {
					n++;
				}
			}
			
			if (resWritten && (br != null)) try {
				br.newLine();
			} catch (Exception e) {}
		}
		
		
		if (br != null) try {
			
			ArrayList<TRECNugget> coveredNugs = new ArrayList<TRECNugget>(covered);
			for (TRECNugget nug : coveredNugs) {
				int when = -1;
				if (coveredWhen.containsKey(nug))
					when = coveredWhen.get(nug).intValue();
				br.write("  (probably) covered (" + nug.nuggetID + "," + nug.nuggetType + ")" + ((when == -1) ? "" : (" first at " + when)) + ": " + nug.nugget);
				br.newLine();
			}
			
			for (TRECNugget nug : nuggets) {
				br.write("  Not (securely) covered (" + nug.nuggetID + "," + nug.nuggetType + "): " + nug.nugget);
				br.newLine();
			}
			
			br.newLine();
			br.newLine();
			br.flush();
			br.close();
		} catch (Exception e) {}
		
		if (cbr != null) try {
			
			ArrayList<TRECNugget> coveredNugs = new ArrayList<TRECNugget>(covered);
			for (TRECNugget nug : coveredNugs) {
				int when = -1;
				if (coveredWhen.containsKey(nug))
					when = coveredWhen.get(nug).intValue();
				cbr.write("  (probably) covered (" + nug.nuggetID + "," + nug.nuggetType + ")" + ((when == -1) ? "" : (" first at " + when)) + ": " + nug.nugget);
				cbr.newLine();
			}
			
			for (TRECNugget nug : nuggets) {
				cbr.write("  Not (securely) covered (" + nug.nuggetID + "," + nug.nuggetType + "): " + nug.nugget);
				cbr.newLine();
			}
			
			cbr.newLine();
			cbr.newLine();
			cbr.flush();
			cbr.close();
		} catch (Exception e) {}
		
		return results;
	}
	
	/**	check if some result covers some nugger
	 * @param	result	the result String
	 * @param	nugget	the nugget string
	 * @return the tokens of the specified nugget String not contained in the specified result String
	 */
	private String[] covers(String result, String nugget) {
		String[] rTokens = NETagger.tokenize(result);
		HashSet<String> rSet = new HashSet<String>();
		for (String r : rTokens)
			if (!FunctionWords.lookup(r) && (r.length() > 1))
				rSet.add(SnowballStemmer.stem(r).toLowerCase());
		
		String[] nTokens = NETagger.tokenize(nugget);
		HashSet<String> nSet = new HashSet<String>();
		for (String n : nTokens)
			if (!FunctionWords.lookup(n) && (n.length() > 1))
				nSet.add(SnowballStemmer.stem(n).toLowerCase());
		
		nSet.removeAll(rSet);
		ArrayList<String> remaining = new ArrayList<String>(nSet);
		
		return remaining.toArray(new String[remaining.size()]);
	}
	
	private static String targetID = null;
	
	private String targetId = null;
	private ArrayList<TRECNugget> nuggets = new ArrayList<TRECNugget>();
	private static int numVital = 0;
	
	private int[] lastVital = new int[7];
	private int[] lastOk = new int[7];
	
	private static HashMap<String, ArrayList<TRECNugget>> nuggetsByTargetID = null;
	
	/**	set the ID of the next target, so upcoming results can be checked against the respective nuggets
	 * @param tid the ID of the next target
	 */
	@SuppressWarnings("unchecked")
	public static synchronized void setTargetID(String tid) {
		System.out.println("NuggetEvaluationFilter: global target ID set to " + tid);
		
		if (nuggetsByTargetID == null) {
			nuggetsByTargetID = new HashMap<String, ArrayList<TRECNugget>>();
			loadNuggets();
		}
		
//		for (NuggetEvaluationFilter instance : instances)
//			instance.setTargetId(tid);
		
		for (Iterator<NuggetEvaluationFilter> ii = instanceSet.values().iterator(); ii.hasNext();)
			ii.next().setTargetId(tid);
		
		targetID = tid;
		ArrayList<TRECNugget> nuggets = (tid == null) ? new ArrayList<TRECNugget>() : (nuggetsByTargetID.get(tid));
		numVital = 0;
		if (nuggets != null) {
			for (TRECNugget nug : nuggets)
				if ("vital".equals(nug.nuggetType)) numVital++;
		}
	}
	
	/**	gather data from instances and write it to global log file
	 */
	public static void targetFinished() {
		try {
			BufferedWriter br = new BufferedWriter(new FileWriter(baseFileName, true));
			br.newLine();
			
//			String titleLine = ";;";
			String titleLine = null;
			for (Iterator<NuggetEvaluationFilter> ii = instanceSet.values().iterator(); ii.hasNext();) {
				NuggetEvaluationFilter nef = ii.next();
				if (titleLine == null) titleLine = ("\"" + nef.lastTarget + "\";\"#chars\";\"#vital\"");
				titleLine += ";" + nef.fileNameMarker + ";;;;";
			}
			
//			for (NuggetEvaluationFilter instance : instances)
//				titleLine += ";" + instance.fileNameMarker + ";;;"; 
			
			br.write(titleLine);
			br.newLine();
			
			for (int i = 0; i < 7; i++) {
				String line = targetID + ";" + ((i+1) * 1000) + ";" + numVital;
				
				for (Iterator<NuggetEvaluationFilter> ii = instanceSet.values().iterator(); ii.hasNext();) {
					NuggetEvaluationFilter nef = ii.next();
					line += ";" + nef.lastOk[i] + ";" + nef.lastVital[i] + ";;;";
				}
				
//				for (NuggetEvaluationFilter instance : instances)
//					line += ";" + instance.lastOk[i] + ";" + instance.lastVital[i] + ";;";
				
				br.write(line);
				br.newLine();
			}
			
			br.flush();
			br.close();
		} catch (Exception e) {}
	}
	
	@SuppressWarnings("unchecked")
	private void setTargetId(String tid) {
		System.out.println("NuggetEvaluationFilter (" + this.fileName + "): local target ID set to " + tid);
		targetId = tid;
		nuggets = (tid == null) ? new ArrayList<TRECNugget>() : ((ArrayList<TRECNugget>) nuggetsByTargetID.get(tid).clone());
	}
	
	/**
	 * @return an array containing the nuggets not covered by results that passed the filter so far
	 */
	public TRECNugget[] getNuggets() {
		return nuggets.toArray(new TRECNugget[nuggets.size()]);
	}
	
	/**
	 * load the nuggets from the answer file
	 */
	private static void loadNuggets() {
		try {
			BufferedReader br = new BufferedReader(new FileReader("./res/testdata/trec/trec15answers_other"));
			String targetID = null;
			ArrayList<TRECNugget> nuggets = new ArrayList<TRECNugget>();
			
			while (br.ready()) {
				String line = br.readLine();
				if ((line != null) && (line.length() != 0) && !line.startsWith("Qid")) {
					String[] parts = line.split("((\\s++)|\\.)", 5);
					
					TRECNugget nugget = new TRECNugget(parts[0], parts[1], parts[2], parts[3], parts[4]);
					
					if (!nugget.targetID.equals(targetID)) {
						if (targetID != null) nuggetsByTargetID.put(targetID, nuggets);
						targetID = nugget.targetID;
						nuggets = new ArrayList<TRECNugget>();
					}
					
					nuggets.add(nugget);
				}
			}
			
			if (targetID != null) nuggetsByTargetID.put(targetID, nuggets);
			
			br.close();
		} catch (Exception e) {
			System.out.println(e.getClass().getName() + " (" + e.getMessage() + ") while loading nuggets");
			e.printStackTrace(System.out);
		}
	}
}
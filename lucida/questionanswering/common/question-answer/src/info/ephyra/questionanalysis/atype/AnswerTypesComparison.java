package info.ephyra.questionanalysis.atype;

import info.ephyra.util.FileUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.HashSet;

/**
 * Compares the answer types used by the classifier to the answer types that can
 * be tagged and lists the differences.
 * 
 * @author Nico Schlaefer
 * @version 2007-07-13
 */
public class AnswerTypesComparison {
	/**
	 * Compares the answer types and lists differences.
	 * 
	 * @param args ignored
	 */
	public static void main(String[] args) {
		// get answer types in hierarchy
		HashSet<String> hierarchyTypes = new HashSet<String>();
		File hierarchyFile =
			new File("../Ephyra/res/answertypes/typesystem/ephyra.types");
		try {
			FileInputStream fis = new FileInputStream(hierarchyFile);
			BufferedReader in =
				new BufferedReader(new InputStreamReader(fis, "UTF-8"));
			while (in.ready()) {
				String line = in.readLine();
				String[] types = line.split("->");
				for (String type : types) hierarchyTypes.add(type);
			}
		} catch (IOException e) {
			System.err.println("Could not read hierarchy:");
			System.err.println(e.getMessage());
			System.exit(0);
		}
		
		// get answer types with taggers
		HashSet<String> taggerTypes = new HashSet<String>();
		// - model-based taggers
		taggerTypes.add("NElocation");
		taggerTypes.add("NEorganization");
		taggerTypes.add("NEperson");
		// - pattern-based taggers
		File patternFile = new File("../Ephyra/res/nlp/netagger/patterns.lst");
		try {
			FileInputStream fis = new FileInputStream(patternFile);
			BufferedReader in =
				new BufferedReader(new InputStreamReader(fis, "UTF-8"));
			while (in.ready()) taggerTypes.add("NE" + in.readLine());
		} catch (IOException e) {
			System.err.println("Could not read patterns file:");
			System.err.println(e.getMessage());
			System.exit(0);
		}
		// - list-based taggers
		File[] listFiles =
			FileUtils.getFiles("../Ephyra/res/nlp/netagger/lists/");
		for (File listFile : listFiles) {
			String type = listFile.getName();
			if (!type.endsWith(".lst")) continue;
			taggerTypes.add("NE" + type.substring(0, type.length() - 4));
		}
		
		System.out.println("Missing types in hierarchy:");
		for (String taggerType : taggerTypes)
			if (!hierarchyTypes.contains(taggerType))
				System.out.println(taggerType);
		System.out.println();
		
		System.out.println("Types without NE taggers:");
		for (String hierarchyType : hierarchyTypes)
			if (!taggerTypes.contains(hierarchyType))
				System.out.println(hierarchyType);
		System.out.println();
		
		System.out.println("Number of types in hierarchy: " +
				hierarchyTypes.size());
		System.out.println();
		
		System.out.println("Number of types with NE taggers: " +
				taggerTypes.size());
	}
}

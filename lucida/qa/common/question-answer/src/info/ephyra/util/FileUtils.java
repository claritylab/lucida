package info.ephyra.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.io.StringReader;
import java.util.ArrayList;

/**
 * A collection of file system related utilities.
 * 
 * @author Nico Schlaefer
 * @version 2007-05-05
 */
public class FileUtils {
	/**
	 * Returns the files in the given directory (only normal files, no
	 * subdirectories).
	 * 
	 * @param dir a directory
	 * @return files in the directory
	 */
	public static File[] getFiles(String dir) {
		ArrayList<File> files = new ArrayList<File>();
		
		// only return normal files, no subdirectories
		File[] filesOrDirs = new File(dir).listFiles();
		for (File fileOrDir : filesOrDirs)
			if (fileOrDir.isFile()) files.add(fileOrDir);
		
		return files.toArray(new File[files.size()]);
	}
	
	/**
	 * Returns the files in the given directory and its subdirectories.
	 * 
	 * @param dir a directory
	 * @return files in the directory and subdirectories
	 */
	public static File[] getFilesRec(String dir) {
		ArrayList<File> files = new ArrayList<File>();
		
		// recursively browse directories
		getFilesRec(new File(dir), files);
		
		return files.toArray(new File[files.size()]);
	}
	
	/**
	 * Recursively browses a directory and its subdirectories for files.
	 * 
	 * @param dir a directory
	 */
	private static void getFilesRec(File dir, ArrayList<File> files) {
		File[] filesOrDirs = dir.listFiles();
		for (File fileOrDir : filesOrDirs)
			if (fileOrDir.isFile()) files.add(fileOrDir);  // add normal files
			else getFilesRec(fileOrDir, files);  // browse subdirectories
	}
	
	/**
	 * Returns the subdirectories of the given directory.
	 * 
	 * @param dir a directory
	 * @return subdirectories
	 */
	public static String[] getSubDirs(String dir) {
		ArrayList<String> subDirs = new ArrayList<String>();
		
		// only return subdirectories, no normal files
		File[] filesOrDirs = new File(dir).listFiles();
		for (File fileOrDir : filesOrDirs)
			if (fileOrDir.isDirectory()) subDirs.add(fileOrDir.getName());
		
		return subDirs.toArray(new String[subDirs.size()]);
	}
	
	/**
	 * Returns the visible subdirectories of the given directory.
	 * 
	 * @param dir a directory
	 * @return visible subdirectories
	 */
	public static String[] getVisibleSubDirs(String dir) {
		String[] subDirs = getSubDirs(dir);
		
		ArrayList<String> visible = new ArrayList<String>();
		for (String subDir : subDirs)
			if (!subDir.startsWith(".")) visible.add(subDir);
		return visible.toArray(new String[visible.size()]);
	}
	
	/**
	 * Reads a string from a file, using the given encoding.
	 * 
	 * @param input input file
	 * @param encoding file encoding
	 * @return string
	 */
	public static String readString(File input, String encoding)
			throws IOException {
		StringBuffer buffer = new StringBuffer();
		
		FileInputStream fis = new FileInputStream(input);
		BufferedReader reader =
			new BufferedReader(new InputStreamReader(fis, encoding));
		for (String nextLine; (nextLine = reader.readLine()) != null;)
			buffer.append(nextLine + "\n");
		reader.close();
		
		return buffer.toString();
	}
	
	/**
	 * Writes a string to a file, using the given encoding. An existing file is
	 * overwritten.
	 * 
	 * @param s string
	 * @param output output file
	 * @param encoding file encoding
	 */
	public static void writeString(String s, File output, String encoding)
			throws IOException {
		BufferedReader buffer = new BufferedReader(new StringReader(s));
		
		FileOutputStream fos = new FileOutputStream(output);
		PrintWriter writer =
			new PrintWriter(new OutputStreamWriter(fos, encoding));
		for (String nextLine; (nextLine = buffer.readLine()) != null;)
			writer.println(nextLine);
		writer.close();
		
		buffer.close();
	}
	
	/**
	 * Reads a serialized object from a file.
	 * 
	 * @param input input file
	 * @return object
	 */
	public static Object readSerialized(File input)
			throws IOException, ClassNotFoundException {
		FileInputStream fis = new FileInputStream(input);
		ObjectInputStream ois = new ObjectInputStream(fis);
		
		Object o = ois.readObject();
		
		ois.close();
		
		return o;
	}
	
	/**
	 * Writes a serialized object to a file.
	 * 
	 * @param o object
	 * @param output output file
	 */
	public static void writeSerialized(Object o, File output)
			throws IOException {
		FileOutputStream fos = new FileOutputStream(output);
		ObjectOutputStream oos = new ObjectOutputStream(fos);
		
		oos.writeObject(o);
		
		oos.close();
	}
}

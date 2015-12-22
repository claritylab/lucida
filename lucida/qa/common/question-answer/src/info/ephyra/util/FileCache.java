package info.ephyra.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;

/**
 * <p>The <code>FileCache</code> is a simple implementation of a permanent
 * cache. The entries of the cache are accessed by keys. Both keys and entries
 * are strings, and there may be an arbitrary number of entries for a key.</p>
 * 
 * <p>The entries are stored in files in a directory which is specified when the
 * cache handler is created. There is one file for each key and the MD5 checksum
 * of the key is used as the filename.</p>
 * 
 * @author Nico Schlaefer
 * @version 2006-11-27
 */
public class FileCache {
	/** The directory where the files are stored. */
	private String cacheDir;
	
	/**
	 * Creates a new cache handler and sets the directory of the cache.
	 * 
	 * @param cacheDir cache directory
	 */
	public FileCache(String cacheDir) {
		this.cacheDir = cacheDir;
	}
	
	/**
	 * Computes the MD5 checksum of a string.
	 * 
	 * @param s the string
	 * @return checksum, or <code>null</code> if the MD5 algorithm is not
	 *         available
	 */
	private String getMD5(String s) {
		MessageDigest digest;
		try {
			 digest = MessageDigest.getInstance("MD5");
		} catch (NoSuchAlgorithmException e) {return null;}
		digest.update(s.getBytes());		
		byte[] md5sum = digest.digest();
		BigInteger bigInt = new BigInteger(1, md5sum);
		
		return bigInt.toString(16);
	}
	
	/**
	 * Read the entries for the given key from the cache.
	 * 
	 * @param key the key
	 * @return the entries, or <code>null</code> if the key is not in the cache
	 */
	public String[] read(String key) {
		// compute checksum for the key
		String checksum = getMD5(key);
		if (checksum == null) return null;
		
		// read cache entries from a file, using the checksum as the filename
		File file = new File(cacheDir, new String(checksum));
		try {
			ArrayList<String> entries = new ArrayList<String>();
			
			BufferedReader in = new BufferedReader(new FileReader(file));
			while (in.ready()) entries.add(in.readLine());
			in.close();
			
			return entries.toArray(new String[entries.size()]);
		} catch (IOException e) {return null;}  // key is not in the cache
	}
	
	/**
	 * Writes new entries to the cache. Existing entries with the given key are
	 * overwritten.
	 * 
	 * @param key the key
	 * @param entries the entries
	 * @return <code>true<code> iff the entries could be written to the cache
	 */
	public boolean write(String key, String[] entries) {
		// compute checksum for the key
		String checksum = getMD5(key);
		if (checksum == null) return false;
		
		// write new cache entries to a file, using the checksum as the filename
		File file = new File(cacheDir, new String(checksum));
		try {
			PrintWriter out = new PrintWriter(file);
			for (String entry : entries) out.println(entry);
			out.close();
			
			return true;
		} catch (IOException e) {return false;}  // entries could not be written
	}
}

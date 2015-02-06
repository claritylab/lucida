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

package edu.cmu.sphinx.util;

import java.io.*;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

/**
 * Supports the loading and saving of files from different sources, e.g., a ZIP file or a plain directory. Provides
 * methods that returns an InputStream or OutputStream to the named file in the given source.
 */
public class StreamFactory {

    /** Identifies a ZIP file. */
    public static final String ZIP_FILE = "ZIP_FILE";


    /** Identifies a plain directory. */
    public static final String DIRECTORY = "DIRECTORY";


    /**
     * Returns an appropriate InputStream of the given file in the given URL location. The location can be a plain
     * directory or a ZIP file (these are the only two supported at this point). The <code>resolve</code> method is
     * called to resolve whether "location" refers to a ZIP file or a directory.
     * <p/>
     * Suppose you want the InputStream to the file "dict/dictionary.txt" in the ZIP file
     * "file:/lab/speech/sphinx4/data/wsj.zip". You will do: <code> StreamFactory.getInputStream(
     * "file:/lab/speech/sphinx4/data/wsj.zip", "dict/dictionary.txt"); </code>
     * <p/>
     * Suppose you want the InputStream to the file "dict/dictionary.txt" in the directory
     * "file:/lab/speech/sphinx4/data/wsj", you will do: <code> StreamFactory.getInputStream(
     * "file:/lab/speech/sphinx4/data/wsj", "dict/dictionary.txt"); </code>
     * <p/>
     * The <code>StreamFactory.resolve()</code> method is called to resolve whether "location" refers to a ZIP file or a
     * directory.
     *
     * @param location the URL location of the input data, it can now be a directory or a ZIP file
     * @param file     the file in the given location to obtain the InputStream
     * @return an InputStream of the given file in the given location
     */
    public static InputStream getInputStream(String location,
                                             String file) throws
            IOException {
        if (location != null) {
            return StreamFactory.getInputStream
                    (StreamFactory.resolve(location), location, file);
        } else {
            return StreamFactory.getInputStream(StreamFactory.DIRECTORY,
                    location, file);
        }
    }


    /**
     * According to the given data format, returns an appropriate InputStream of the given file in the given URL
     * location. The location can be a plain directory or a JAR or ZIP file (these are the only ones supported at this
     * point).
     * <p/>
     * Suppose you want the InputStream to the file "dict/dictionary.txt" in the ZIP file
     * "file:/lab/speech/sphinx4/data/wsj.zip". You will do: <code> StreamFactory.getInputStream(StreamFactory.ZIP_FILE,
     * "file:/lab/speech/sphinx4/data/wsj.zip", "dict/dictionary.txt"); </code>
     * <p/>
     * Suppose you want the InputStream to the file "dict/dictionary.txt" in the directory
     * "file:/lab/speech/sphinx4/data/wsj", you will do: <code> StreamFactory.getInputStream(StreamFactory.DIRECTORY,
     * "file:/lab/speech/sphinx4/data/wsj", "dict/dictionary.txt"); </code>
     *
     * @param format   the format of the input data, the currently supported formats are: <br>StreamFactory.ZIP_FILE
     *                 <br>StreamFactory.DIRECTORY
     * @param location the URL location of the input data, it can now be a directory or a JAR or ZIP file, or null if no
     *                 location is given, which means that the <code>argument</code> also specifies the exact location
     * @param file     the file in the given location to obtain the InputStream
     * @return an InputStream of the given file in the given location
     */
    public static InputStream getInputStream(String format,
                                             String location,
                                             String file) throws
            IOException {
        InputStream stream = null;
        String absoluteLocation;

        if (location == null) {
            absoluteLocation = null;
        } else {
            // Create a url from the location, possibly relative path
            URI uri = URI.create(location);
            // Get the scheme and the path
            String scheme = uri.getScheme();
            String path = uri.getSchemeSpecificPart();
            // Create a file with the path (aka scheme-specific part)
            File relativeFile = new File(path);
            // Make the path absolute and reconstruct the location, with
            // the correct scheme
            URI absoluteURI = relativeFile.getAbsoluteFile().toURI();
            if (scheme == null) {
                absoluteLocation = absoluteURI.getSchemeSpecificPart();
            } else {
                absoluteLocation = scheme + ':' +
                        absoluteURI.getSchemeSpecificPart();
            }
        }

        if (format.equals(ZIP_FILE)) {
            try {
                URI newURI = new URI(absoluteLocation);
                ZipFile zipFile =
                        new ZipFile(new File(newURI));
                ZipEntry entry = zipFile.getEntry(file);
                if (entry != null) {
                    stream = zipFile.getInputStream(entry);
                }
                zipFile.close();
            } catch (URISyntaxException use) {
                use.printStackTrace();
                throw new ZipException("URISyntaxException: " +
                        use.getMessage());
            }
        } else if (format.equals(DIRECTORY)) {
            if (absoluteLocation != null) {
                stream = new FileInputStream(absoluteLocation +
                        File.separator + file);
            } else {
                stream = new FileInputStream(file);
            }
        }
        return stream;
    }


    /**
     * Returns an appropriate OutputStream of the given file in the given URL location.  The location can be a plain
     * directory or a ZIP file (these are the only two supported at this point).  The <code>resolve</code> method is
     * called to resolve whether "location" refers to a ZIP file or a directory. If saving to a zip or jar, the file can
     * be appended or overwritten. If saving to a directory, files are always overwritten.
     * <p/>
     * Suppose you want the OutputStream to the file "dict/dictionary.txt" in the ZIP file
     * "file:/lab/speech/sphinx4/data/wsj.zip". You will do: <code> StreamFactory.getOutputStream(
     * "file:/lab/speech/sphinx4/data/wsj.zip", "dict/dictionary.txt", true); </code>
     * <p/>
     * Suppose you want the OutputStream to the file "dict/dictionary.txt" in the directory
     * "file:/lab/speech/sphinx4/data/wsj", you will do: <code> StreamFactory.getOutputStream(
     * "file:/lab/speech/sphinx4/data/wsj", "dict/dictionary.txt", false); </code>
     * <p/>
     * The <code>StreamFactory.resolve()</code> method is called to resolve whether "location" refers to a ZIP file or a
     * directory.
     *
     * @param location the URL location of the output data, it can now be a directory or a ZIP file
     * @param file     the file in the given location to obtain the OutputStream
     * @param append   if true and saving to a zip file, then file is appended rather than overwritten.
     * @return an OutputStream of the given file in the given location
     */
    public static OutputStream getOutputStream(String location,
                                               String file,
                                               boolean append) throws
            IOException {
        if (location != null) {
            return StreamFactory.getOutputStream
                    (StreamFactory.resolve(location), location, file, append);
        } else {
            return StreamFactory.getOutputStream(StreamFactory.DIRECTORY,
                    location, file);
        }
    }


    /**
     * According to the given data format, returns an appropriate OutputStream of the given file in the given URL
     * location.  The location can be a plain directory or a JAR or ZIP file (these are the only ones supported at this
     * point). If saving to a zip or jar, the file can be appended or overwritten. If saving to a directory, files are
     * always overwritten.
     * <p/>
     * Suppose you want the OutputStream to the file "dict/dictionary.txt" in the ZIP file
     * "file:/lab/speech/sphinx4/data/wsj.zip". You will do: <code> StreamFactory.getOutputStream(StreamFactory.ZIP_FILE,
     * "file:/lab/speech/sphinx4/data/wsj.zip", "dict/dictionary.txt", true); </code>
     * <p/>
     * Suppose you want the OutputStream to the file "dict/dictionary.txt" in the directory
     * "file:/lab/speech/sphinx4/data/wsj", you will do: <code> StreamFactory.getOutputStream(StreamFactory.DIRECTORY,
     * "file:/lab/speech/sphinx4/data/wsj", "dict/dictionary.txt", false); </code>
     *
     * @param format   the format of the output data, the currently supported formats are: <br>StreamFactory.ZIP_FILE
     *                 <br>StreamFactory.DIRECTORY
     * @param location the URL location of the output data, it can now be a directory or a JAR or ZIP file, or null if
     *                 no location is given, which means that the <code>argument</code> also specifies the exact
     *                 location
     * @param file     the file in the given location to obtain the OutputStream
     * @param append   if true and saving to a zip file, then file is appended rather than overwritten.
     * @return an OutputStream of the given file in the given location
     */
    public static OutputStream getOutputStream(String format,
                                               String location,
                                               String file,
                                               boolean append) throws
            IOException {
        OutputStream stream = null;
        if (format.equals(ZIP_FILE)) {
            try {
                System.out.println("WARNING: ZIP not yet fully supported.!");
                File path = new File(location);
                File parent = new File(path.getParent());
                if (!parent.exists()) {
                    parent.mkdirs();
                }
                FileOutputStream fos =
                        new FileOutputStream(new File(new URI(location)), append);
                stream = new ZipOutputStream(new BufferedOutputStream(fos));

                ZipEntry entry = new ZipEntry(file);
                ((ZipOutputStream) stream).putNextEntry(entry);
            } catch (URISyntaxException use) {
                use.printStackTrace();
                throw new ZipException("URISyntaxException: " +
                        use.getMessage());
            }
        } else if (format.equals(DIRECTORY)) {
            if (location != null) {
                File path = new File(location + File.separator + file);
                File parent = new File(path.getParent());
                if (!parent.exists()) {
                    parent.mkdirs();
                }
                stream = new FileOutputStream(location + File.separator + file);
            } else {
                File path = new File(file);
                File parent = new File(path.getParent());
                if (!parent.exists()) {
                    parent.mkdirs();
                }
                stream = new FileOutputStream(file);
            }
        } else {
            throw new IOException("Format not supported for writing");
        }
        return stream;
    }


    /**
     * Returns an appropriate OutputStream of the given file in the given URL location.  The location can be a plain
     * directory or a ZIP file (these are the only two supported at this point).  The <code>resolve</code> method is
     * called to resolve whether "location" refers to a ZIP file or a directory. Files are overwritten, which may be
     * risky for ZIP of JAR files.
     * <p/>
     * Suppose you want the OutputStream to the file "dict/dictionary.txt" in the ZIP file
     * "file:/lab/speech/sphinx4/data/wsj.zip". You will do: <code> StreamFactory.getOutputStream(
     * "file:/lab/speech/sphinx4/data/wsj.zip", "dict/dictionary.txt"); </code>
     * <p/>
     * Suppose you want the OutputStream to the file "dict/dictionary.txt" in the directory
     * "file:/lab/speech/sphinx4/data/wsj", you will do: <code> StreamFactory.getOutputStream(
     * "file:/lab/speech/sphinx4/data/wsj", "dict/dictionary.txt"); </code>
     * <p/>
     * The <code>StreamFactory.resolve()</code> method is called to resolve whether "location" refers to a ZIP file or a
     * directory.
     *
     * @param location the URL location of the output data, it can now be a directory or a ZIP file
     * @param file     the file in the given location to obtain the OutputStream
     * @return an OutputStream of the given file in the given location
     */
    public static OutputStream getOutputStream(String location,
                                               String file) throws
            IOException {
        if (location != null) {
            return StreamFactory.getOutputStream
                    (StreamFactory.resolve(location), location, file);
        } else {
            return StreamFactory.getOutputStream(StreamFactory.DIRECTORY,
                    location, file);
        }
    }


    /**
     * According to the given data format, returns an appropriate OutputStream of the given file in the given URL
     * location.  The location can be a plain directory or a JAR or ZIP file (these are the only ones supported at this
     * point). Files are always overwritten, which can be risky for ZIP or JAR files.
     * <p/>
     * Suppose you want the OutputStream to the file "dict/dictionary.txt" in the ZIP file
     * "file:/lab/speech/sphinx4/data/wsj.zip". You will do: <code> StreamFactory.getOutputStream(StreamFactory.ZIP_FILE,
     * "file:/lab/speech/sphinx4/data/wsj.zip", "dict/dictionary.txt"); </code>
     * <p/>
     * Suppose you want the OutputStream to the file "dict/dictionary.txt" in the directory
     * "file:/lab/speech/sphinx4/data/wsj", you will do: <code> StreamFactory.getOutputStream(StreamFactory.DIRECTORY,
     * "file:/lab/speech/sphinx4/data/wsj", "dict/dictionary.txt"); </code>
     *
     * @param format   the format of the output data, the currently supported formats are: <br>StreamFactory.ZIP_FILE
     *                 <br>StreamFactory.DIRECTORY
     * @param location the URL location of the output data, it can now be a directory or a JAR or ZIP file, or null if
     *                 no location is given, which means that the <code>argument</code> also specifies the exact
     *                 location
     * @param file     the file in the given location to obtain the OutputStream
     * @return an OutputStream of the given file in the given location
     */
    public static OutputStream getOutputStream(String format,
                                               String location,
                                               String file) throws
            IOException {
        if (format.equals(ZIP_FILE)) {
            System.out.println("WARNING: overwriting ZIP or JAR file!");
            return StreamFactory.getOutputStream
                    (StreamFactory.resolve(location), location, file, false);
        } else if (format.equals(DIRECTORY)) {
            return StreamFactory.getOutputStream(StreamFactory.DIRECTORY,
                    location, file, false);
        } else {
            throw new IOException("Format not supported for writing");
        }
    }


    /**
     * Returns the type of the given data source. The current supported types are: <code> StreamFactory.ZIP_FILE
     * StreamFactory.DIRECTORY </code>
     */
    public static String resolve(String sourceName) {
        if ((sourceName.endsWith(".jar")) || (sourceName.endsWith(".zip"))) {
            return StreamFactory.ZIP_FILE;
        } else {
            return StreamFactory.DIRECTORY;
        }
    }
}



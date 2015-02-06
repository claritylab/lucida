/*
 * Copyright 2004 Carnegie Mellon University.  
 * Portions Copyright 2004 Sun Microsystems, Inc.  
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.util;

import java.io.*;
import java.net.InetAddress;
import java.nio.channels.FileLock;
import java.util.List;

/** A simple implementation of the batch manager suitable for single threaded batch processing */
public class PooledBatchManager implements BatchManager {

    private final String batchFile;
    private final int skip;
    private File processingFile;

    private final static File topDir = new File("tests");
    private final static File inputDir = new File(topDir, "ToRun");
    private final static File inProcessDir = new File(topDir, "InProcess");
    private final static File completedDir = new File(topDir, "Completed");
    private final static File resultsDir = new File(topDir, "Results");
    private final static File lockFile = new File(".lock");
    private FileLock lock;
    private PrintStream oldOut;
    private final FileFilter testFileFilter = new TestFileFilter();


    /**
     * Creates a pooled batch manager
     *
     * @param filename the name of the batch file
     * @param skip     items to skip between runs
     */

    public PooledBatchManager(String filename, int skip) {
        this.batchFile = filename;
        this.skip = skip;
    }


    /** Starts processing the batch */
    public void start() throws IOException {

        // redirect standard out to a file
        lock();
        try {
            createDirectories();
            redirectStdout();
        } finally {
            unlock();
        }
    }


    /**
     * Gets the next available batch item or null if no more are available
     *
     * @return the next available batch item
     * @throws IOException if an I/O error occurs while reading the next item from the batch file.
     */
    public BatchItem getNextItem() throws IOException {
        lock();
        try {
            // move the last 'in process' file to the 'completed'
            // tests section.

            if (processingFile != null) {
                File completedFile = getCompletedFile(processingFile);
                processingFile.renameTo(completedFile);
                processingFile = null;
            }
            File testFile = getNextFile();
            if (testFile != null) {
                processingFile = getProcessingFile(testFile);
                testFile.renameTo(processingFile);
                System.out.println("Processing: " + processingFile);
                return getBatchItem(processingFile);
            } else {
                return null;
            }
        } finally {
            unlock();
        }
    }


    /** Stops processing the batch */
    public void stop() throws IOException {
        lock();
        try {
            closeStdout();
        } finally {
            unlock();
        }
    }


    /**
     * Returns the name of the file
     *
     * @return the filename
     */
    public String getFilename() {
        return batchFile;
    }


    /** Creates the test directories as necessary */
    private void createDirectories() throws IOException {
        if (!topDir.isDirectory()) {
            topDir.mkdir();
            inProcessDir.mkdir();
            completedDir.mkdir();
            resultsDir.mkdir();
            createInputDirectory();
        }
    }


    /** Creates the input directory */
    private void createInputDirectory() throws IOException {
        inputDir.mkdir();
        // read in the batch file
        List<String> list = BatchFile.getLines(batchFile, skip);

        for (int i = 0; i < list.size(); i++) {
            String name = Integer.toString(i + 1);
            String line = list.get(i);
            createInputFile(inputDir, name, line);
        }
    }


    /**
     * Creates the individual batch files
     *
     * @param dir  the directory to place the input file in
     * @param name the name of the file
     * @param line the contents of the file
     */
    private void createInputFile(File dir, String name, String line)
            throws IOException {

        File path = new File(dir, name);
        FileOutputStream fos = new FileOutputStream(path);
        PrintStream ps = new PrintStream(fos);
        ps.println(line);
        ps.close();
    }


    /** Redirects standard out to a file in the results directory with a name 'Results_xxx.out' */
    private void redirectStdout() throws IOException {
        String myName = getMyName();
        File resultFile = File.createTempFile(myName, ".out", resultsDir);

        FileOutputStream fos = new FileOutputStream(resultFile);
        PrintStream ps = new PrintStream(fos);
        oldOut = System.out;
        System.setOut(ps);

        System.out.println("# These results collected on " + getMyName());
    }


    /**
     * Gets my network name
     *
     * @return my network name
     */
    private String getMyName() throws IOException {
        return InetAddress.getLocalHost().getHostName();
    }


    /** Close the redirected stdout and restore it to what it was before we redirected it. */
    private void closeStdout() throws IOException {
        System.out.close();
        System.setOut(oldOut);
    }


    /** Lock the test suite so we can manipulate the set of tests */
    private void lock() throws IOException {
        RandomAccessFile raf = new RandomAccessFile(lockFile, "rw");
        lock = raf.getChannel().lock();
        raf.close();
    }


    /** unlock the test suite so we can manipulate the set of tests */
    private void unlock() throws IOException {
        lock.release();
        lock = null;
    }


    /**
     * Given an 'in process' file, generate the corresponding completed file.
     *
     * @param file the in process file
     * @return the completed file
     */
    private File getCompletedFile(File file) {
        return new File(completedDir, file.getName());
    }


    /**
     * Given an 'input' file, generate the corresponding inProcess file.
     *
     * @param file the in process file
     * @return the completed file
     */
    private File getProcessingFile(File file) {
        return new File(inProcessDir, file.getName());
    }


    /** returns the next batch item file in the input directory */
    private File getNextFile() throws IOException {
        File[] match = inputDir.listFiles(testFileFilter);

        if (match.length > 0) {
            return match[0];
        }
        return null;
    }


    /**
     * Given a file parse the contents of the file into a BatchItem
     *
     * @param file the file to parse
     * @return the contents in the form of a batch item
     */
    private BatchItem getBatchItem(File file) throws IOException {
        List<String> list = BatchFile.getLines(file.getPath());
        if (list.size() != 1) {
            throw new IOException("Bad batch file size");
        }
        String line = list.get(0);
        return new BatchItem(BatchFile.getFilename(line),
                BatchFile.getReference(line));
    }
}

/** Filter that only yields filenames that are integer numbers */
class TestFileFilter implements FileFilter {

    public boolean accept(File pathname) {
        String name = pathname.getName();
        try {
            Integer.parseInt(name);
            return true;
        } catch (NumberFormatException nfe) {
            return false;
        }
    }
}


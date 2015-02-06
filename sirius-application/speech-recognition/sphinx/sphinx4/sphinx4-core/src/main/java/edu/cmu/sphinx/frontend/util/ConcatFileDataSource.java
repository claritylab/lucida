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
package edu.cmu.sphinx.frontend.util;

import edu.cmu.sphinx.util.BatchFile;
import edu.cmu.sphinx.util.ReferenceSource;
import edu.cmu.sphinx.util.props.*;

import java.io.*;
import java.util.Enumeration;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;


/**
 * Concatenates a list raw headerless audio files as one continuous audio stream. A {@link
 * edu.cmu.sphinx.frontend.DataStartSignal DataStartSignal} will be placed before the start of the first file, and a
 * {@link edu.cmu.sphinx.frontend.DataEndSignal DataEndSignal} after the last file. No DataStartSignal or DataEndSignal
 * will be placed between them. Optionally, silence can be added in-between the audio files by setting the property:
 * <pre>edu.cmu.sphinx.frontend.util.ConcatFileDataSource.silenceFile</pre>
 * to a audio file for silence. By default, no silence is added. Moreover, one can also specify how many files to skip
 * for every file read.
 * <p/>
 * You can also specify the name of a transcript file to write the transcription to. The transcription will be written
 * in HUB-4 style. A sample HUB-4 transcript looks like:
 * <pre>
 * bn99en_1 1 peter_jennings 0.806084 7.079850 <o,f4,male> Tonight this
 * Thursday big pressure on the Clinton administration to do something about
 * the latest killing in Yugoslavia
 * bn99en_1 1 peter_jennings 7.079850 14.007608 <o,fx,male> Airline passengers
 * and outrageous behavior at thirty thousand feet What can an airline do
 * ...
 * bn99en_1 1 inter_segment_gap 23.097000 28.647000 <o,fx,>
 * ...
 * </pre>
 * The format of each line is:
 * <pre>
 * test_set_name category speaker_name start_time_in_seconds
 * end_time_in_seconds <category,hub4_focus_conditions,speaker_sex> transcript
 * </pre>
 * In our example above,
 * <pre>
 * test_set_name is "bn99en_1"
 * category is "1"
 * speaker_name is "peter_jennings"
 * start_time_in_seconds is "0.806084"
 * end_time_in_seconds is "7.079850"
 * category is "o" for "Overall"
 * hub4_focus_conditions is:
 *     "f0" for "Baseline//Broadcast//Speech"
 *     "f1" for "Spontaneous//Broadcast//Speech"
 *     "f2" for "Speech Over//Telephone//Channels"
 *     "f3" for "Speech in the//Presence of//Background Music"
 *     "f4" for "Speech Under//Degraded//Acoustic Conditions"
 *     "f5" for "Speech from//Non-Native//Speakers"
 *     "fx" for "All other speech"
 * speaker_sex is "male"
 * transcript is "Tonight this Thursday big pressure on the Clinton
 * administration to do something about the latest killing in Yugoslavia
 * </pre>
 * The ConcatFileDataSource will produce such a transcript if the name of the file to write to is supplied in the
 * constructor. This transcript file will be used in detected gap insertion errors, because it accurately describes the
 * "correct" sequence of speech and silences in the concatenated version of the audio files.
 */
public class ConcatFileDataSource extends StreamDataSource implements ReferenceSource {

    /** The property that specifies which file to start at. */
    @S4Integer(defaultValue = 1)
    public static final String PROP_START_FILE = "startFile";

    /** The property that specifies the number of files to skip for every file read. */
    @S4Integer(defaultValue = 0)
    public static final String PROP_SKIP = "skip";

    /** The property that specifies the total number of files to read. The default value should be no limit. */
    @S4Integer(defaultValue = -1)
    public static final String PROP_TOTAL_FILES = "totalFiles";

    /**
     * The property that specifies the silence audio file, if any. If this property is null, then no silences are
     * added in between files.
     */
    @S4String
    public static final String PROP_SILENCE_FILE = "silenceFile";

    /** The property that specifies whether to add random silence. */
    @S4Boolean(defaultValue = false)
    public static final String PROP_ADD_RANDOM_SILENCE = "addRandomSilence";

    /**
     * The property that specifies the maximum number of times the silence file is added  between files. If
     * PROP_ADD_RANDOM_SILENCE is set to true, the number of times the silence file is added is between 1 and this
     * value. If PROP_ADD_RANDOM_SILENCE is set to false, this value will be the number of times the silence file is
     * added. So if PROP_MAX_SILENCE is set to 3, then the silence file will be added three times between files.
     */
    @S4Integer(defaultValue = 3)
    public static final String PROP_MAX_SILENCE = "maxSilence";

    /**
     * The property that specifies the name of the transcript file. If this property is set, a transcript file
     * will be created. No transcript file will be created if this property is not set.
     */
    @S4String
    public static final String PROP_TRANSCRIPT_FILE = "transcriptFile";

    /** The property for the file containing a list of audio files to read from. */
    @S4String
    public static final String PROP_BATCH_FILE = "batchFile";


    private static final String GAP_LABEL = "inter_segment_gap";
    private boolean addRandomSilence;
    private int skip;
    private int maxSilence;
    private int silenceCount;
    private int bytesPerSecond;
    private long totalBytes;
    private long silenceFileLength;
    private String silenceFileName;
    private String nextFile;
    private String context;
    private String transcriptFile;
    private List<String> referenceList;
    private FileWriter transcript;
    private int startFile;
    private int totalFiles;
    private String batchFile;

    public ConcatFileDataSource( int sampleRate, int bytesPerRead, int bitsPerSample, boolean bigEndian, boolean signedData,
        boolean addRandomSilence,
        int maxSilence,
        int skip,
        String silenceFileName,
        int startFile,
        int totalFiles,
        String transcriptFile,
        String batchFile) {
        super(sampleRate,bytesPerRead,bitsPerSample,bigEndian,signedData );

        this.bytesPerSecond = sampleRate * (bitsPerSample / 8);
        this.addRandomSilence = addRandomSilence;
        this.maxSilence = maxSilence;
        this.skip = skip;
        this.silenceFileName = silenceFileName;
        this.startFile = startFile;
        this.totalFiles = totalFiles;
        this.transcriptFile = transcriptFile;
        this.batchFile = batchFile;
    }

    public ConcatFileDataSource() {
        
    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        super.newProperties(ps);

        bytesPerSecond = sampleRate * (bitsPerSample / 8);
        addRandomSilence = ps.getBoolean(PROP_ADD_RANDOM_SILENCE);
        maxSilence = ps.getInt(PROP_MAX_SILENCE);
        skip = ps.getInt(PROP_SKIP);
        silenceFileName = ps.getString(PROP_SILENCE_FILE);
        startFile = ps.getInt(PROP_START_FILE);
        totalFiles = ps.getInt(PROP_TOTAL_FILES);
        transcriptFile = ps.getString(PROP_TRANSCRIPT_FILE);
        batchFile = ps.getString(PROP_BATCH_FILE);
    }


    /** Initializes a ConcatFileDataSource. */
    @Override
    public void initialize() {
        super.initialize();

        try {
            File silenceFile = new File(silenceFileName);
            silenceFileLength = silenceFile.length();

            if (transcriptFile != null) {
                transcript = new FileWriter(transcriptFile);
            }
            if (batchFile == null) {
                throw new Error("BatchFile cannot be null!");
            }
            setInputStream
                    (new SequenceInputStream
                            (new InputStreamEnumeration
                                    (batchFile, startFile, totalFiles)));
            referenceList = new LinkedList<String>();
        } catch (IOException e) {
            e.printStackTrace(); //TODO fix this
        }
    }


    /**
     * Returns a list of all reference text. Implements the getReferences() method of ReferenceSource.
     *
     * @return a list of all reference text
     */
    public List<String> getReferences() {
        return referenceList;
    }


    /**
     * Returns the name of the transcript file.
     *
     * @return the name of the transcript file
     */
    public String getTranscriptFile() {
        return transcriptFile;
    }


    /**
     * Returns the audio time in seconds represented by the given number of bytes.
     *
     * @param bytes the number of bytes
     * @return the audio time
     */
    private float getSeconds(long bytes) {
        return ((float) bytes / bytesPerSecond);
    }


    /**
     * The work of the concatenating of the audio files are done here. The idea here is to turn the list of audio files
     * into an Enumeration, and then fed it to a SequenceInputStream, giving the illusion that the audio files are
     * concatenated, but only logically.
     */
    class InputStreamEnumeration implements Enumeration<InputStream> {

        private final int totalFiles;
        private boolean inSilence;
        private Random silenceRandom;
        private BufferedReader reader;


        InputStreamEnumeration(String batchFile, int startFile,
                               int totalFiles)
                throws IOException {
            this.totalFiles = totalFiles;
            reader = new BufferedReader(new FileReader(batchFile));
            if (silenceFileName != null) {
                inSilence = true;
                silenceRandom = new Random(System.currentTimeMillis());
                silenceCount = getSilenceCount();
            }
            // go to the start file
            for (int i = 1; i < startFile; i++) {
                reader.readLine();
            }
        }


        /**
         * Tests if this enumeration contains more elements.
         *
         * @return true if and only if this enumeration object contains at least one more element to provide; false
         *         otherwise.
         */
        public boolean hasMoreElements() {
            if (nextFile == null) {
                nextFile = readNext();
            }
            return (nextFile != null);
        }


        /**
         * Returns the next element of this enumeration if this enumeration object has at least one more element to
         * provide.
         *
         * @return the next element of this enumeration.
         */
        public InputStream nextElement() {
            InputStream stream = null;
            if (nextFile == null) {
                nextFile = readNext();
            }
            if (nextFile != null) {
                try {
                    stream = new FileInputStream(nextFile);
                    // System.out.println(nextFile);
                    nextFile = null;
                } catch (IOException ioe) {
                    ioe.printStackTrace();
                    throw new Error("Cannot convert " + nextFile +
                            " to a FileInputStream");
                }
            }

            // close the transcript file no more files
            if (stream == null && transcript != null) {
                try {
                    transcript.close();
                } catch (IOException ioe) {
                    ioe.printStackTrace();
                }
            }
            return stream;
        }


        /**
         * Returns the name of next audio file, taking into account file skipping and the adding of silence.
         *
         * @return the name of the appropriate audio file
         */
        public String readNext() {
            if (!inSilence) {
                return readNextDataFile();
            } else {
                // return the silence file
                String next = null;
                if (silenceCount > 0) {
                    next = silenceFileName;
                    if (transcript != null) {
                        writeSilenceToTranscript();
                    }
                    silenceCount--;
                    if (silenceCount <= 0) {
                        inSilence = false;
                    }
                }
                return next;
            }
        }


        /**
         * Returns the next audio file.
         *
         * @return the name of the next audio file
         */
        private String readNextDataFile() {
            try {
                if (0 <= totalFiles &&
                        totalFiles <= referenceList.size()) {
                    return null;
                }
                String next = reader.readLine();
                if (next != null) {
                    String reference = BatchFile.getReference(next);
                    referenceList.add(reference);
                    next = BatchFile.getFilename(next);
                    for (int i = 1; i < skip; i++) {
                        reader.readLine();
                    }
                    if (silenceFileName != null && maxSilence > 0) {
                        silenceCount = getSilenceCount();
                        inSilence = true;
                    }
                    if (transcript != null) {
                        writeTranscript(next, reference);
                    }
                }
                return next;
            } catch (IOException ioe) {
                ioe.printStackTrace();
                throw new Error("Problem reading from batch file");
            }
        }


        /**
         * Writes the transcript file.
         *
         * @param fileName  the name of the decoded file
         * @param reference the reference text
         */
        private void writeTranscript(String fileName, String reference) {
            try {
                File file = new File(fileName);
                float start = getSeconds(totalBytes);
                totalBytes += file.length();
                float end = getSeconds(totalBytes);
                transcript.write(context + " 1 " + fileName + ' ' + start +
                    ' ' + end + "  " + reference + '\n');
                transcript.flush();
            } catch (IOException ioe) {
                ioe.printStackTrace();
            }
        }


        /** Writes silence to the transcript file. */
        private void writeSilenceToTranscript() {
            try {
                float start = getSeconds(totalBytes);
                totalBytes += silenceFileLength;
                float end = getSeconds(totalBytes);
                transcript.write(context + " 1 " + GAP_LABEL + ' ' +
                        start + ' ' + end + " \n");
                transcript.flush();
            } catch (IOException ioe) {
                ioe.printStackTrace();
            }
        }


        /**
         * Returns how many times the silence file should be added between utterances.
         *
         * @return the number of times the silence file should be added between utterances
         */
        private int getSilenceCount() {
            if (addRandomSilence) {
                return silenceRandom.nextInt(maxSilence) + 1;
            } else {
                return maxSilence;
            }
        }
    }
}

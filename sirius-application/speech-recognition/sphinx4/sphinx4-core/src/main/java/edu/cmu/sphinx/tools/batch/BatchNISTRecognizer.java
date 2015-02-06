package edu.cmu.sphinx.tools.batch;

import edu.cmu.sphinx.decoder.search.Token;
import edu.cmu.sphinx.frontend.DataProcessor;
import edu.cmu.sphinx.frontend.util.StreamCepstrumSource;
import edu.cmu.sphinx.frontend.util.StreamDataSource;
import edu.cmu.sphinx.linguist.WordSearchState;
import edu.cmu.sphinx.linguist.dictionary.Word;
import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.util.Utilities;
import edu.cmu.sphinx.util.props.*;

import java.io.*;
import java.net.URL;
import java.util.Iterator;
import java.util.List;
import java.util.logging.Logger;

/**
 * Copyright 1999-2002 Carnegie Mellon University. Portions Copyright 2002 Sun Microsystems, Inc. Portions Copyright
 * 2002 Mitsubishi Electric Research Laboratories. All Rights Reserved.  Use is subject to license terms.
 * <p/>
 * See the file "license.terms" for information on usage and redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 * <p/>
 * User: Peter Wolf Date: Nov 10, 2005 Time: 2:42:06 PM Copyright 2005, Peter Wolf
 * <p/>
 * Runs a NIST corpus as used by the GALE project.  The inputs are a CTL file, and a REF file.  The output is a CTM
 * file.
 * <p/>
 * A CTL file contains a list of utterances to decode. The format is
 * <p/>
 * <utterance file> <start offset> <end offset> <utterance name>
 * <p/>
 * The <utterance file> is a base to which the property "dataDirectory" is prepended, and ".raw" is appended.  The
 * utterance file should be raw PCM that agrees with the "bitsPerSample", "channelCount", "samplesPerSecond", and
 * "framesPerSecond" properties.
 * <p/>
 * The <start offset> and <end offset> are specified in frames, where
 * <p/>
 * bytesPerFrame = (bitsPerSample/8)*channelCount*samplesPerSecond/framesPerSecond
 * <p/>
 * The <utterance name> should be a unique string.  For example "<utterance file>_<start offset>_<end offset>".
 * <p/>
 * A REF file contains the correct transcripts of the utterances specified in the CTL file.  Each line should be of the
 * form
 * <p/>
 * <ASCII transcript> (<utterance name>)
 * <p/>
 * The output is a "processed" CTM file.  It is used by the NIST tools to compute the performance on the copus.  The
 * format is not documented because it is currently a hack to get the Dry Run going.  We need to think more about it. If
 * you want to use this tool talk to Peter Wolf, or Arthur Chan.
 */
public class BatchNISTRecognizer extends BatchModeRecognizer {

    protected String ctlFile;
    protected String dataDir;
    protected String refFile;
    protected String ctmFile;
    protected int bitsPerSample;
    protected int samplesPerSecond;
    protected int framesPerSecond;
    protected int channelCount;
    protected int bytesPerFrame;

    /**
     * The property that specifies the file containing the corpus utterance audio
     */
    @S4String(defaultValue = "<raw data directory not set>")
    public final static String PROP_DATA_DIR = "dataDirectory";

    /**
     * The property that specifies the file containing the corpus utterance audio
     */
    @S4String(defaultValue = "<ctl file not set>")
    public final static String PROP_CTL_FILE = "ctlFile";

    /**
     * The property that specifies the file containing the transcripts of the corpus
     */
    @S4String(defaultValue = "<ref file not set>")
    public final static String PROP_REF_FILE = "refFile";

    /**
     * The property that specifies the the directory where the output XXX files should be placed
     */
    @S4String(defaultValue = "<ctm file not set>")
    public final static String PROP_CTM_FILE = "ctmFile";

    /**
     * The sphinx properties that specify the format of the PCM audio in the data file
     */
    @S4Integer(defaultValue = 16)
    public final static String PROP_BITS_PER_SAMPLE = "bitsPerSample";
    @S4Integer(defaultValue = 1)
    public final static String PROP_CHANNEL_COUNT = "channelCount";
    @S4Integer(defaultValue = 16000)
    public final static String PROP_SAMPLES_PER_SECOND = "samplesPerSecond";
    @S4Integer(defaultValue = 100)
    public final static String PROP_FRAMES_PER_SECOND = "framesPerSecond";


    public BatchNISTRecognizer(
            Recognizer recognizer,
            List<DataProcessor> inputDataProcessors,
            String ctlFile,
            String dataDir,
            String refFile,
            String ctmFile,
            int bitsPerSample,
            int samplesPerSecond,
            int framesPerSecond,
            int channelCount
    ) {
        this.logger = Logger.getLogger(getClass().getName());
        this.recognizer = recognizer;
        this.inputDataProcessors = inputDataProcessors;
        this.dataDir = dataDir;
        this.ctlFile = ctlFile;
        this.refFile = refFile;
        this.ctmFile = ctmFile;

        this.bitsPerSample = bitsPerSample;
        this.channelCount = channelCount;
        this.samplesPerSecond = samplesPerSecond;
        this.framesPerSecond = framesPerSecond;

        this.bytesPerFrame = ((bitsPerSample / 8) * channelCount * samplesPerSecond) / framesPerSecond;

        logger.info(
                "BatchNISTRecognizer:\n" +
                        "  dataDirectory=" + dataDir + '\n' +
                        "  ctlFile=" + ctlFile + '\n' +
                        "  bitsPerSample=" + bitsPerSample + '\n' +
                        "  channelCount=" + channelCount + '\n' +
                        "  samplesPerSecond=" + samplesPerSecond + '\n' +
                        "  framesPerSecond=" + framesPerSecond + '\n');
    }

    public BatchNISTRecognizer() {

    }

    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    @Override
    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();
        recognizer = (Recognizer) ps.getComponent(PROP_RECOGNIZER);
        inputDataProcessors = ps.getComponentList(PROP_INPUT_DATA_PROCESSORS, DataProcessor.class);
        dataDir = ps.getString(PROP_DATA_DIR);
        ctlFile = ps.getString(PROP_CTL_FILE);
        refFile = ps.getString(PROP_REF_FILE);
        ctmFile = ps.getString(PROP_CTM_FILE);

        bitsPerSample = ps.getInt(PROP_BITS_PER_SAMPLE);
        channelCount = ps.getInt(PROP_CHANNEL_COUNT);
        samplesPerSecond = ps.getInt(PROP_SAMPLES_PER_SECOND);
        framesPerSecond = ps.getInt(PROP_FRAMES_PER_SECOND);

        bytesPerFrame = ((bitsPerSample / 8) * channelCount * samplesPerSecond) / framesPerSecond;

        logger.info(
                "BatchNISTRecognizer:\n" +
                        "  dataDirectory=" + dataDir + '\n' +
                        "  ctlFile=" + ctlFile + '\n' +
                        "  bitsPerSample=" + bitsPerSample + '\n' +
                        "  channelCount=" + channelCount + '\n' +
                        "  samplesPerSecond=" + samplesPerSecond + '\n' +
                        "  framesPerSecond=" + framesPerSecond + '\n');
    }


    @SuppressWarnings("serial")
    protected class CTLException extends Exception {
        CTLException(String msg) {
            super(msg);
        }
    }

    public class CTLUtterance {

        int startOffset;
        int endOffset;
        String name;
        byte[] data;
        final String ref;


        public String getFile() {
            return file;
        }


        String file;


        CTLUtterance(String ctl, String ref) throws CTLException {
            /*
                example line:
                20040422_150000_NTDTV.80Hz-6400Hz 64155 65103 20040422_150000_NTDTV_64155-65103_spk8
            */
            this.ref = ref;
            String[] fields = ctl.split(" ");
            if (fields.length != 4) throw new CTLException("CTL Syntax Error: " + ctl);
            startOffset = Integer.parseInt(fields[1]);
            endOffset = Integer.parseInt(fields[2]);
            name = fields[3];
            data = new byte[(endOffset - startOffset) * bytesPerFrame];
            int i = fields[0].indexOf('.');
            file = fields[0];
            if (i >= 0) {
                file = file.substring(0, i);
            }
            file = dataDir + '/' + file + ".raw";
            try {
                InputStream dataStream = new FileInputStream(file);
                dataStream.skip(startOffset * bytesPerFrame);
                if (dataStream.read(data) != data.length) {
                    dataStream.close();
                    throw new CTLException("Unable to read " + data.length + " bytes of utterance " + name);
                }
                dataStream.close();
            }
            catch (IOException e) {
                throw new CTLException("Unable to read utterance " + name + ": " + e.getMessage());
            }
        }


        public InputStream getInputStream() {
            return new ByteArrayInputStream(data);
        }


        public String getName() {
            return name;
        }


        public String getRef() {
            return ref;
        }


        public int getStartOffset() {
            return startOffset;
        }


        public int getEndOffset() {
            return endOffset;
        }
    }

    protected class CTLIterator implements Iterator<CTLUtterance> {

        CTLUtterance utterance;
        LineNumberReader ctlReader;
        LineNumberReader refReader;


        public CTLIterator() throws IOException {
            ctlReader = new LineNumberReader(new FileReader(ctlFile));
            refReader = new LineNumberReader(new FileReader(refFile));
            utterance = nextUtterance();
        }


        private CTLUtterance nextUtterance() {
            try {
                String ctl = ctlReader.readLine();
                String ref = refReader.readLine();
                if (ctl == null || ref == null)
                    return null;
                else
                    return new CTLUtterance(ctl, ref);
            } catch (Exception e) {
                throw new Error(e.getMessage());
            }
        }


        public boolean hasNext() {
            return utterance != null;
        }


        public CTLUtterance next() {
            CTLUtterance u = utterance;
            utterance = nextUtterance();
            return u;
        }


        public void remove() {
            throw new Error("Not implemented");
        }
    }


    protected void setInputStream(CTLUtterance utt) throws IOException {
        for (DataProcessor dataSource : inputDataProcessors) {
            if (dataSource instanceof StreamDataSource) {
                ((StreamDataSource)
                 dataSource).setInputStream(utt.getInputStream());
            } else if (dataSource instanceof StreamCepstrumSource) {
                boolean isBigEndian = Utilities
                        .isCepstraFileBigEndian(utt.getName());
                StreamCepstrumSource cepstrumSource =
                        (StreamCepstrumSource) dataSource;
                cepstrumSource.setInputStream(utt.getInputStream(), isBigEndian);
            }
        }
    }


    public void decode() {

        try {
            utteranceId = 0;
            DataOutputStream ctm = new DataOutputStream(new FileOutputStream(ctmFile));
            recognizer.allocate();

            for (Iterator<CTLUtterance> i = new CTLIterator(); i.hasNext();) {
                CTLUtterance utt = i.next();
                setInputStream(utt);
                Result result = recognizer.recognize();
                System.out.println("Utterance " + utteranceId + ": " + utt.getName());
                System.out.println("Reference: " + utt.getRef());
                System.out.println("Result   : " + result);
                logger.info("Utterance " + utteranceId + ": " + utt.getName());
                logger.info("Result   : " + result);
                handleResult(ctm, utt, result);
                utteranceId++;
            }

            recognizer.deallocate();
        } catch (IOException io) {
            logger.severe("I/O error during decoding: " + io.getMessage());
        }
        logger.info("BatchCTLDecoder: " + utteranceId + " utterances decoded");
    }


    protected void handleResult(DataOutputStream out, CTLUtterance utt, Result result) throws IOException {
        dumpBestPath(out, utt, result.getBestFinalToken());
    }


    private int dumpBestPath(DataOutputStream out, CTLUtterance utt, Token token) throws IOException {

        if (token == null) return 0;

        Token pred = token.getPredecessor();
        int startFrame = dumpBestPath(out, utt, pred);
        if (token.isWord()) {

            int endFrame = token.getFrameNumber();

            WordSearchState wordState = (WordSearchState) token.getSearchState();
            Word word = wordState.getPronunciation().getWord();
            String spelling = word.getSpelling();
            if (!spelling.startsWith("<")) {
                String[] names = utt.name.split("_");
                out.write((names[0] + '_' + names[1] + '_' + names[2]
                        + " 1 " + (utt.startOffset + startFrame) / 100.0 + ' ' + (endFrame - startFrame) / 100.0 + ' ').getBytes());
                out.write(hex2Binary(spelling));
                out.write(" 0.700000\n".getBytes());
            }
            return endFrame;
        }
        return startFrame;
    }


    static public byte[] hex2Binary(String spelling) {
        byte[] bin = new byte[spelling.length() / 2];
        for (int i = 0; i < spelling.length(); i += 2) {
            int i0 = hexToByte(spelling.charAt(i));
            int i1 = hexToByte(spelling.charAt(i + 1));
            bin[i / 2] = (byte) (i1 + (16 * i0));
        }
        return bin;
    }


    static private int hexToByte(char c) {
        switch (c) {
            case '0':
                return 0;
            case '1':
                return 1;
            case '2':
                return 2;
            case '3':
                return 3;
            case '4':
                return 4;
            case '5':
                return 5;
            case '6':
                return 6;
            case '7':
                return 7;
            case '8':
                return 8;
            case '9':
                return 9;
            case 'a':
                return 10;
            case 'b':
                return 11;
            case 'c':
                return 12;
            case 'd':
                return 13;
            case 'e':
                return 14;
            case 'f':
                return 15;
            default:
                throw new Error("Bad hex char " + c);
        }

    }


    public static void main(String[] argv) {

        if (argv.length != 1) {
            System.out.println(
                    "Usage: BatchNISTRecognizer propertiesFile");
            System.exit(1);
        }

        String propertiesFile = argv[0];

        ConfigurationManager cm;

        BatchNISTRecognizer bmr;

        try {
            URL url = new File(propertiesFile).toURI().toURL();
            cm = new ConfigurationManager(url);
            bmr = (BatchNISTRecognizer) cm.lookup("batchNIST");
        } catch (IOException ioe) {
            System.err.println("I/O error during initialization: \n   " + ioe);
            return;
        } catch (PropertyException e) {
            System.err.println("Error during initialization: \n  " + e);
            return;
        }

        if (bmr == null) {
            System.err.println("Can't find batchNIST in " + propertiesFile);
            return;
        }

        bmr.decode();
    }
}

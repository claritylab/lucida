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
package edu.cmu.sphinx.tools.batch;

import edu.cmu.sphinx.frontend.BaseDataProcessor;
import edu.cmu.sphinx.frontend.DataProcessor;
import edu.cmu.sphinx.frontend.util.StreamCepstrumSource;
import edu.cmu.sphinx.frontend.util.StreamDataSource;
import edu.cmu.sphinx.frontend.util.StreamHTKCepstrum;
import edu.cmu.sphinx.recognizer.Recognizer;
import edu.cmu.sphinx.recognizer.Recognizer.State;
import edu.cmu.sphinx.result.Result;
import edu.cmu.sphinx.util.*;
import edu.cmu.sphinx.util.props.*;

import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.List;
import java.util.logging.Logger;

/**
 * Decodes a batch file containing a list of files to decode. The files can be
 * either audio files or cepstral files, but defaults to audio files. The audio
 * data should be 16-bit, 16kHz, PCM-linear data. Since this classes makes use
 * of Java Sound, it supports all the audio file formats that are supported by
 * Java Sound. If the audio file does not correspond to a format supported by
 * Java Sound, it is treated as a raw audio file (i.e., one without a header).
 * Audio file formats differ in the endian order of the audio data. Therefore,
 * it is important to specify it correctly in the configuration of the <a
 * href="../../frontend/util/StreamDataSource.html">StreamDataSource</a>. Note
 * that in the ideal situation, the audio format of the data should be passed
 * into the StreamDataSource, so that no extra configuration is needed. This
 * will be fixed in future releases.
 * <p/>
 * To run this BatchModeRecognizer:
 * 
 * <pre>
 * java BatchModeRecognizer &lt;xmlConfigFile&gt; &lt;batchFile&gt;
 * </pre>
 * 
 * where <code>xmlConfigFile</code> is an XML-based configuration file and
 * <code>batchFile</code> is a file listing all the files to decode and
 * transcript of those files. For information about the configuration file,
 * refer to the document <a
 * href="../../util/props/doc-files/ConfigurationManagement.html"> Sphinx-4
 * Configuration Management</a>. For information about the batch file, refer to
 * the <a href="../../../../../../index.html#batch_files"> batch file
 * description</a>.
 * <p/>
 * This class will send recognition results to the logger if the log level is
 * set to INFO.
 */
public class BatchModeRecognizer implements Configurable {

    /**
     * The property or how many files to skip for every decode.
     */
    @S4Integer(defaultValue = 0)
    public final static String PROP_SKIP = "skip";

    /**
     * The property for how many utterances to process
     */
    @S4Integer(defaultValue = 1000000)
    public final static String PROP_COUNT = "count";

    /**
     * The property that specified which batch job is to be run.
     */
    @S4Integer(defaultValue = 0)
    public final static String PROP_WHICH_BATCH = "whichBatch";

    /**
     * The property for the total number of batch jobs the decoding run is being divided into.
     * <p/>
     * The BatchDecoder supports running a subset of a batch. This allows a test to be distributed among several
     * machines.
     */
    @S4Integer(defaultValue = 1)
    public final static String PROP_TOTAL_BATCHES = "totalBatches";

    /**
     * The property that defines whether or not the decoder should use the pooled batch manager
     */
    @S4Boolean(defaultValue = false)
    public final static String PROP_USE_POOLED_BATCH_MANAGER = "usePooledBatchManager";

    /**
     * The property that specifies the recognizer to use
     */
    @S4Component(type = Recognizer.class)
    public final static String PROP_RECOGNIZER = "recognizer";

    /**
     * The property that specifies the input source
     */
    @S4ComponentList(type = BaseDataProcessor.class)
    public final static String PROP_INPUT_DATA_PROCESSORS = "inputDataProcessors";


    // -------------------------------
    // Configuration data
    // --------------------------------
    protected String name;
    protected List<DataProcessor> inputDataProcessors;
    protected int skip;
    protected int utteranceId;
    protected int whichBatch;
    protected int totalBatches;
    protected boolean usePooledBatchManager;
    protected BatchManager batchManager;
    protected Recognizer recognizer;
    protected Logger logger;

    protected BatchItem curBatchItem;
    protected ConfigurationManager cm;

    public BatchModeRecognizer(
            Recognizer recognizer,
            List<DataProcessor> inputDataProcessors,
            int skip,
            int utteranceId,
            int whichBatch,
            int totalBatches,
            boolean usePooledBatchManager
    ) {
        logger = Logger.getLogger(getClass().getName());
        cm = null;

        this.skip = skip;
        this.utteranceId = utteranceId;
        this.whichBatch = whichBatch;
        this.totalBatches = totalBatches;
        this.usePooledBatchManager = usePooledBatchManager;

        this.recognizer = recognizer;
        this.inputDataProcessors = inputDataProcessors;
    }

    public BatchModeRecognizer() {

    }


    /*
    * (non-Javadoc)
    *
    * @see edu.cmu.sphinx.util.props.Configurable#newProperties(edu.cmu.sphinx.util.props.PropertySheet)
    */
    public void newProperties(PropertySheet ps) throws PropertyException {
        logger = ps.getLogger();
        cm = ConfigurationManagerUtils.getPropertyManager(ps);

        skip = ps.getInt(PROP_SKIP);
        utteranceId = ps.getInt(PROP_COUNT);
        if (utteranceId <= 0) {
            utteranceId = Integer.MAX_VALUE;
        }

        whichBatch = ps.getInt(PROP_WHICH_BATCH);
        totalBatches = ps.getInt(PROP_TOTAL_BATCHES);
        usePooledBatchManager = ps.getBoolean(PROP_USE_POOLED_BATCH_MANAGER);

        recognizer = (Recognizer) ps.getComponent(PROP_RECOGNIZER);
        inputDataProcessors = ps.getComponentList(PROP_INPUT_DATA_PROCESSORS, DataProcessor.class);
    }

    /**
     * Sets the batch file to use for this recognition
     *
     * @param batchFile the name of the batch file
     * @throws IOException if the file could not be opened or read.
     */
    public void setBatchFile(String batchFile) throws IOException {
        if (usePooledBatchManager) {
            batchManager = new PooledBatchManager(batchFile, skip);
        } else {
            batchManager = new SimpleBatchManager(batchFile, skip, whichBatch,
                    totalBatches);
        }
    }


    /**
     * Decodes the batch of audio files
     */
    public void decode(String batchFile) throws IOException {
        BatchItem batchItem;
        int count = 0;
        try {
            recognizer.allocate();
            setBatchFile(batchFile);

            batchManager.start();
            logger.info("BatchDecoder: decoding files in "
                    + batchManager.getFilename());

            while (count < utteranceId &&
                    (batchItem = batchManager.getNextItem()) != null) {
                setInputStream(batchItem.getFilename());
                Result result = recognizer.recognize(batchItem.getTranscript());
                logger.info("File  : " + batchItem.getFilename());
                logger.info("Result: " + result);
                count++;
            }
            batchManager.stop();
            recognizer.deallocate();
        } catch (IOException io) {
            logger.severe("I/O error during decoding: " + io.getMessage());
            throw io;
        }
        logger.info("BatchDecoder: " + count + " files decoded");
    }


    /**
     * Sets the input stream to the given filename
     *
     * @param filename the filename to set the input stream to
     * @throws IOException if an error occurs
     */
    void setInputStream(String filename) throws IOException {
        for (DataProcessor dataSource : inputDataProcessors) {
            InputStream is;
            try {
                File file = new File(filename);
                logger.info
                        (AudioSystem.getAudioFileFormat(file).toString());
                is = AudioSystem.getAudioInputStream(file);
            } catch (UnsupportedAudioFileException uafe) {
                logger.info
                        ("Reading " + filename + " as raw audio file.");
                is = new FileInputStream(filename);
		// Total hack: NIST Sphere files aren't supported by
		// javax.sound, so skip their header
		if (filename.toLowerCase().endsWith(".sph")) {
			logger.info("Skipping 1024-byte Sphere header.");
			is.skip(1024);
		}
            }
            if (dataSource instanceof StreamDataSource) {
                ((StreamDataSource) dataSource).setInputStream(is);
            } else if (dataSource instanceof StreamCepstrumSource) {
                boolean isBigEndian = Utilities
                        .isCepstraFileBigEndian(filename);
                StreamCepstrumSource cepstrumSource =
                        (StreamCepstrumSource) dataSource;
                cepstrumSource.setInputStream(is, isBigEndian);
                // TODO: christophe: should use an interface there !!
            } else if (dataSource instanceof StreamHTKCepstrum) {
                StreamHTKCepstrum cepstrumSource =
                        (StreamHTKCepstrum) dataSource;
                cepstrumSource.setInputStream(is);
            }
        }
    }


    /**
     * Add commands to the given interpreter to support shell mode
     *
     * @param ci the interpreter
     */
    void addCommands(CommandInterpreter ci) {
        ci.add("ls", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                if (args.length != 1) {
                    ci.putResponse("Usage: ls");
                } else {
                    for (String name : cm.getInstanceNames(Configurable.class))
                        ci.putResponse(name);
                }
                return "";
            }


            public String getHelp() {
                return "list active components";
            }
        });
        ci.add("show", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                if (args.length < 2) {
                    ConfigurationManagerUtils.showConfig(cm);
                } else {
                    for (int i = 1; i < args.length; i++) {
                        String name = args[i];
                        ConfigurationManagerUtils.showConfig(cm, name);
                    }
                }
                return "";
            }


            public String getHelp() {
                return "show component configuration";
            }
        });
        ci.add("edit", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                if (args.length != 2) {
                    ci.putResponse("Usage: edit component");
                } else {
                    ConfigurationManagerUtils.editConfig(cm, args[1]);
//                    cm.editConfig(args[1]);
                }
                return "";
            }


            public String getHelp() {
                return "edit a  component's configuration";
            }
        });
        ci.add("save", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                if (args.length != 2) {
                    ci.putResponse("Usage: save filename.xml");
                } else {
                    ConfigurationManagerUtils.save(cm, new File(args[1]));
//                    cm.save(new File(args[1]));
                }
                return "";
            }


            public String getHelp() {
                return "save configuration to a file";
            }
        });
        ci.add("set", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                if (args.length != 4) {
                    ci.putResponse("Usage: set component property value");
                } else {
//                    System.err.println("tried to configure the CM with " + args );
                    ConfigurationManagerUtils.setProperty(BatchModeRecognizer.this.cm, args[1], args[3], args[2]);
                }
                return "";
            }


            public String getHelp() {
                return "set component property to a given value";
            }
        });
        ci.add("recognize", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                Result result = null;

                if (args.length < 2) {
                    ci.putResponse("Usage: recognize audio [transcript]");
                } else {
                    String audioFile = args[1];
                    String transcript = null;
                    if (args.length > 2) {
                        transcript = args[2];
                    }

                    try {
                        setInputStream(audioFile);
                        result = recognizer.recognize(transcript);
                    } catch (IOException io) {
                        ci.putResponse("I/O error during decoding: " +
                                io.getMessage());
                    }
                }

                return result != null ? result.getBestResultNoFiller() : "";
            }


            public String getHelp() {
                return "perform recognition on the given audio";
            }
        });
        ci.addAlias("recognize", "rec");

        ci.add("statsReset", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                if (args.length != 1) {
                    ci.putResponse("Usage: statsReset");
                } else {
                    recognizer.resetMonitors();
                }
                return "";
            }


            public String getHelp() {
                return "resets gathered statistics";
            }
        });

        ci.add("batchRecognize", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                Result result = null;

                if (args.length != 1) {
                    ci.putResponse("Usage: batchRecognize");
                } else {

                    try {
                        if (curBatchItem == null) {
                            batchManager.start();
                            curBatchItem = batchManager.getNextItem();
                        }
                        String audioFile = curBatchItem.getFilename();
                        String transcript = curBatchItem.getTranscript();
                        setInputStream(audioFile);
                        result = recognizer.recognize(transcript);
                    } catch (IOException io) {
                        ci.putResponse("I/O error during decoding: " +
                                io.getMessage());
                    }
                }
                return result != null ? result.getBestResultNoFiller() : "";
            }


            public String getHelp() {
                return "perform recognition on the current batch item";
            }
        });
        ci.addAlias("batchRecognize", "br");

        ci.add("batchNext", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                Result result = null;

                if (args.length != 1 && args.length != 2) {
                    ci.putResponse("Usage: batchNext [norec]");
                } else {
                    try {

                        // if we don't have a batch item, start (or
                        // start over)

                        if (curBatchItem == null) {
                            batchManager.start();
                        }
                        curBatchItem = batchManager.getNextItem();

                        // if we reach the end, just loop back and
                        // start over.

                        if (curBatchItem == null) {
                            batchManager.start();
                            curBatchItem = batchManager.getNextItem();
                        }

                        String audioFile = curBatchItem.getFilename();
                        String transcript = curBatchItem.getTranscript();
                        if (args.length == 2) {
                            ci.putResponse("Skipping: " + transcript);
                        } else {
                            setInputStream(audioFile);
                            result = recognizer.recognize(transcript);
                        }
                    } catch (IOException io) {
                        ci.putResponse("I/O error during decoding: " +
                                io.getMessage());
                    }
                }
                return result != null ? result.getBestResultNoFiller() : "";
            }


            public String getHelp() {
                return "advance the batch and perform recognition";
            }
        });
        ci.addAlias("batchNext", "bn");

        ci.add("batchAll", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                Result result = null;

                if (args.length != 1) {
                    ci.putResponse("Usage: batchAll");
                } else {
                    try {
                        if (curBatchItem == null) {
                            batchManager.start();
                        }

                        while (true) {
                            curBatchItem = batchManager.getNextItem();
                            // if we reach the end  bail out

                            if (curBatchItem == null) {
                                return "";
                            }
                            String audioFile = curBatchItem.getFilename();
                            String transcript = curBatchItem.getTranscript();
                            setInputStream(audioFile);
                            result = recognizer.recognize(transcript);
                        }
                    } catch (IOException io) {
                        ci.putResponse("I/O error during decoding: " +
                                io.getMessage());
                    }
                }
                return result != null ? result.getBestResultNoFiller() : "";
            }


            public String getHelp() {
                return "recognize all of the remaining batch items";
            }
        });

        ci.add("batchReset", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                if (args.length != 1) {
                    ci.putResponse("Usage: batchReset");
                } else {
                    try {
                        batchManager.start();
                    } catch (IOException ioe) {
                        ci.putResponse("trouble reseting batch");
                    }
                }
                return "";
            }


            public String getHelp() {
                return "reset the batch to the beginning";
            }
        });
        ci.add("batchLoad", new CommandInterface() {
            public String execute(CommandInterpreter ci, String[] args) {
                if (args.length != 2) {
                    ci.putResponse("Usage: batchReset batchfile");
                } else {
                    try {
                        setBatchFile(args[1]);
                    } catch (IOException ioe) {
                        ci.putResponse("Can't load " + args[1] + ' ' + ioe);
                    }
                }
                return "";
            }


            public String getHelp() {
                return "reset the batch to the beginning";
            }
        });
    }


    public void shell(String batchfile) throws IOException {
        try {
            CommandInterpreter ci = new CommandInterpreter();
            ci.setPrompt("s4> ");
            addCommands(ci);
            setBatchFile(batchfile);
            recognizer.allocate();
            ci.run();
            batchManager.stop();
            if (recognizer.getState() == State.READY) {
                recognizer.deallocate();
            }
        } catch (IOException io) {
            logger.severe("I/O error during decoding: " + io.getMessage());
            throw io;
        }
    }


    /**
     * Main method of this BatchDecoder.
     *
     * @param argv argv[0] : config.xml argv[1] : a file listing all the audio files to decode
     */
    public static void main(String[] argv) {
        if (argv.length < 2) {
            System.out.println(
                    "Usage: BatchDecoder propertiesFile batchFile [-shell]");
            System.exit(1);
        }
        String cmFile = argv[0];
        String batchFile = argv[1];
        ConfigurationManager cm;
        BatchModeRecognizer bmr;

        try {
            URL url = new File(cmFile).toURI().toURL();
            cm = new ConfigurationManager(url);
            bmr = (BatchModeRecognizer) cm.lookup("batch");
            if (bmr == null) {
                System.err.println("Can't find batchModeRecognizer in " + cmFile);
                return;
            }
            if (argv.length >= 3 && argv[2].equals("-shell")) {
                bmr.shell(batchFile);
            } else {
                bmr.decode(batchFile);
            }
            /*
           } catch (IOException ioe) {
               System.err.println("I/O error: \n");
           ioe.printStackTrace();
           } catch (InstantiationException e) {
               System.err.println("Error during initialization: \n");
           e.printStackTrace();
           } catch (PropertyException e) {
               System.err.println("Error during initialization: \n");
           e.printStackTrace();
           */
        } catch (Exception e) {
            System.err.println("Error during decoding: \n  ");
            e.printStackTrace();
        }
    }


    int count;


    public void start(String batchFile) throws IOException {
        recognizer.allocate();
        setBatchFile(batchFile);
        batchManager.start();
        logger.info("BatchDecoder: decoding files in "
                + batchManager.getFilename());
        count = 0;
    }


    public void stop() throws IOException {
        batchManager.stop();
        recognizer.deallocate();
    }


    public Result recognize() throws IOException {
        Result result = null;
        BatchItem batchItem;
        if (count < utteranceId &&
                (batchItem = batchManager.getNextItem()) != null) {
            setInputStream(batchItem.getFilename());
            result = recognizer.recognize(batchItem.getTranscript());
            logger.info("File  : " + batchItem.getFilename());
            logger.info("Result: " + result);
            count++;
        }
        logger.info("BatchDecoder: " + count + " files decoded");
        return result;
    }
}

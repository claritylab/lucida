/*
 * Copyright 1999-2013 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */

package edu.cmu.sphinx.tools.aligner;

import java.io.File;
import java.io.IOException;

import java.util.ArrayList;
import java.util.List;

import javax.sound.sampled.AudioFileFormat;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.UnsupportedAudioFileException;

import edu.cmu.sphinx.api.Configuration;
import edu.cmu.sphinx.api.SpeechAligner;

import edu.cmu.sphinx.util.TimeFrame;

import edu.cmu.sphinx.result.WordResult;

/**
 * This is a simple tool to align audio to text and dump a database
 * for the training/evaluation.
 *
 * You need to provide a model, dictionary, audio and the text to align.
 */
public class Aligner {

    private static int MIN_FILLER_LENGTH = 200;

    /**
     * @param args acoustic model, dictionary, audio file, text
     */
    public static void main(String args[]) throws Exception {
        Configuration configuration = new Configuration();
        configuration.setAcousticModelPath(args[0]);
        configuration.setDictionaryPath(args[1]);

        File file = new File(args[2]);
        SpeechAligner aligner = new SpeechAligner(configuration);
        splitStream(file, aligner.align(file.toURI().toURL(), args[3]));
    }

    private static void splitStream(File inFile, List<WordResult> results)
        throws UnsupportedAudioFileException, IOException
    {
        System.err.println(results.size());

        List<List<WordResult>> utts = new ArrayList<List<WordResult>>();
        List<WordResult> currentUtt = null;
        int fillerLength = 0;

        for (WordResult result : results) {
            if (result.isFiller()) {
                fillerLength += result.getTimeFrame().length(); 
                if (fillerLength > MIN_FILLER_LENGTH) {
                    if (currentUtt != null)
                        utts.add(currentUtt);

                    currentUtt = null;
                }
            } else {
                fillerLength = 0;
                if (currentUtt == null)
                    currentUtt = new ArrayList<WordResult>();

                currentUtt.add(result);
            }
        }

        if (null != currentUtt)
            utts.add(currentUtt);

        int count = 0;
        for (List<WordResult> utt : utts) {
            long startFrame = Long.MAX_VALUE;
            long endFrame = Long.MIN_VALUE;

            for (WordResult result : utt) {
                TimeFrame frame = result.getTimeFrame();
                startFrame = Math.min(startFrame, frame.getStart());
                endFrame = Math.max(endFrame, frame.getEnd());
                System.out.print(result.getPronunciation().getWord());
                System.out.print(' ');
            }

            String[] basename = inFile.getName().split("\\.wav$");
            String uttId = String.format("%03d0", count);
            String outPath = String.format("%s-%s.wav", basename[0], uttId); 
            System.out.println("(" + uttId + ")");
            count++;

            dumpStreamChunk(inFile, outPath, startFrame - MIN_FILLER_LENGTH,
                            endFrame - startFrame + MIN_FILLER_LENGTH);
        }
    }

    private static void dumpStreamChunk(File file, String dstPath,
                                        long offset, long length)
        throws UnsupportedAudioFileException, IOException
    {
        AudioFileFormat fileFormat = AudioSystem.getAudioFileFormat(file);
        AudioInputStream inputStream = AudioSystem.getAudioInputStream(file);
        AudioFormat audioFormat = fileFormat.getFormat();
        int bitrate = Math.round(audioFormat.getFrameSize() *
                audioFormat.getFrameRate() / 1000);

        inputStream.skip(offset * bitrate);
        AudioInputStream chunkStream =
            new AudioInputStream(inputStream, audioFormat, length * bitrate);
        AudioSystem.write(chunkStream, fileFormat.getType(), new File(dstPath));
        inputStream.close();
        chunkStream.close();
    }
}

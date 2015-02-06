/*
 * Copyright 1999-2002 Carnegie Mellon University.  
 * Portions Copyright 2002 Sun Microsystems, Inc.  
 * Portions Copyright 2010 PC-NG Inc.
 * 
 * All Rights Reserved.  Use is subject to license terms.
 * 
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL 
 * WARRANTIES.
 *
 */

package edu.cmu.sphinx.linguist.language.ngram.large;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;

/**
 * Language model that reads whole model into memory. Useful
 * for loading language models from resources or external locations.
 * 
 * @author Nikolay Shmyrev
 */
public class BinaryStreamLoader extends BinaryLoader {

    byte[] modelData;
    
    public BinaryStreamLoader(URL location, String format, boolean
            applyLanguageWeightAndWip,
            float languageWeight, double wip,
            float unigramWeight)
        throws IOException
    {
        super(format, applyLanguageWeightAndWip, languageWeight, wip,
                unigramWeight);
        
        InputStream stream = location.openStream();
        loadModelLayout(stream);
        
        stream = location.openStream();
        loadModelData(stream);
    }

    
    /**
     * Reads whole data into memory
     * 
     * @param stream  the stream to load model from
     * @throws IOException 
     */
    private void loadModelData(InputStream stream) throws IOException {
        DataInputStream dataStream = new DataInputStream (new BufferedInputStream (stream));
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        byte[] buffer = new byte[4096];
        while (true) {
            if (dataStream.read(buffer) < 0)
                break;
            bytes.write(buffer);
        }
        modelData = bytes.toByteArray();
    }

    @Override
    public byte[] loadBuffer(long position, int size) throws IOException {
        byte[] result = new byte[size];
        System.arraycopy(modelData, (int)position, result, 0, size);
        return result;
    }
}

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
import java.util.ArrayList;
import java.util.List;


/** A class that provides a mechanism for tokenizing a stream */
public class ExtendedStreamTokenizer {

    private String path;
    private final StreamTokenizer st;
    private final Reader reader;
    private boolean atEOF;
    private final List<String> putbackList;


    /**
     * Creates and returns a stream tokenizer that has been properly configured to parse sphinx3 data This
     * ExtendedStreamTokenizer has no comment characters.
     *
     * @param path the source of the data
     * @throws FileNotFoundException if a file cannot be found
     */
    public ExtendedStreamTokenizer(String path) throws FileNotFoundException {
        this(path, false);
    }


    /**
     * Creates and returns a stream tokenizer that has been properly configured to parse sphinx3 data This
     * ExtendedStreamTokenizer has no comment characters.
     *
     * @param path             the source of the data
     * @param eolIsSignificant if true eol is significant
     * @throws FileNotFoundException if a file cannot be found
     */
    public ExtendedStreamTokenizer(String path, boolean eolIsSignificant)
            throws FileNotFoundException {
        this(new FileReader(path), eolIsSignificant);
        this.path = path;
    }


    /**
     * Constructs an ExtendedStreamTokenizer from the given InputStream
     *
     * @param inputStream      the source of the data
     * @param commentChar      the comment character
     * @param eolIsSignificant true if EOL is significant, false otherwise
     */
    public ExtendedStreamTokenizer(InputStream inputStream, int commentChar,
                                   boolean eolIsSignificant) {
        this(new InputStreamReader(inputStream), eolIsSignificant);
        commentChar(commentChar);
    }


    /**
     * Constructs an ExtendedStreamTokenizer from the given InputStream. This ExtendedStreamTokenizer has no comment
     * characters.
     *
     * @param inputStream      the source of the data
     * @param eolIsSignificant true if EOL is significant, false otherwise
     */
    public ExtendedStreamTokenizer(InputStream inputStream,
                                   boolean eolIsSignificant) {
        this(new InputStreamReader(inputStream), eolIsSignificant);
    }


    /**
     * Constructs an ExtendedStreamTokenizer from the given Reader. This ExtendedStreamTokenizer has no comment
     * characters.
     *
     * @param reader           the source of the data
     * @param eolIsSignificant true if eol is significant
     */
    public ExtendedStreamTokenizer(Reader reader, boolean eolIsSignificant) {
        this.reader = new BufferedReader(reader);

        st = new StreamTokenizer(reader);
        st.resetSyntax();
        st.whitespaceChars(0, 32);
        st.wordChars(33, 255);
        st.eolIsSignificant(eolIsSignificant);
        putbackList = new ArrayList<String>();
    }


    /**
     * Closes the tokenizer
     *
     * @throws IOException if an error occurs while closing the stream
     */
    public void close() throws IOException {
        reader.close();
    }


    /**
     * Specifies that all the characters between low and hi incluseive are whitespace characters
     *
     * @param low the low end of the range
     * @param hi  the high end of the range
     */
    public void whitespaceChars(int low, int hi) {
        st.whitespaceChars(low, hi);
    }


    /**
     * Specified that the character argument starts a single-line comment. All characters from the comment character to
     * the end of the line are ignored by this stream tokenizer.
     *
     * @param ch the comment character
     */
    public void commentChar(int ch) {
        st.commentChar(ch);
    }


    /**
     * Gets the next word from the tokenizer
     *
     * @return the next word
     * @throws StreamCorruptedException if the word does not match
     * @throws IOException              if an error occurs while loading the data
     */
    public String getString() throws IOException {
        if (!putbackList.isEmpty()) {
            return putbackList.remove(putbackList.size() - 1);
        } else {
            st.nextToken();
            if (st.ttype == StreamTokenizer.TT_EOF) {
                atEOF = true;
            }
            if (st.ttype != StreamTokenizer.TT_WORD &&
                    st.ttype != StreamTokenizer.TT_EOL &&
                    st.ttype != StreamTokenizer.TT_EOF) {
                corrupt("word expected but not found");
            }
            if (st.ttype == StreamTokenizer.TT_EOL ||
                    st.ttype == StreamTokenizer.TT_EOF) {
                return null;
            } else {
                return st.sval;
            }
        }
    }


    /**
     * Puts a string back, the next get will return this string
     *
     * @param string the string to unget
     */
    public void unget(String string) {
        putbackList.add(string);
    }


    /**
     * Determines if the stream is at the end of file
     *
     * @return true if the stream is at EOF
     */
    public boolean isEOF() {
        return atEOF;
    }


    /**
     * Throws an error with the line and path added
     *
     * @param msg the annotation message
     */
    private void corrupt(String msg) throws StreamCorruptedException {
        throw new StreamCorruptedException(
                msg + " at line " + st.lineno() + " in file " + path);
    }


    /**
     * Gets the current line number
     *
     * @return the line number
     */
    public int getLineNumber() {
        return st.lineno();
    }


    /**
     * Loads a word from the tokenizer and ensures that it matches 'expecting'
     *
     * @param expecting the word read must match this
     * @throws StreamCorruptedException if the word does not match
     * @throws IOException              if an error occurs while loading the data
     */
    public void expectString(String expecting)
            throws IOException {
        String line = getString();
        if (!line.equals(expecting)) {
            corrupt("error matching expected string '" + expecting +
                    "' in line: '" + line + '\'');
        }
    }


    /**
     * Loads an integer  from the tokenizer and ensures that it matches 'expecting'
     *
     * @param name      the name of the value
     * @param expecting the word read must match this
     * @throws StreamCorruptedException if the word does not match
     * @throws IOException              if an error occurs while loading the data
     */
    public void expectInt(String name, int expecting)
            throws IOException {
        int val = getInt(name);
        if (val != expecting) {
            corrupt("Expecting integer " + expecting);
        }
    }


    /**
     * gets an integer from the tokenizer stream
     *
     * @param name the name of the parameter (for error reporting)
     * @return the next word in the stream as an integer
     * @throws StreamCorruptedException if the next value is not a
     * @throws IOException              if an error occurs while loading the data number
     */
    public int getInt(String name)
            throws IOException {
        int iVal = 0;
        try {
            String val = getString();
            iVal = Integer.parseInt(val);
        } catch (NumberFormatException nfe) {
            corrupt("while parsing int " + name);
        }
        return iVal;
    }


    /**
     * gets a double from the tokenizer stream
     *
     * @param name the name of the parameter (for error reporting)
     * @return the next word in the stream as a double
     * @throws StreamCorruptedException if the next value is not a
     * @throws IOException              if an error occurs while loading the data number
     */
    public double getDouble(String name)
            throws IOException {
        double dVal = 0.0;
        try {
            String val = getString();
            if (val.equals("inf")) {
                dVal = Double.POSITIVE_INFINITY;
            } else {
                dVal = Double.parseDouble(val);
            }
        } catch (NumberFormatException nfe) {
            corrupt("while parsing double " + name);
        }
        return dVal;
    }


    /**
     * gets a float from the tokenizer stream
     *
     * @param name the name of the parameter (for error reporting)
     * @return the next word in the stream as a float
     * @throws StreamCorruptedException if the next value is not a
     * @throws IOException              if an error occurs while loading the data number
     */
    public float getFloat(String name)
            throws IOException {
        float fVal = 0.0F;
        try {
            String val = getString();
            if (val.equals("inf")) {
                fVal = Float.POSITIVE_INFINITY;
            } else {
                fVal = Float.parseFloat(val);
            }
        } catch (NumberFormatException nfe) {
            corrupt("while parsing float " + name);
        }
        return fVal;
    }


    /**
     * gets a optional float from the tokenizer stream. If a float is not present, the default is returned
     *
     * @param name         the name of the parameter (for error reporting)
     * @param defaultValue the default value
     * @return the next word in the stream as a float
     * @throws StreamCorruptedException if the next value is not a
     * @throws IOException              if an error occurs while loading the data number
     */
    public float getFloat(String name, float defaultValue)
            throws IOException {
        float fVal = 0.0F;
        try {
            String val = getString();
            if (val == null) {
                fVal = defaultValue;
            } else if (val.equals("inf")) {
                fVal = Float.POSITIVE_INFINITY;
            } else {
                fVal = Float.parseFloat(val);
            }
        } catch (NumberFormatException nfe) {
            corrupt("while parsing float " + name);
        }
        return fVal;
    }


    /**
     * Skip any carriage returns.
     *
     * @throws IOException if an error occurs while reading data from the stream.
     */
    public void skipwhite() throws IOException {
        String next = null;

        while (!isEOF()) {
            if ((next = getString()) != null) {
                unget(next);
                break;
            }
        }
    }
}


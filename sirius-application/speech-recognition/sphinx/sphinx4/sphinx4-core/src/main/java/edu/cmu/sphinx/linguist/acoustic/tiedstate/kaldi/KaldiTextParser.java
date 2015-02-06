package edu.cmu.sphinx.linguist.acoustic.tiedstate.kaldi;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.SequenceInputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.ArrayList;
import java.util.InputMismatchException;
import java.util.List;
import java.util.Scanner;


public class KaldiTextParser {

    private final Scanner scanner;

    public KaldiTextParser(String path)
        throws IOException, MalformedURLException
    {
        // TODO: rewrite with StreamTokenizer, see ExtendedStreamTokenizer.
        File modelFile = new File(path, "final.mdl");
        InputStream modelStream = new URL(modelFile.getPath()).openStream();
        File treeFile = new File(path, "tree");
        InputStream treeStream = new URL(treeFile.getPath()).openStream();

        InputStream stream = new SequenceInputStream(modelStream, treeStream);
        scanner = new Scanner(stream);
    }

    public String getToken() {
        return scanner.next();
    }

    public int getInt() {
        return scanner.nextInt();
    }

    public float parseFloat() {
        return scanner.nextFloat();
    }

    public int[] getIntArray() {
        List<Integer> ints = new ArrayList<Integer>();
        for (String token : getTokenList("[", "]"))
            ints.add(Integer.parseInt(token));

        int[] result = new int[ints.size()];
        for (int i = 0 ; i < result.length; ++i)
            result[i] = ints.get(i);
        
        return result;
    }

    public float[] getFloatArray() {
        List<Float> floats = new ArrayList<Float>();
        for (String token : getTokenList("[", "]"))
            floats.add(Float.parseFloat(token));

        float[] result = new float[floats.size()];
        for (int i = 0; i < result.length; ++i)
            result[i] = floats.get(i);
        
        return result;
    }

    public List<String> getTokenList(String openToken, String closeToken) {
        expectToken(openToken);
        List<String> tokens = new ArrayList<String>();
        String token;

        while (!closeToken.equals(token = scanner.next()))
            tokens.add(token);

        return tokens;
    }

    public void expectToken(String expected) {
        String actual = scanner.next();
        assertToken(expected, actual);
    }

    public void assertToken(String expected, String actual) {
        if (actual.equals(expected))
            return;

        String msg;
        msg = String.format("'%s' expected, '%s' got", expected, actual);
        throw new InputMismatchException(msg);
    }
}

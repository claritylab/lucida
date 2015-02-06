/*
 * Copyright 1999-2012 Carnegie Mellon University. Portions Copyright 2002 Sun
 * Microsystems, Inc. Portions Copyright 2002 Mitsubishi Electric Research
 * Laboratories. Portions Copyright 2012 Nexiwave All Rights Reserved. Use is
 * subject to license terms. See the file "license.terms" for information on
 * usage and redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
package edu.cmu.sphinx.linguist.language.grammar;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.isOneOf;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.net.URL;

import org.testng.annotations.Test;

import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.linguist.dictionary.FastDictionary;


public class BatchForcedAlignerGrammarTest {

    @Test
    public void testForcedAlignerGrammar() throws IOException, URISyntaxException {
        URL dictionaryUrl = getClass()
                .getResource(
                             "/edu/cmu/sphinx/models/acoustic/wsj/dict/digits.dict");
        URL noisedictUrl = getClass()
                .getResource("/edu/cmu/sphinx/models/acoustic/wsj/noisedict");

        Dictionary dictionary = new FastDictionary(dictionaryUrl,
                                                   noisedictUrl,
                                                   null,
                                                   false,
                                                   null,
                                                   false,
                                                   false,
                                                   new UnitManager());

        URL url = getClass().getResource("BatchForcedAlignerGrammarTest.utts");
        BatchForcedAlignerGrammar grammar;
        grammar = new BatchForcedAlignerGrammar(new File(url.toURI()).getPath(),
                                                true,
                                                true,
                                                true,
                                                true,
                                                dictionary);
        grammar.allocate();
        assertThat(grammar.getRandomSentence(), isOneOf("one", "two", "three"));
    }
}

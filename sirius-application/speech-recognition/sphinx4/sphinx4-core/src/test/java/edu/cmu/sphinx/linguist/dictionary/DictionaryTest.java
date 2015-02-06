/*
 * Copyright 1999-2012 Carnegie Mellon University. Portions Copyright 2002 Sun
 * Microsystems, Inc. Portions Copyright 2002 Mitsubishi Electric Research
 * Laboratories. Portions Copyright 2012 Nexiwave All Rights Reserved. Use is
 * subject to license terms. See the file "license.terms" for information on
 * usage and redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
package edu.cmu.sphinx.linguist.dictionary;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.arrayWithSize;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.nullValue;

import java.io.IOException;
import java.net.URL;

import org.testng.annotations.Test;

import edu.cmu.sphinx.linguist.acoustic.UnitManager;


public class DictionaryTest {

    @Test
    public void testDictionary() throws IOException {
        URL dictUrl = getClass()
                .getResource("/edu/cmu/sphinx/models/acoustic/wsj/dict/digits.dict");
        URL noiseDictUrl = getClass()
                .getResource("/edu/cmu/sphinx/models/acoustic/wsj/noisedict");

        Dictionary dictionary = new FullDictionary(dictUrl,
                                                   noiseDictUrl,
                                                   null,
                                                   false,
                                                   null,
                                                   false,
                                                   false,
                                                   new UnitManager());
        dictionary.allocate();
        Word word = dictionary.getWord("one");

        assertThat(word.getPronunciations(), arrayWithSize(2));
        assertThat(word.getPronunciations()[0].toString(),
                   equalTo("one(HH W AH N )"));
        assertThat(word.getPronunciations()[1].toString(),
                   equalTo("one(W AH N )"));

        word = dictionary.getWord("something");
        assertThat(word, nullValue());

        assertThat(dictionary.getSentenceStartWord().getSpelling(),
                   equalTo("<s>"));
        assertThat(dictionary.getSentenceEndWord().getSpelling(),
                   equalTo("</s>"));
        assertThat(dictionary.getSilenceWord().getSpelling(), equalTo("<sil>"));

        assertThat(dictionary.getFillerWords(), arrayWithSize(12));
    }
}
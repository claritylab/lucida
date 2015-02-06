package edu.cmu.sphinx.linguist.language.grammar;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.hasSize;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.net.URL;

import org.testng.annotations.Test;

import edu.cmu.sphinx.linguist.acoustic.UnitManager;
import edu.cmu.sphinx.linguist.dictionary.Dictionary;
import edu.cmu.sphinx.linguist.dictionary.FastDictionary;


public class FSTGrammarTest {

    @Test
    public void testForcedAlignerGrammar() throws IOException, URISyntaxException {
        URL dictionaryUrl = getClass().getResource("FSTGrammarTest.dic");
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

        URL url = getClass().getResource("FSTGrammarTest.gram");
        FSTGrammar grammar = new FSTGrammar(new File(url.toURI()).getPath(),
                                            true,
                                            true,
                                            true,
                                            true,
                                            dictionary);
        grammar.allocate();
        assertThat(grammar.getGrammarNodes(), hasSize(14));
    }
}

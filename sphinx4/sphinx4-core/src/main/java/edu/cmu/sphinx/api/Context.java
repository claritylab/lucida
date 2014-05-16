/*
 * Copyright 2013 Carnegie Mellon University.
 * Portions Copyright 2004 Sun Microsystems, Inc.
 * Portions Copyright 2004 Mitsubishi Electric Research Laboratories.
 * All Rights Reserved.  Use is subject to license terms.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */

package edu.cmu.sphinx.api;

import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;

import edu.cmu.sphinx.frontend.util.StreamDataSource;

import edu.cmu.sphinx.util.props.Configurable;
import edu.cmu.sphinx.util.props.ConfigurationManager;

import static edu.cmu.sphinx.util.props.ConfigurationManagerUtils.resourceToURL;
import static edu.cmu.sphinx.util.props.ConfigurationManagerUtils.setProperty;


/**
 * Helps to tweak configuration without touching XML-file directly.
 */
public class Context {

    private final ConfigurationManager configurationManager;

    /**
     * Constructs builder that uses default XML configuration.
     *
     * @throws MalformedURLException if failed to load configuration file
     */
    public Context(Configuration config)
        throws IOException, MalformedURLException
    {
        this("resource:/edu/cmu/sphinx/api/default.config.xml", config);
    }

    /**
     * Constructs builder using user-supplied XML configuration.
     *
     * @param  path path to XML-resource with configuration
     * @return      the same instance of {@link Configuration}
     *
     * @throws MalformedURLException if failed to load configuration file
     * @throws IOException           if failed to load configuration file
     */
    public Context(String path, Configuration config)
        throws IOException, MalformedURLException
    {
        configurationManager = new ConfigurationManager(resourceToURL(path));

        setAcousticModel(config.getAcousticModelPath());
        setDictionary(config.getDictionaryPath());

        if (null != config.getGrammarPath() && config.getUseGrammar())
            setGrammar(config.getGrammarPath(), config.getGrammarName());
        if (null != config.getLanguageModelPath() && !config.getUseGrammar())
            setLanguageModel(config.getLanguageModelPath());
            
        setSampleRate(config.getSampleRate());

        // Force ConfigurationManager to build the whole graph
        // in order to enable instance lookup by class.
        configurationManager.lookup("recognizer");
    }

    /**
     * Sets acoustic model location.
     *
     * It also reads feat.params which should be located at the root of
     * acoustic model and sets corresponding parameters of
     * {@link MelFrequencyFilterBank2} instance.
     *
     * @param  path path to directory with acoustic model files
     *
     * @throws IOException if failed to read feat.params
     */
    public void setAcousticModel(String path) throws IOException {
        setLocalProperty("acousticModelLoader->location", path);
        setLocalProperty("dictionary->fillerPath", path + "/noisedict");
    }

    /**
     * Sets dictionary.
     *
     * @param path path to directory with dictionary files
     */
    public void setDictionary(String path) {
        setLocalProperty("dictionary->dictionaryPath", path);
    }

    /**
     * Sets sampleRate.
     *
     * @param sampleRate sample rate of the input stream.
     */
    public void setSampleRate(int sampleRate) {
        setLocalProperty("dataSource->sampleRate", Integer.toString(sampleRate));
    }

    /**
     * Sets path to the grammar files.
     *
     * Enables static grammar and disables probabilistic language model.
     * JSGF and GrXML formats are supported.
     *
     * @param path path to the grammar files
     * @param name name of the main grammar to use
     * @see        Context#setLanguageModel(String)
     */
    public void setGrammar(String path, String name) {
        // TODO: use a single param of type File, cache directory part
        if (name.endsWith(".grxml")) {
            setLocalProperty("grXmlGrammar->grammarLocation", path + name);
            setLocalProperty("flatLinguist->grammar", "grXmlGrammar");
        } else {
            setLocalProperty("jsgfGrammar->grammarLocation", path);
            setLocalProperty("jsgfGrammar->grammarName", name);
            setLocalProperty("flatLinguist->grammar", "jsgfGrammar");
        }
        setLocalProperty("decoder->searchManager", "simpleSearchManager");
    }

    /**
     * Sets path to the language model.
     *
     * Enables probabilistic language model and distables static grammar.
     * Currently it supports ".lm" and ".dmp" file formats.
     *
     * @param  path path to the language model file
     * @see         Context#setGrammar(String)
     *
     * @throws IllegalArgumentException if path ends with unsupported extension
     */
    public void setLanguageModel(String path) {
        if (path.endsWith(".lm")) {
            setLocalProperty("simpleNGramModel->location", path);
            setLocalProperty(
                "lexTreeLinguist->languageModel", "simpleNGramModel");
        } else if (path.endsWith(".dmp")) {
            setLocalProperty("largeTrigramModel->location", path);
            setLocalProperty(
                "lexTreeLinguist->languageModel", "largeTrigramModel");
        } else {
            throw new IllegalArgumentException(
                "Unknown format extension: " + path);
        }
        setLocalProperty("decoder->searchManager", "wordPruningSearchManager");
    }

    /**
     * Sets byte stream as the speech source.
     *
     * @param  stream stream
     * @see           Context#useMicrophone()
     */
    public void setSpeechSource(InputStream stream) {
        // TODO: setup stream sample rate and other parameters
        getInstance(StreamDataSource.class).setInputStream(stream);
        setLocalProperty("threadedScorer->frontend", "liveFrontEnd");
    }

    /**
     * Sets batch frontend to process whole file.
     *
     * @see    Context#setSpeechSource(URL)
     */
    public void processBatch() {
        setLocalProperty("threadedScorer->frontend", "batchFrontEnd");
    }

    /**
     * Sets property within a "component" tag in configuration.
     *
     * Use this method to alter "value" property of a "property" tag inside a
     * "component" tag of the XML configuration.
     *
     * @param  name  property name
     * @param  value property value
     * @see          Context#setGlobalProperty(String, Object)
     */
    public void setLocalProperty(String name, Object value) {
        setProperty(configurationManager, name, value.toString());
    }

    /**
     * Sets property of a top-level "property" tag.
     *
     * Use this method to alter "value" property of a "property" tag whose
     * parent is the root tag "config" of the XML configuration.
     *
     * @param  name  property name
     * @param  value property value
     * @see          Context#setLocalProperty(String, Object) 
     */
    public void setGlobalProperty(String name, Object value) {
        configurationManager.setGlobalProperty(name, value.toString());
    }

    /**
     * Returns instance of the XML configuration by its class.
     *
     * @param  clazz class to look up
     * @return       instance of the specified class or null
     */
    public <C extends Configurable> C getInstance(Class<C> clazz) {
        return configurationManager.lookup(clazz);
    }
}

/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2007 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 * Author: David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <gst/gst.h>

#include <sphinxbase/strfuncs.h>

#include "gstpocketsphinx.h"
#include "gstvader.h"
#include "psmarshal.h"

GST_DEBUG_CATEGORY_STATIC(pocketsphinx_debug);
#define GST_CAT_DEFAULT pocketsphinx_debug

/*
 * Forward declarations.
 */

static void gst_pocketsphinx_set_property(GObject * object, guint prop_id,
                                          const GValue * value, GParamSpec * pspec);
static void gst_pocketsphinx_get_property(GObject * object, guint prop_id,
                                          GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_pocketsphinx_chain(GstPad * pad, GstBuffer * buffer);
static gboolean gst_pocketsphinx_event(GstPad *pad, GstEvent *event);

enum
{
    SIGNAL_PARTIAL_RESULT,
    SIGNAL_RESULT,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_HMM_DIR,
    PROP_LM_FILE,
    PROP_LMCTL_FILE,
    PROP_LM_NAME,
    PROP_DICT_FILE,
    PROP_MLLR_FILE,
    PROP_FSG_FILE,
    PROP_FSG_MODEL,
    PROP_FWDFLAT,
    PROP_BESTPATH,
    PROP_MAXHMMPF,
    PROP_MAXWPF,
    PROP_BEAM,
    PROP_WBEAM,
    PROP_PBEAM,
    PROP_DSRATIO,
    PROP_LATDIR,
    PROP_LATTICE,
    PROP_NBEST,
    PROP_NBEST_SIZE,
    PROP_DECODER,
    PROP_CONFIGURED
};

/*
 * Static data.
 */

/* Default command line. (will go away soon and be constructed using properties) */
static char *default_argv[] = {
    "gst-pocketsphinx",
    "-samprate", "8000",
    "-cmn", "prior",
    "-fwdflat", "no",
    "-bestpath", "no",
    "-maxhmmpf", "2000",
    "-maxwpf", "20"
};
static const int default_argc = sizeof(default_argv)/sizeof(default_argv[0]);

static GstStaticPadTemplate sink_factory =
    GST_STATIC_PAD_TEMPLATE("sink",
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("audio/x-raw-int, "
                                            "width = (int) 16, "
                                            "depth = (int) 16, "
                                            "signed = (boolean) true, "
                                            "endianness = (int) BYTE_ORDER, "
                                            "channels = (int) 1, "
                                            "rate = (int) 8000")
        );

static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE("src",
                            GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("text/plain")
        );
static guint gst_pocketsphinx_signals[LAST_SIGNAL];

/*
 * Boxing of ps_lattice_t.
 */

GType
ps_lattice_get_type(void)
{
    static GType ps_lattice_type = 0;

    if (G_UNLIKELY(ps_lattice_type == 0)) {
        ps_lattice_type = g_boxed_type_register_static
            ("PSLattice",
             /* Conveniently, these should just work. */
             (GBoxedCopyFunc) ps_lattice_retain,
             (GBoxedFreeFunc) ps_lattice_free);
    }

    return ps_lattice_type;
}

/*
 * Boxing of ps_decoder_t.
 */

GType
ps_decoder_get_type(void)
{
    static GType ps_decoder_type = 0;

    if (G_UNLIKELY(ps_decoder_type == 0)) {
        ps_decoder_type = g_boxed_type_register_static
            ("PSDecoder",
             /* Conveniently, these should just work. */
             (GBoxedCopyFunc) ps_retain,
             (GBoxedFreeFunc) ps_free);
    }

    return ps_decoder_type;
}


/*
 * gst_pocketsphinx element.
 */
GST_BOILERPLATE (GstPocketSphinx, gst_pocketsphinx, GstElement, GST_TYPE_ELEMENT);

static void
gst_pocketsphinx_base_init(gpointer gclass)
{
    static const GstElementDetails element_details = {
        "PocketSphinx",
        "Filter/Audio",
        "Convert speech to text",
        "David Huggins-Daines <dhuggins@cs.cmu.edu>"
    };
    GstElementClass *element_class = GST_ELEMENT_CLASS(gclass);

    gst_element_class_add_pad_template(element_class,
                                       gst_static_pad_template_get(&sink_factory));
    gst_element_class_add_pad_template(element_class,
                                       gst_static_pad_template_get(&src_factory));
    gst_element_class_set_details(element_class, &element_details);
}

static void
string_disposal(gpointer key, gpointer value, gpointer user_data)
{
    g_free(value);
}

static void
gst_pocketsphinx_finalize(GObject * gobject)
{
    GstPocketSphinx *ps = GST_POCKETSPHINX(gobject);

    g_hash_table_foreach(ps->arghash, string_disposal, NULL);
    g_hash_table_destroy(ps->arghash);
    g_free(ps->last_result);
    ps_free(ps->ps);
    cmd_ln_free_r(ps->config);
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize,(gobject));
}

static void
gst_pocketsphinx_class_init(GstPocketSphinxClass * klass)
{
    GObjectClass *gobject_class;

    gobject_class =(GObjectClass *) klass;

    gobject_class->set_property = gst_pocketsphinx_set_property;
    gobject_class->get_property = gst_pocketsphinx_get_property;
    gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_pocketsphinx_finalize);

    /* TODO: We will bridge cmd_ln.h properties to GObject
     * properties here somehow eventually. */
    g_object_class_install_property
        (gobject_class, PROP_HMM_DIR,
         g_param_spec_string("hmm", "HMM Directory",
                             "Directory containing acoustic model parameters",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_LM_FILE,
         g_param_spec_string("lm", "LM File",
                             "Language model file",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_LMCTL_FILE,
         g_param_spec_string("lmctl", "LM Control File",
                             "Language model control file (for class LMs)",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_LM_NAME,
         g_param_spec_string("lmname", "LM Name",
                             "Language model name (to select LMs from lmctl)",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_FSG_FILE,
         g_param_spec_string("fsg", "FSG File",
                             "Finite state grammar file",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_FSG_MODEL,
         g_param_spec_pointer("fsg_model", "FSG Model",
                              "Finite state grammar object (fsg_model_t *)",
                              G_PARAM_WRITABLE));
    g_object_class_install_property
        (gobject_class, PROP_DICT_FILE,
         g_param_spec_string("dict", "Dictionary File",
                             "Dictionary File",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_MLLR_FILE,
         g_param_spec_string("mllr", "MLLR file",
                             "MLLR file",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_FWDFLAT,
         g_param_spec_boolean("fwdflat", "Flat Lexicon Search",
                              "Enable Flat Lexicon Search",
                              FALSE,
                              G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_BESTPATH,
         g_param_spec_boolean("bestpath", "Graph Search",
                              "Enable Graph Search",
                              FALSE,
                              G_PARAM_READWRITE));

    g_object_class_install_property
        (gobject_class, PROP_LATDIR,
         g_param_spec_string("latdir", "Lattice Directory",
                             "Output Directory for Lattices",
                             NULL,
                             G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_LATTICE,
         g_param_spec_boxed("lattice", "Word Lattice",
                            "Word lattice object for most recent result",
                            PS_LATTICE_TYPE,
                            G_PARAM_READABLE));
    g_object_class_install_property
        (gobject_class, PROP_NBEST,
         g_param_spec_value_array("nbest", "N-best results",
                          "N-best results",
                          g_param_spec_string("nbest-hyp", "N-best hyp",
                            "N-best hyp",
                            NULL,
                            G_PARAM_READABLE),
                          G_PARAM_READABLE));  
    g_object_class_install_property
        (gobject_class, PROP_NBEST_SIZE,
         g_param_spec_int("nbest_size", "Size of N-best list",
                          "Number of hypothesis in the N-best list",
                          1, 1000, 10,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_MAXHMMPF,
         g_param_spec_int("maxhmmpf", "Maximum HMMs per frame",
                          "Maximum number of HMMs searched per frame",
                          1, 100000, 1000,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_MAXWPF,
         g_param_spec_int("maxwpf", "Maximum words per frame",
                          "Maximum number of words searched per frame",
                          1, 100000, 10,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_BEAM,
         g_param_spec_float("beam", "Beam width applied to every frame in Viterbi search",
                          "Beam width applied to every frame in Viterbi search",
                          -1, 1, 1e-48,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_PBEAM,
         g_param_spec_float("pbeam", "Beam width applied to phone transitions",
                          "Beam width applied to phone transitions",
                          -1, 1, 1e-48,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_WBEAM,
         g_param_spec_float("wbeam", "Beam width applied to word exits",
                          "Beam width applied to phone transitions",
                          -1, 1, 7e-29,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_DSRATIO,
         g_param_spec_int("dsratio", "Frame downsampling ratio",
                          "Evaluate acoustic model every N frames",
                          1, 10, 1,
                          G_PARAM_READWRITE));
    g_object_class_install_property
        (gobject_class, PROP_DECODER,
         g_param_spec_boxed("decoder", "Decoder object",
                            "The underlying decoder",
                            PS_DECODER_TYPE,
                            G_PARAM_READABLE));
    g_object_class_install_property
        (gobject_class, PROP_CONFIGURED,
         g_param_spec_boolean("configured", "Finalize configuration",
                              "Set this to finalize configuration",
                              FALSE,
                              G_PARAM_READWRITE));

    gst_pocketsphinx_signals[SIGNAL_PARTIAL_RESULT] = 
        g_signal_new("partial_result",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GstPocketSphinxClass, partial_result),
                     NULL, NULL,
                     ps_marshal_VOID__STRING_STRING,
                     G_TYPE_NONE,
                     2, G_TYPE_STRING, G_TYPE_STRING
            );

    gst_pocketsphinx_signals[SIGNAL_RESULT] = 
        g_signal_new("result",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(GstPocketSphinxClass, result),
                     NULL, NULL,
                     ps_marshal_VOID__STRING_STRING,
                     G_TYPE_NONE,
                     2, G_TYPE_STRING, G_TYPE_STRING
            );

    GST_DEBUG_CATEGORY_INIT(pocketsphinx_debug, "pocketsphinx", 0,
                            "Automatic Speech Recognition");
}

static void
gst_pocketsphinx_set_string(GstPocketSphinx *ps,
                            const gchar *key, const GValue *value)
{
    gchar *oldstr, *newstr;

    if (value != NULL)
        newstr = g_strdup(g_value_get_string(value));
    else
        newstr = NULL;
    if ((oldstr = g_hash_table_lookup(ps->arghash, key)))
        g_free(oldstr);
    cmd_ln_set_str_r(ps->config, key, newstr);
    g_hash_table_foreach(ps->arghash, (gpointer)key, newstr);
}

static void
gst_pocketsphinx_set_int(GstPocketSphinx *ps,
                         const gchar *key, const GValue *value)
{
    cmd_ln_set_int32_r(ps->config, key, g_value_get_int(value));
}

static void
gst_pocketsphinx_set_boolean(GstPocketSphinx *ps,
                             const gchar *key, const GValue *value)
{
    cmd_ln_set_boolean_r(ps->config, key, g_value_get_boolean(value));
}

static void
gst_pocketsphinx_set_float(GstPocketSphinx *ps,
                         const gchar *key, const GValue *value)
{
    cmd_ln_set_float_r(ps->config, key, g_value_get_float(value));
}

static void
gst_pocketsphinx_set_property(GObject * object, guint prop_id,
                              const GValue * value, GParamSpec * pspec)
{
    GstPocketSphinx *ps = GST_POCKETSPHINX(object);

    switch (prop_id) {
    case PROP_HMM_DIR:
        gst_pocketsphinx_set_string(ps, "-hmm", value);
        if (ps->ps) {
            /* Reinitialize the decoder with the new acoustic model. */
            ps_reinit(ps->ps, NULL);
        }
        break;
    case PROP_LM_FILE:
        /* FSG and LM are mutually exclusive. */
        gst_pocketsphinx_set_string(ps, "-fsg", NULL);
        gst_pocketsphinx_set_string(ps, "-lmctl", NULL);
        gst_pocketsphinx_set_string(ps, "-lm", value);
        break;
    case PROP_LMCTL_FILE:
        /* FSG and LM are mutually exclusive. */
        gst_pocketsphinx_set_string(ps, "-fsg", NULL);
        gst_pocketsphinx_set_string(ps, "-lmctl", value);
        gst_pocketsphinx_set_string(ps, "-lm", NULL);
        break;
    case PROP_LM_NAME:
        gst_pocketsphinx_set_string(ps, "-fsg", NULL);
        gst_pocketsphinx_set_string(ps, "-lmname", value);
        break;
    case PROP_DICT_FILE:
        gst_pocketsphinx_set_string(ps, "-dict", value);
        if (ps->ps) {
            /* Reinitialize the decoder with the new dictionary. */
            ps_reinit(ps->ps, NULL);
        }
        break;
    case PROP_MLLR_FILE:
        gst_pocketsphinx_set_string(ps, "-mllr", value);
        /* Reinitialize the decoder with the new MLLR transform. */
        if (ps->ps)
            ps_reinit(ps->ps, NULL);
        break;
    case PROP_FSG_MODEL:
        {
            fsg_model_t *fsg = g_value_get_pointer(value);
            const char *name = fsg_model_name(fsg);
            ps_set_fsg(ps->ps, name, fsg);
            ps_set_search(ps->ps, name);
        }
        break;
    case PROP_FSG_FILE:
        /* FSG and LM are mutually exclusive */
        gst_pocketsphinx_set_string(ps, "-lm", NULL);
        gst_pocketsphinx_set_string(ps, "-fsg", value);
        break;
    case PROP_FWDFLAT:
        gst_pocketsphinx_set_boolean(ps, "-fwdflat", value);
        break;
    case PROP_BESTPATH:
        gst_pocketsphinx_set_boolean(ps, "-bestpath", value);
        break;
    case PROP_LATDIR:
        if (ps->latdir)
            g_free(ps->latdir);
        ps->latdir = g_strdup(g_value_get_string(value));
        break;
    case PROP_NBEST_SIZE:
	ps->n_best_size = g_value_get_int(value);
        break;
    case PROP_MAXHMMPF:
        gst_pocketsphinx_set_int(ps, "-maxhmmpf", value);
        break;
    case PROP_MAXWPF:
        gst_pocketsphinx_set_int(ps, "-maxwpf", value);
        break;
    case PROP_BEAM:
        gst_pocketsphinx_set_float(ps, "-beam", value);
        break;
    case PROP_PBEAM:
        gst_pocketsphinx_set_float(ps, "-pbeam", value);
        break;
    case PROP_WBEAM:
        gst_pocketsphinx_set_float(ps, "-wbeam", value);
        break;
    case PROP_DSRATIO:
        gst_pocketsphinx_set_int(ps, "-ds", value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        return;
    }
}

static void
gst_pocketsphinx_get_property(GObject * object, guint prop_id,
                              GValue * value, GParamSpec * pspec)
{
    GstPocketSphinx *ps = GST_POCKETSPHINX(object);

    switch (prop_id) {
    case PROP_DECODER:
        g_value_set_boxed(value, ps->ps);
        break;
    case PROP_CONFIGURED:
        g_value_set_boolean(value, ps->ps != NULL);
        break;
    case PROP_HMM_DIR:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-hmm"));
        break;
    case PROP_LM_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-lm"));
        break;
    case PROP_LMCTL_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-lmctl"));
        break;
    case PROP_LM_NAME:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-lmname"));
        break;
    case PROP_DICT_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-dict"));
        break;
    case PROP_MLLR_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-mllr"));
        break;
    case PROP_FSG_FILE:
        g_value_set_string(value, cmd_ln_str_r(ps->config, "-fsg"));
        break;
    case PROP_FWDFLAT:
        g_value_set_boolean(value, cmd_ln_boolean_r(ps->config, "-fwdflat"));
        break;
    case PROP_BESTPATH:
        g_value_set_boolean(value, cmd_ln_boolean_r(ps->config, "-bestpath"));
        break;
    case PROP_LATDIR:
        g_value_set_string(value, ps->latdir);
        break;
    case PROP_LATTICE: {
        ps_lattice_t *dag;

        if (ps->ps && (dag = ps_get_lattice(ps->ps)))
            g_value_set_boxed(value, dag);
        else
            g_value_set_boxed(value, NULL);
        break;
    }
    case PROP_MAXHMMPF:
        g_value_set_int(value, cmd_ln_int32_r(ps->config, "-maxhmmpf"));
        break;
    case PROP_MAXWPF:
        g_value_set_int(value, cmd_ln_int32_r(ps->config, "-maxwpf"));
        break;
    case PROP_BEAM:
        g_value_set_double(value, cmd_ln_float_r(ps->config, "-beam"));
        break;
    case PROP_PBEAM:
        g_value_set_double(value, cmd_ln_float_r(ps->config, "-pbeam"));
        break;
    case PROP_WBEAM:
        g_value_set_double(value, cmd_ln_float_r(ps->config, "-wbeam"));
        break;
    case PROP_DSRATIO:
        g_value_set_int(value, cmd_ln_int32_r(ps->config, "-ds"));
        break;
    case PROP_NBEST_SIZE:
        g_value_set_int(value, ps->n_best_size);
        break;
    case PROP_NBEST: {
        int i = 0, out_score = 0;
        GValueArray *arr;
        if (!ps->ps) {
            break;
        }
	arr = g_value_array_new(1);
        ps_nbest_t *ps_nbest_list = ps_nbest(ps->ps, 0, -1, NULL, NULL);   
        if (ps_nbest_list) {
            ps_nbest_list = ps_nbest_next(ps_nbest_list);
            while ((i < ps->n_best_size) && (ps_nbest_list != NULL)) {
                GValue value1 = { 0 };
                g_value_init (&value1, G_TYPE_STRING);
                const char* hyp = ps_nbest_hyp(ps_nbest_list, &out_score);
                g_value_set_string(&value1, hyp);
                g_value_array_append(arr, &value1);  
                ps_nbest_list = ps_nbest_next(ps_nbest_list);
                i++;
            }
            if (ps_nbest_list) {
                ps_nbest_free(ps_nbest_list);
            }
        }
        g_value_set_boxed (value, arr);
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_pocketsphinx_init(GstPocketSphinx * ps,
                      GstPocketSphinxClass * gclass)
{
    ps->sinkpad =
        gst_pad_new_from_static_template(&sink_factory, "sink");
    ps->srcpad =
        gst_pad_new_from_static_template(&src_factory, "src");

    /* Create the hash table to store argument strings. */
    ps->arghash = g_hash_table_new(g_str_hash, g_str_equal);

    /* Parse default command-line options. */
    ps->config = cmd_ln_parse_r(NULL, ps_args(), default_argc, default_argv, FALSE);

    /* Set up pads. */
    gst_element_add_pad(GST_ELEMENT(ps), ps->sinkpad);
    gst_pad_set_chain_function(ps->sinkpad, gst_pocketsphinx_chain);
    gst_pad_set_event_function(ps->sinkpad, gst_pocketsphinx_event);
    gst_pad_use_fixed_caps(ps->sinkpad);

    gst_element_add_pad(GST_ELEMENT(ps), ps->srcpad);
    gst_pad_use_fixed_caps(ps->srcpad);

    /* Initialize time. */
    ps->last_result_time = 0;
    ps->last_result = NULL;

    /* Nbest size */
    ps->n_best_size = 10;
}

static GstFlowReturn
gst_pocketsphinx_chain(GstPad * pad, GstBuffer * buffer)
{
    GstPocketSphinx *ps;

    ps = GST_POCKETSPHINX(GST_OBJECT_PARENT(pad));

    /* Start an utterance for the first buffer we get (i.e. we assume
     * that the VADER is "leaky") */
    if (!ps->listening) {
        ps->listening = TRUE;
        ps_start_utt(ps->ps, NULL);
    }
    ps_process_raw(ps->ps,
                   (short *)GST_BUFFER_DATA(buffer),
                   GST_BUFFER_SIZE(buffer) / sizeof(short),
                   FALSE, FALSE);

    /* Get a partial result every now and then, see if it is different. */
    if (ps->last_result_time == 0
        /* Check every 100 milliseconds. */
        || (GST_BUFFER_TIMESTAMP(buffer) - ps->last_result_time) > 100*10*1000) {
        int32 score;
        char const *hyp;
        char const *uttid;

        hyp = ps_get_hyp(ps->ps, &score, &uttid);
        ps->last_result_time = GST_BUFFER_TIMESTAMP(buffer);
        if (hyp && strlen(hyp) > 0) {
            if (ps->last_result == NULL || 0 != strcmp(ps->last_result, hyp)) {
                g_free(ps->last_result);
                ps->last_result = g_strdup(hyp);
                /* Emit a signal for applications. */
                g_signal_emit(ps, gst_pocketsphinx_signals[SIGNAL_PARTIAL_RESULT],
                              0, hyp, uttid);
            }
        }
    }
    gst_buffer_unref(buffer);
    return GST_FLOW_OK;
}

static gboolean
gst_pocketsphinx_event(GstPad *pad, GstEvent *event)
{
    GstPocketSphinx *ps;

    ps = GST_POCKETSPHINX(GST_OBJECT_PARENT(pad));

    /* Pick out VAD events. */
    switch (event->type) {
    case GST_EVENT_NEWSEGMENT:
        /* Initialize the decoder once the audio starts, if it's not
         * there yet. */
        if (ps->ps == NULL) {
            ps->ps = ps_init(ps->config);
            if (ps->ps == NULL) {
                GST_ELEMENT_ERROR(GST_ELEMENT(ps), LIBRARY, INIT,
                                  ("Failed to initialize PocketSphinx"),
                                  ("Failed to initialize PocketSphinx"));
                return FALSE;
            }
        }
        return gst_pad_event_default(pad, event);
    case GST_EVENT_VADER_START:
        ps->listening = TRUE;
        ps_start_utt(ps->ps, NULL);
        /* Forward this event. */
        return gst_pad_event_default(pad, event);
    case GST_EVENT_EOS:
    case GST_EVENT_VADER_STOP: {
        GstBuffer *buffer;
        int32 score;
        char const *hyp;
        char const *uttid;

        hyp = NULL;
        if (ps->listening) {
            ps->listening = FALSE;
            ps_end_utt(ps->ps);
            hyp = ps_get_hyp(ps->ps, &score, &uttid);
            /* Dump the lattice if requested. */
            if (ps->latdir) {
                char *latfile = string_join(ps->latdir, "/", uttid, ".lat", NULL);
                ps_lattice_t *dag;

                if ((dag = ps_get_lattice(ps->ps)))
                    ps_lattice_write(dag, latfile);
                ckd_free(latfile);
            }
        }
        if (hyp) {
            /* Emit a signal for applications. */
            g_signal_emit(ps, gst_pocketsphinx_signals[SIGNAL_RESULT],
                          0, hyp, uttid);
            /* Forward this result in a buffer. */
            buffer = gst_buffer_new_and_alloc(strlen(hyp) + 2);
            strcpy((char *)GST_BUFFER_DATA(buffer), hyp);
            GST_BUFFER_DATA(buffer)[strlen(hyp)] = '\n';
            GST_BUFFER_DATA(buffer)[strlen(hyp)+1] = '\0';
            GST_BUFFER_TIMESTAMP(buffer) = GST_EVENT_TIMESTAMP(event);
            gst_buffer_set_caps(buffer, GST_PAD_CAPS(ps->srcpad));
            gst_pad_push(ps->srcpad, buffer);
        }

        /* Forward this event. */
        return gst_pad_event_default(pad, event);
    }
    default:
        /* Don't bother with other events. */
        return gst_pad_event_default(pad, event);
    }
}

static gboolean
plugin_init(GstPlugin * plugin)
{
    if (!gst_element_register(plugin, "pocketsphinx",
                              GST_RANK_NONE, GST_TYPE_POCKETSPHINX))
        return FALSE;
    if (!gst_element_register(plugin, "vader",
                              GST_RANK_NONE, GST_TYPE_VADER))
        return FALSE;
    return TRUE;
}

#define VERSION PACKAGE_VERSION
#define PACKAGE PACKAGE_NAME
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  "pocketsphinx",
                  "PocketSphinx plugin",
                  plugin_init, VERSION,
#if (GST_VERSION_MINOR == 10 && GST_VERSION_MICRO < 15) /* Nokia's bogus old GStreamer */
                  "LGPL",
#else
                  "BSD",
#endif
                  "PocketSphinx", "http://cmusphinx.sourceforge.net/")

/* vim: set ts=4 sw=4: */


// File: index.xml

// File: structacmod__s.xml
%feature("docstring") acmod_s "

Acoustic model structure.

This object encapsulates all stages of acoustic processing, from raw
audio input to acoustic score output. The reason for grouping all of
these modules together is that they all have to \"agree\" in their
parameterizations, and the configuration of the acoustic and dynamic
feature computation is completely dependent on the parameters used to
build the original acoustic model (which should by now always be
specified in a feat.params file).

Because there is not a one-to-one correspondence from blocks of input
audio or frames of input features to frames of acoustic scores (due to
dynamic feature calculation), results may not be immediately available
after input, and the output results will not correspond to the last
piece of data input.

TODO: In addition, this structure serves the purpose of queueing
frames of features (and potentially also scores in the future) for
asynchronous passes of recognition operating in parallel.

C++ includes: acmod.h ";


// File: structastar__seg__s.xml
%feature("docstring") astar_seg_s "

Segmentation \"iterator\" for A* search results.

C++ includes: ps_lattice_internal.h ";


// File: structbestbp__rc__s.xml
%feature("docstring") bestbp_rc_s "";


// File: structbin__mdef__s.xml
%feature("docstring") bin_mdef_s "";


// File: structblkarray__list__s.xml
%feature("docstring") blkarray_list_s "";


// File: structbptbl__s.xml
%feature("docstring") bptbl_s "

Back pointer table (forward pass lattice; actually a tree)

C++ includes: ngram_search.h ";


// File: structbptbl__seg__s.xml
%feature("docstring") bptbl_seg_s "

Segmentation \"iterator\" for backpointer table results.

C++ includes: ngram_search.h ";


// File: structcand__sf__t.xml
%feature("docstring") cand_sf_t "";


// File: structcd__tree__s.xml
%feature("docstring") cd_tree_s "";


// File: structchan__s.xml
%feature("docstring") chan_s "

Lexical tree node data type.

Not the first HMM for words, which multiplex HMMs based on different
left contexts. This structure is used both in the dynamic HMM tree
structure and in the per-word last-phone right context fanout.

C++ includes: ngram_search.h ";


// File: structciphone__t.xml
%feature("docstring") ciphone_t "

CI phone information.

C++ includes: mdef.h ";


// File: structdag__seg__s.xml
%feature("docstring") dag_seg_s "

Segmentation \"iterator\" for backpointer table results.

C++ includes: ps_lattice_internal.h ";


// File: structdict2pid__t.xml
%feature("docstring") dict2pid_t "

Building composite triphone (as well as word internal triphones) with
the dictionary.

C++ includes: dict2pid.h ";


// File: structdict__t.xml
%feature("docstring") dict_t "

a structure for a dictionary.

C++ includes: dict.h ";


// File: structdictword__t.xml
%feature("docstring") dictword_t "

a structure for one dictionary word.

C++ includes: dict.h ";


// File: structfsg__glist__linklist__t.xml
%feature("docstring") fsg_glist_linklist_t "";


// File: structfsg__hist__entry__s.xml
%feature("docstring") fsg_hist_entry_s "";


// File: structfsg__history__s.xml
%feature("docstring") fsg_history_s "";


// File: structfsg__lextree__s.xml
%feature("docstring") fsg_lextree_s "

Collection of lextrees for an FSG.

C++ includes: fsg_lextree.h ";


// File: structfsg__pnode__ctxt__t.xml
%feature("docstring") fsg_pnode_ctxt_t "";


// File: structfsg__pnode__s.xml
%feature("docstring") fsg_pnode_s "";


// File: structfsg__search__s.xml
%feature("docstring") fsg_search_s "

Implementation of FSG search (and \"FSG set\") structure.

C++ includes: fsg_search_internal.h ";


// File: structfsg__seg__s.xml
%feature("docstring") fsg_seg_s "

Segmentation \"iterator\" for FSG history.

C++ includes: fsg_search_internal.h ";


// File: structgauden__dist__t.xml
%feature("docstring") gauden_dist_t "

Structure to store distance (density) values for a given input
observation wrt density values in some given codebook.

C++ includes: ms_gauden.h ";


// File: structgauden__t.xml
%feature("docstring") gauden_t "

Multivariate gaussian mixture density parameters.

C++ includes: ms_gauden.h ";


// File: structhmm__context__s.xml
%feature("docstring") hmm_context_s "";


// File: structhmm__context__t.xml
%feature("docstring") hmm_context_t "

Shared information between a set of HMMs.

We assume that the initial state is emitting and that the transition
matrix is n_emit_state x (n_emit_state+1), where the extra destination
dimension correponds to the non-emitting final or exit state.

C++ includes: hmm.h ";


// File: structhmm__s.xml
%feature("docstring") hmm_s "";


// File: structhmm__t.xml
%feature("docstring") hmm_t "

An individual HMM among the HMM search space.

An individual HMM among the HMM search space. An HMM with N emitting
states consists of N+1 internal states including the non-emitting exit
(out) state.

C++ includes: hmm.h ";


// File: structkws__detection__s.xml
%feature("docstring") kws_detection_s "";


// File: structkws__detections__s.xml
%feature("docstring") kws_detections_s "";


// File: structkws__keyword__s.xml
%feature("docstring") kws_keyword_s "";


// File: structkws__search__s.xml
%feature("docstring") kws_search_s "

Implementation of KWS search structure.

C++ includes: kws_search.h ";


// File: structkws__seg__s.xml
%feature("docstring") kws_seg_s "

Segmentation \"iterator\" for KWS history.

C++ includes: kws_search.h ";


// File: structlast__ltrans__t.xml
%feature("docstring") last_ltrans_t "";


// File: structlastphn__cand__s.xml
%feature("docstring") lastphn_cand_s "";


// File: structlatlink__list__s.xml
%feature("docstring") latlink_list_s "

Linked list of DAG link pointers.

Because the same link structure is used for forward and reverse links,
as well as for the agenda used in bestpath search, we can't store the
list pointer inside latlink_t. We could use glist_t here, but it
wastes 4 bytes per entry on 32-bit machines.

C++ includes: ps_lattice_internal.h ";


// File: structmdef__entry__s.xml
%feature("docstring") mdef_entry_s "";


// File: structmdef__t.xml
%feature("docstring") mdef_t "

The main model definition structure.

strcture for storing the model definition.

C++ includes: mdef.h ";


// File: structms__mgau__model__t.xml
%feature("docstring") ms_mgau_model_t "";


// File: structms__mgau__t.xml
%feature("docstring") ms_mgau_t "

Multi-stream mixture gaussian.

It is not necessary to be continr

C++ includes: ms_mgau.h ";


// File: structngram__search__s.xml
%feature("docstring") ngram_search_s "

N-Gram search module structure.

C++ includes: ngram_search.h ";


// File: structngram__search__stats__s.xml
%feature("docstring") ngram_search_stats_s "

Various statistics for profiling.

C++ includes: ngram_search.h ";


// File: structph__lc__s.xml
%feature("docstring") ph_lc_s "";


// File: structph__lc__t.xml
%feature("docstring") ph_lc_t "

Structures for storing the left context.

C++ includes: mdef.h ";


// File: structph__rc__s.xml
%feature("docstring") ph_rc_s "";


// File: structph__rc__t.xml
%feature("docstring") ph_rc_t "

Structures needed for mapping <ci,lc,rc,wpos> into pid.

(See mdef_t.wpos_ci_lclist below.) (lc = left context; rc = right
context.) NOTE: Both ph_rc_t and ph_lc_t FOR INTERNAL USE ONLY.

C++ includes: mdef.h ";


// File: structphone__loop__renorm__s.xml
%feature("docstring") phone_loop_renorm_s "

Renormalization event.

C++ includes: phone_loop_search.h ";


// File: structphone__loop__s.xml
%feature("docstring") phone_loop_s "

Phone loop structure.

C++ includes: phone_loop_search.h ";


// File: structphone__loop__search__s.xml
%feature("docstring") phone_loop_search_s "

Phone loop search structure.

C++ includes: phone_loop_search.h ";


// File: structphone__t.xml
%feature("docstring") phone_t "

Triphone information, including base phones as a subset.

For the latter, lc, rc and wpos are non-existent.

C++ includes: mdef.h ";


// File: structps__alignment__entry__s.xml
%feature("docstring") ps_alignment_entry_s "";


// File: structps__alignment__iter__s.xml
%feature("docstring") ps_alignment_iter_s "";


// File: structps__alignment__s.xml
%feature("docstring") ps_alignment_s "";


// File: structps__alignment__vector__s.xml
%feature("docstring") ps_alignment_vector_s "";


// File: structps__astar__s.xml
%feature("docstring") ps_astar_s "

A* search structure.

C++ includes: ps_lattice_internal.h ";


// File: structps__decoder__s.xml
%feature("docstring") ps_decoder_s "

Decoder object.

C++ includes: pocketsphinx_internal.h ";


// File: structps__latlink__s.xml
%feature("docstring") ps_latlink_s "

Links between DAG nodes.

A link corresponds to a single hypothesized instance of a word with a
given start and end point.

C++ includes: ps_lattice_internal.h ";


// File: structps__latnode__s.xml
%feature("docstring") ps_latnode_s "

DAG nodes.

A node corresponds to a number of hypothesized instances of a word
which all share the same starting point.

C++ includes: ps_lattice_internal.h ";


// File: structps__latpath__s.xml
%feature("docstring") ps_latpath_s "

Partial path structure used in N-best (A*) search.

Each partial path (latpath_t) is constructed by extending another
partial pathparentby one node.

C++ includes: ps_lattice_internal.h ";


// File: structps__lattice__s.xml
%feature("docstring") ps_lattice_s "

Word graph structure used in bestpath/nbest search.

C++ includes: ps_lattice_internal.h ";


// File: structps__mgau__s.xml
%feature("docstring") ps_mgau_s "";


// File: structps__mgaufuncs__s.xml
%feature("docstring") ps_mgaufuncs_s "";


// File: structps__mllr__s.xml
%feature("docstring") ps_mllr_s "

Feature space linear transform structure.

C++ includes: acmod.h ";


// File: structps__search__iter__s.xml
%feature("docstring") ps_search_iter_s "";


// File: structps__search__s.xml
%feature("docstring") ps_search_s "

Base structure for search module.

C++ includes: pocketsphinx_internal.h ";


// File: structps__searchfuncs__s.xml
%feature("docstring") ps_searchfuncs_s "

V-table for search algorithm.

C++ includes: pocketsphinx_internal.h ";


// File: structps__seg__s.xml
%feature("docstring") ps_seg_s "

Base structure for hypothesis segmentation iterator.

C++ includes: pocketsphinx_internal.h ";


// File: structps__segfuncs__s.xml
%feature("docstring") ps_segfuncs_s "";


// File: structptm__fast__eval__s.xml
%feature("docstring") ptm_fast_eval_s "";


// File: structptm__mgau__s.xml
%feature("docstring") ptm_mgau_s "";


// File: structptm__topn__s.xml
%feature("docstring") ptm_topn_s "";


// File: structroot__chan__s.xml
%feature("docstring") root_chan_s "

Lexical tree node data type for the first phone (root) of each dynamic
HMM tree structure.

Each state may have a different parent static HMM. Most fields are
similar to those in chan_t.

C++ includes: ngram_search.h ";


// File: structs2__semi__mgau__s.xml
%feature("docstring") s2_semi_mgau_s "";


// File: structsenone__t.xml
%feature("docstring") senone_t "

8-bit senone PDF structure.

8-bit senone PDF structure. Senone pdf values are normalized, floored,
converted to logs3 domain, and finally truncated to 8 bits precision
to conserve memory space.

C++ includes: ms_senone.h ";


// File: structstate__align__search__s.xml
%feature("docstring") state_align_search_s "

Phone loop search structure.

C++ includes: state_align_search.h ";


// File: structtmat__t.xml
%feature("docstring") tmat_t "

Transition matrix data structure.

All phone HMMs are assumed to have the same topology.

C++ includes: tmat.h ";


// File: structvqFeature__s.xml
%feature("docstring") vqFeature_s "";


// File: structxwdssid__t.xml
%feature("docstring") xwdssid_t "

cross word triphone model structure

C++ includes: dict2pid.h ";


// File: cmdln__macro_8h.xml


// File: pocketsphinx_8h.xml
%feature("docstring")  ps_default_search_args "

Sets default grammar and language model if they are not set explicitly
and are present in the default search path. ";

%feature("docstring")  ps_init "

Initialize the decoder from a configuration object.

The decoder retains ownership of the pointer config, so you must not
attempt to free it manually. If you wish to reuse it elsewhere, call
cmd_ln_retain() on it.

Parameters:
-----------

config:  a command-line structure, as created by cmd_ln_parse_r() or
cmd_ln_parse_file_r(). ";

%feature("docstring")  Decoder::reinit "

Reinitialize the decoder with updated configuration.

This function allows you to switch the acoustic model, dictionary, or
other configuration without creating an entirely new decoding object.

The decoder retains ownership of the pointer config, so you must not
attempt to free it manually. If you wish to reuse it elsewhere, call
cmd_ln_retain() on it.

Parameters:
-----------

ps:  Decoder.

config:  An optional new configuration to use. If this is NULL, the
previous configuration will be reloaded, with any changes applied.

0 for success, <0 for failure. ";

%feature("docstring")  ps_args "

Returns the argument definitions used in ps_init().

This is here to avoid exporting global data, which is problematic on
Win32 and Symbian (and possibly other platforms). ";

%feature("docstring")  Decoder::retain "

Retain a pointer to the decoder.

This increments the reference count on the decoder, allowing it to be
shared between multiple parent objects. In general you will not need
to use this function, ever. It is mainly here for the convenience of
scripting language bindings.

pointer to retained decoder. ";

%feature("docstring")  Decoder::free "

Finalize the decoder.

This releases all resources associated with the decoder, including any
language models or grammars which have been added to it, and the
initial configuration object passed to ps_init().

Parameters:
-----------

ps:  Decoder to be freed.

New reference count (0 if freed). ";

%feature("docstring")  Decoder::get_config "

Get the configuration object for this decoder.

The configuration object for this decoder. The decoder retains
ownership of this pointer, so you should not attempt to free it
manually. Use cmd_ln_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Decoder::get_logmath "

Get the log-math computation object for this decoder.

The log-math object for this decoder. The decoder retains ownership of
this pointer, so you should not attempt to free it manually. Use
logmath_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Decoder::get_fe "

Get the feature extraction object for this decoder.

The feature extraction object for this decoder. The decoder retains
ownership of this pointer, so you should not attempt to free it
manually. Use fe_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Decoder::get_feat "

Get the dynamic feature computation object for this decoder.

The dynamic feature computation object for this decoder. The decoder
retains ownership of this pointer, so you should not attempt to free
it manually. Use feat_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Decoder::update_mllr "

Adapt current acoustic model using a linear transform.

Parameters:
-----------

mllr:  The new transform to use, or NULL to update the existing
transform. The decoder retains ownership of this pointer, so you
should not attempt to free it manually. Use ps_mllr_retain() if you
wish to reuse it elsewhere.

The updated transform object for this decoder, or NULL on failure. ";

%feature("docstring")  Decoder::set_search "

Actives search with the provided name.

Activates search with the provided name. The search must be added
before using either ps_set_fsg(), ps_set_lm() or ps_set_kws().

0 on success, 1 on failure ";

%feature("docstring")  Decoder::get_search "

Returns name of curent search in decoder.

See:   ps_set_search ";

%feature("docstring")  Decoder::unset_search "

Unsets the search and releases related resources.

Unsets the search previously added with using either ps_set_fsg(),
ps_set_lm() or ps_set_kws().

See:   ps_set_fsg

ps_set_lm

ps_set_kws ";

%feature("docstring")  Decoder::search_iter "

Returns iterator over current searches.

See:   ps_set_search ";

%feature("docstring")  ps_search_iter_next "

Updates search iterator to point to the next position.

This function automatically frees the iterator object upon reaching
the final entry. See:   ps_set_search ";

%feature("docstring")  ps_search_iter_val "

Retrieves the name of the search the iterator points to.

Updates search iterator to point to the next position.

See:   ps_set_search  This function automatically frees the iterator
object upon reaching the final entry. See:   ps_set_search ";

%feature("docstring")  ps_search_iter_free "

Delete an unfinished search iterator.

See:   ps_set_search ";

%feature("docstring")  Decoder::get_lm "

Get the language model set object for this decoder.

If N-Gram decoding is not enabled, this will return NULL. You will
need to enable it using ps_update_lmset().

The language model set object for this decoder. The decoder retains
ownership of this pointer, so you should not attempt to free it
manually. Use ngram_model_retain() if you wish to reuse it elsewhere.
";

%feature("docstring")  Decoder::set_lm "

Adds new search based on N-gram language model.

Associates N-gram search with the provided name. The search can be
activated using ps_set_search().

See:   ps_set_search. ";

%feature("docstring")  Decoder::set_lm_file "

Adds new search based on N-gram language model.

Convenient method to load N-gram model and create a search.

See:   ps_set_lm ";

%feature("docstring")  Decoder::get_fsg "

Get the finite-state grammar set object for this decoder.

If FSG decoding is not enabled, this returns NULL. Call
ps_update_fsgset() to enable it.

The current FSG set object for this decoder, or NULL if none is
available. ";

%feature("docstring")  Decoder::set_fsg "

Adds new search based on finite state grammar.

Associates FSG search with the provided name. The search can be
activated using ps_set_search().

See:   ps_set_search ";

%feature("docstring")  Decoder::set_jsgf_file "

Adds new search using JSGF model.

Convenient method to load JSGF model and create a search.

See:   ps_set_fsg ";

%feature("docstring")  Decoder::get_kws "

Get the current Key phrase to spot.

If KWS is not enabled, this returns NULL. Call ps_update_kws() to
enable it.

The current keyphrase to spot ";

%feature("docstring")  Decoder::set_kws "

Adds keywords from a file to spotting.

Associates KWS search with the provided name. The search can be
activated using ps_set_search().

See:   ps_set_search ";

%feature("docstring")  Decoder::set_keyphrase "

Adds new keyword to spot.

Associates KWS search with the provided name. The search can be
activated using ps_set_search().

See:   ps_set_search ";

%feature("docstring")  Decoder::load_dict "

Reload the pronunciation dictionary from a file.

This function replaces the current pronunciation dictionary with the
one stored in dictfile. This also causes the active search module(s)
to be reinitialized, in the same manner as calling ps_add_word() with
update=TRUE.

Parameters:
-----------

dictfile:  Path to dictionary file to load.

fdictfile:  Path to filler dictionary to load, or NULL to keep the
existing filler dictionary.

format:  Format of the dictionary file, or NULL to determine
automatically (currently unused,should be NULL) ";

%feature("docstring")  Decoder::save_dict "

Dump the current pronunciation dictionary to a file.

This function dumps the current pronunciation dictionary to a tex

Parameters:
-----------

dictfile:  Path to file where dictionary will be written.

format:  Format of the dictionary file, or NULL for the default (text)
format (currently unused, should be NULL) ";

%feature("docstring")  Decoder::add_word "

Add a word to the pronunciation dictionary.

This function adds a word to the pronunciation dictionary and the
current language model (but, obviously, not to the current FSG if FSG
mode is enabled). If the word is already present in one or the other,
it does whatever is necessary to ensure that the word can be
recognized.

Parameters:
-----------

word:  Word string to add.

phones:  Whitespace-separated list of phoneme strings describing
pronunciation of word.

update:  If TRUE, update the search module (whichever one is currently
active) to recognize the newly added word. If adding multiple words,
it is more efficient to pass FALSE here in all but the last word.

The internal ID (>= 0) of the newly added word, or <0 on failure. ";

%feature("docstring")  Decoder::decode_raw "

Decode a raw audio stream.

No headers are recognized in this files. The configuration parameters
-samprate and -input_endian are used to determine the sampling rate
and endianness of the stream, respectively. Audio is always assumed to
be 16-bit signed PCM.

Parameters:
-----------

ps:  Decoder.

rawfh:  Previously opened file stream.

uttid:  Utterance ID (or NULL to generate automatically).

maxsamps:  Maximum number of samples to read from rawfh, or -1 to read
until end- of-file.

Number of samples of audio. ";

%feature("docstring")  Decoder::decode_senscr "

Decode a senone score dump file.

Parameters:
-----------

ps:  Decoder

fh:  Previously opened file handle positioned at start of file.

uttid:  Utterance ID (or NULL to generate automatically).

Number of frames read. ";

%feature("docstring")  Decoder::start_utt "

Start utterance processing.

This function should be called before any utterance data is passed to
the decoder. It marks the start of a new utterance and reinitializes
internal data structures.

Parameters:
-----------

ps:  Decoder to be started.

uttid:  String uniquely identifying this utterance. If NULL, one will
be created.

0 for success, <0 on error. ";

%feature("docstring")  Decoder::get_uttid "

Get current utterance ID.

Parameters:
-----------

ps:  Decoder to query.

Read-only string of the current utterance ID. This is valid only until
the beginning of the next utterance. ";

%feature("docstring")  Decoder::process_raw "

Decode raw audio data.

Parameters:
-----------

ps:  Decoder.

no_search:  If non-zero, perform feature extraction but don't do any
recognition yet. This may be necessary if your processor has trouble
doing recognition in real-time.

full_utt:  If non-zero, this block of data is a full utterance worth
of data. This may allow the recognizer to produce more accurate
results.

Number of frames of data searched, or <0 for error. ";

%feature("docstring")  Decoder::process_cep "

Decode acoustic feature data.

Parameters:
-----------

ps:  Decoder.

no_search:  If non-zero, perform feature extraction but don't do any
recognition yet. This may be necessary if your processor has trouble
doing recognition in real-time.

full_utt:  If non-zero, this block of data is a full utterance worth
of data. This may allow the recognizer to produce more accurate
results.

Number of frames of data searched, or <0 for error. ";

%feature("docstring")  Decoder::get_n_frames "

Get the number of frames of data searched.

Note that there is a delay between this and the number of frames of
audio which have been input to the system. This is due to the fact
that acoustic features are computed using a sliding window of audio,
and dynamic features are computed over a sliding window of acoustic
features.

Parameters:
-----------

ps:  Decoder.

Number of frames of speech data which have been recognized so far. ";

%feature("docstring")  Decoder::end_utt "

End utterance processing.

Parameters:
-----------

ps:  Decoder.

0 for success, <0 on error ";

%feature("docstring")  Decoder::get_hyp "

Get hypothesis string and path score.

Parameters:
-----------

ps:  Decoder.

out_best_score:  Output: path score corresponding to returned string.

out_uttid:  Output: utterance ID for this utterance.

String containing best hypothesis at this point in decoding. NULL if
no hypothesis is available. ";

%feature("docstring")  Decoder::get_hyp_final "

Get hypothesis string and final flag.

Parameters:
-----------

ps:  Decoder.

out_is_best_score:  Output: if hypothesis is reached final state in
the grammar.

String containing best hypothesis at this point in decoding. NULL if
no hypothesis is available. ";

%feature("docstring")  Decoder::get_prob "

Get posterior probability.

Unless the -bestpath option is enabled, this function will always
return zero (corresponding to a posterior probability of 1.0). Even if
-bestpath is enabled, it will also return zero when called on a
partial result. Ongoing research into effective confidence annotation
for partial hypotheses may result in these restrictions being lifted
in future versions.

Parameters:
-----------

ps:  Decoder.

out_uttid:  Output: utterance ID for this utterance.

Posterior probability of the best hypothesis. ";

%feature("docstring")  Decoder::get_lattice "

Get word lattice.

There isn't much you can do with this so far, a public API will appear
in the future.

Parameters:
-----------

ps:  Decoder.

Word lattice object containing all hypotheses so far. NULL if no
hypotheses are available. This pointer is owned by the decoder and you
should not attempt to free it manually. It is only valid until the
next utterance, unless you use ps_lattice_retain() to retain it. ";

%feature("docstring")  Decoder::seg_iter "

Get an iterator over the word segmentation for the best hypothesis.

Parameters:
-----------

ps:  Decoder.

out_best_score:  Output: path score corresponding to hypothesis.

Iterator over the best hypothesis at this point in decoding. NULL if
no hypothesis is available. ";

%feature("docstring")  Segment::next "

Get the next segment in a word segmentation.

Parameters:
-----------

seg:  Segment iterator.

Updated iterator with the next segment. NULL at end of utterance (the
iterator will be freed in this case). ";

%feature("docstring")  Segment::word "

Get word string from a segmentation iterator.

Parameters:
-----------

seg:  Segment iterator.

Read-only string giving string name of this segment. This is only
valid until the next call to ps_seg_next(). ";

%feature("docstring")  Segment::frames "

Get inclusive start and end frames from a segmentation iterator.

These frame numbers are inclusive, i.e. the end frame refers to the
last frame in which the given word or other segment was active.
Therefore, the actual duration is *out_ef - *out_sf + 1.

Parameters:
-----------

seg:  Segment iterator.

out_sf:  Output: First frame index in segment.

out_sf:  Output: Last frame index in segment. ";

%feature("docstring")  Segment::prob "

Get language, acoustic, and posterior probabilities from a
segmentation iterator.

Unless the -bestpath option is enabled, this function will always
return zero (corresponding to a posterior probability of 1.0). Even if
-bestpath is enabled, it will also return zero when called on a
partial result. Ongoing research into effective confidence annotation
for partial hypotheses may result in these restrictions being lifted
in future versions.

Parameters:
-----------

out_ascr:  Output: acoustic model score for this segment.

out_lscr:  Output: language model score for this segment.

out_lback:  Output: language model backoff mode for this segment (i.e.
the number of words used in calculating lscr). This field is, of
course, only meaningful for N-Gram models.

Log posterior probability of current segment. Log is expressed in the
log-base used in the decoder. To convert to linear floating-point, use
logmath_exp( ps_get_logmath(), pprob). ";

%feature("docstring")  Segment::free "

Finish iterating over a word segmentation early, freeing resources. ";

%feature("docstring")  Decoder::nbest "

Get an iterator over the best hypotheses, optionally within a selected
region of the utterance.

Iterator is empty now, it must be advanced with ps_nbest_next first.
The function may also return a NULL which means that there is no
hypothesis available for this utterance.

Parameters:
-----------

ps:  Decoder.

sf:  Start frame for N-best search (0 for whole utterance)

ef:  End frame for N-best search (-1 for whole utterance)

ctx1:  First word of trigram context (NULL for whole utterance)

ctx2:  First word of trigram context (NULL for whole utterance)

Iterator over N-best hypotheses or NULL if no hypothesis is available
";

%feature("docstring")  NBest::next "

Move an N-best list iterator forward.

Parameters:
-----------

nbest:  N-best iterator.

Updated N-best iterator, or NULL if no more hypotheses are available
(iterator is freed ni this case). ";

%feature("docstring")  NBest::hyp "

Get the hypothesis string from an N-best list iterator.

Parameters:
-----------

nbest:  N-best iterator.

out_score:  Output: Path score for this hypothesis.

String containing next best hypothesis. ";

%feature("docstring")  NBest::seg "

Get the word segmentation from an N-best list iterator.

Parameters:
-----------

nbest:  N-best iterator.

out_score:  Output: Path score for this hypothesis.

Iterator over the next best hypothesis. ";

%feature("docstring")  NBest::free "

Finish N-best search early, releasing resources.

Parameters:
-----------

nbest:  N-best iterator. ";

%feature("docstring")  Decoder::get_utt_time "

Get performance information for the current utterance.

Parameters:
-----------

ps:  Decoder.

out_nspeech:  Output: Number of seconds of speech.

out_ncpu:  Output: Number of seconds of CPU time used.

out_nwall:  Output: Number of seconds of wall time used. ";

%feature("docstring")  Decoder::get_all_time "

Get overall performance information.

Parameters:
-----------

ps:  Decoder.

out_nspeech:  Output: Number of seconds of speech.

out_ncpu:  Output: Number of seconds of CPU time used.

out_nwall:  Output: Number of seconds of wall time used. ";

%feature("docstring")  Decoder::get_vad_state "

Checks if the last feed audio buffer contained speech.

Parameters:
-----------

ps:  Decoder.

1 if last buffer contained speech, 0 - otherwise ";


// File: pocketsphinx__export_8h.xml


// File: ps__lattice_8h.xml
%feature("docstring")  ps_lattice_read "

Read a lattice from a file on disk.

Parameters:
-----------

ps:  Decoder to use for processing this lattice, or NULL.

file:  Path to lattice file.

Newly created lattice, or NULL for failure. ";

%feature("docstring")  Lattice::retain "

Retain a lattice.

This function retains ownership of a lattice for the caller,
preventing it from being freed automatically. You must call
ps_lattice_free() to free it after having called this function.

pointer to the retained lattice. ";

%feature("docstring")  Lattice::free "

Free a lattice.

new reference count (0 if dag was freed) ";

%feature("docstring")  Lattice::write "

Write a lattice to disk.

0 for success, <0 on failure. ";

%feature("docstring")  Lattice::write_htk "

Write a lattice to disk in HTK format.

0 for success, <0 on failure. ";

%feature("docstring")  Lattice::get_logmath "

Get the log-math computation object for this lattice.

The log-math object for this lattice. The lattice retains ownership of
this pointer, so you should not attempt to free it manually. Use
logmath_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Lattice::ps_latnode_iter "

Start iterating over nodes in the lattice.

No particular order of traversal is guaranteed, and you should not
depend on this.

Parameters:
-----------

dag:  Lattice to iterate over.

Iterator over lattice nodes. ";

%feature("docstring")  ps_latnode_iter_next "

Move to next node in iteration.

Parameters:
-----------

itor:  Node iterator.

Updated node iterator, or NULL if finished ";

%feature("docstring")  ps_latnode_iter_free "

Stop iterating over nodes.

Parameters:
-----------

itor:  Node iterator. ";

%feature("docstring")  ps_latnode_iter_node "

Get node from iterator. ";

%feature("docstring")  ps_latnode_times "

Get start and end time range for a node.

Parameters:
-----------

node:  Node inquired about.

out_fef:  Output: End frame of first exit from this node.

out_lef:  Output: End frame of last exit from this node.

Start frame for all edges exiting this node. ";

%feature("docstring")  Lattice::ps_latnode_word "

Get word string for this node.

Parameters:
-----------

dag:  Lattice to which node belongs.

node:  Node inquired about.

Word string for this node (possibly a pronunciation variant). ";

%feature("docstring")  Lattice::ps_latnode_baseword "

Get base word string for this node.

Parameters:
-----------

dag:  Lattice to which node belongs.

node:  Node inquired about.

Base word string for this node. ";

%feature("docstring")  ps_latnode_exits "

Iterate over exits from this node.

Parameters:
-----------

node:  Node inquired about.

Iterator over exit links from this node. ";

%feature("docstring")  ps_latnode_entries "

Iterate over entries to this node.

Parameters:
-----------

node:  Node inquired about.

Iterator over entry links to this node. ";

%feature("docstring")  Lattice::ps_latnode_prob "

Get best posterior probability and associated acoustic score from a
lattice node.

Parameters:
-----------

dag:  Lattice to which node belongs.

node:  Node inquired about.

out_link:  Output: exit link with highest posterior probability

Posterior probability of the best link exiting this node. Log is
expressed in the log-base used in the decoder. To convert to linear
floating-point, use logmath_exp(ps_lattice_get_logmath(), pprob). ";

%feature("docstring")  ps_latlink_iter_next "

Get next link from a lattice link iterator.

Parameters:
-----------

itor:  Iterator.

Updated iterator, or NULL if finished. ";

%feature("docstring")  ps_latlink_iter_free "

Stop iterating over links.

Parameters:
-----------

itor:  Link iterator. ";

%feature("docstring")  ps_latlink_iter_link "

Get link from iterator. ";

%feature("docstring")  ps_latlink_times "

Get start and end times from a lattice link.

these are inclusive - i.e. the last frame of this word is ef, not
ef-1.

Parameters:
-----------

link:  Link inquired about.

out_sf:  Output: (optional) start frame of this link.

End frame of this link. ";

%feature("docstring")  ps_latlink_nodes "

Get destination and source nodes from a lattice link.

Parameters:
-----------

link:  Link inquired about

out_src:  Output: (optional) source node.

destination node ";

%feature("docstring")  Lattice::ps_latlink_word "

Get word string from a lattice link.

Parameters:
-----------

dag:  Lattice to which node belongs.

link:  Link inquired about

Word string for this link (possibly a pronunciation variant). ";

%feature("docstring")  Lattice::ps_latlink_baseword "

Get base word string from a lattice link.

Parameters:
-----------

dag:  Lattice to which node belongs.

link:  Link inquired about

Base word string for this link ";

%feature("docstring")  ps_latlink_pred "

Get predecessor link in best path.

Parameters:
-----------

link:  Link inquired about

Best previous link from bestpath search, if any. Otherwise NULL ";

%feature("docstring")  Lattice::ps_latlink_prob "

Get acoustic score and posterior probability from a lattice link.

Parameters:
-----------

dag:  Lattice to which node belongs.

link:  Link inquired about

out_ascr:  Output: (optional) acoustic score.

Posterior probability for this link. Log is expressed in the log-base
used in the decoder. To convert to linear floating-point, use
logmath_exp(ps_lattice_get_logmath(), pprob). ";

%feature("docstring")  Lattice::link "

Create a directed link between \"from\" and \"to\" nodes, but if a
link already exists, choose one with the best link_scr. ";

%feature("docstring")  Lattice::traverse_edges "

Start a forward traversal of edges in a word graph.

A keen eye will notice an inconsistency in this API versus other types
of iterators in PocketSphinx. The reason for this is that the
traversal algorithm is much more efficient when it is able to modify
the lattice structure. Therefore, to avoid giving the impression that
multiple traversals are possible at once, no separate iterator
structure is provided.

Parameters:
-----------

dag:  Lattice to be traversed.

start:  Start node (source) of traversal.

end:  End node (goal) of traversal.

First link in traversal. ";

%feature("docstring")  Lattice::traverse_next "

Get the next link in forward traversal.

Parameters:
-----------

dag:  Lattice to be traversed.

end:  End node (goal) of traversal.

Next link in traversal. ";

%feature("docstring")  Lattice::reverse_edges "

Start a reverse traversal of edges in a word graph.

See ps_lattice_traverse_edges() for why this API is the way it is.

Parameters:
-----------

dag:  Lattice to be traversed.

start:  Start node (goal) of traversal.

end:  End node (source) of traversal.

First link in traversal. ";

%feature("docstring")  Lattice::reverse_next "

Get the next link in reverse traversal.

Parameters:
-----------

dag:  Lattice to be traversed.

start:  Start node (goal) of traversal.

Next link in traversal. ";

%feature("docstring")  Lattice::bestpath "

Do N-Gram based best-path search on a word graph.

This function calculates both the best path as well as the forward
probability used in confidence estimation.

Final link in best path, NULL on error. ";

%feature("docstring")  Lattice::posterior "

Calculate link posterior probabilities on a word graph.

This function assumes that bestpath search has already been done.

Posterior probability of the utterance as a whole. ";

%feature("docstring")  Lattice::posterior_prune "

Prune all links (and associated nodes) below a certain posterior
probability.

This function assumes that ps_lattice_posterior() has already been
called.

Parameters:
-----------

beam:  Minimum posterior probability for links. This is expressed in
the log- base used in the decoder. To convert from linear floating-
point, use logmath_log(ps_lattice_get_logmath(), prob).

number of arcs removed. ";

%feature("docstring")  Lattice::n_frames "

Get the number of frames in the lattice.

Parameters:
-----------

dag:  The lattice in question.

Number of frames in this lattice. ";


// File: ps__mllr_8h.xml
%feature("docstring")  ps_mllr_read "

Read a speaker-adaptive linear transform from a file. ";

%feature("docstring")  ps_mllr_retain "

Retain a pointer to a linear transform. ";

%feature("docstring")  ps_mllr_free "

Release a pointer to a linear transform. ";


// File: acmod_8c.xml
%feature("docstring")  acmod_process_mfcbuf "

Process MFCCs that are in the internal buffer into features. ";

%feature("docstring")  acmod_init_am "";

%feature("docstring")  acmod_init_feat "";

%feature("docstring")  acmod_fe_mismatch "";

%feature("docstring")  acmod_feat_mismatch "";

%feature("docstring")  acmod_init "

Initialize an acoustic model.

Parameters:
-----------

config:  a command-line object containing parameters. This pointer is
not retained by this object.

lmath:  global log-math parameters.

fe:  a previously-initialized acoustic feature module to use, or NULL
to create one automatically. If this is supplied and its parameters do
not match those in the acoustic model, this function will fail. This
pointer is not retained.

fe:  a previously-initialized dynamic feature module to use, or NULL
to create one automatically. If this is supplied and its parameters do
not match those in the acoustic model, this function will fail. This
pointer is not retained.

a newly initialized acmod_t, or NULL on failure. ";

%feature("docstring")  acmod_free "

Finalize an acoustic model. ";

%feature("docstring")  acmod_update_mllr "

Adapt acoustic model using a linear transform.

Parameters:
-----------

mllr:  The new transform to use, or NULL to update the existing
transform. The decoder retains ownership of this pointer, so you
should not attempt to free it manually. Use ps_mllr_retain() if you
wish to reuse it elsewhere.

The updated transform object for this decoder, or NULL on failure. ";

%feature("docstring")  acmod_write_senfh_header "

Write senone dump file header. ";

%feature("docstring")  acmod_set_senfh "

Start logging senone scores to a filehandle.

Parameters:
-----------

acmod:  Acoustic model object.

logfh:  Filehandle to log to.

0 for success, <0 on error. ";

%feature("docstring")  acmod_set_mfcfh "

Start logging MFCCs to a filehandle.

Parameters:
-----------

acmod:  Acoustic model object.

logfh:  Filehandle to log to.

0 for success, <0 on error. ";

%feature("docstring")  acmod_set_rawfh "

Start logging raw audio to a filehandle.

Parameters:
-----------

acmod:  Acoustic model object.

logfh:  Filehandle to log to.

0 for success, <0 on error. ";

%feature("docstring")  acmod_grow_feat_buf "";

%feature("docstring")  acmod_set_grow "

Set memory allocation policy for utterance processing.

Parameters:
-----------

grow_feat:  If non-zero, the internal dynamic feature buffer will
expand as necessary to encompass any amount of data fed to the model.

previous allocation policy. ";

%feature("docstring")  acmod_start_utt "

Mark the start of an utterance. ";

%feature("docstring")  acmod_end_utt "

Mark the end of an utterance. ";

%feature("docstring")  acmod_log_mfc "";

%feature("docstring")  acmod_process_full_cep "";

%feature("docstring")  acmod_process_full_raw "";

%feature("docstring")  acmod_process_raw "

TODO: Set queue length for utterance processing.

This function allows multiple concurrent passes of search to operate
on different parts of the utterance. Feed raw audio data to the
acoustic model for scoring.

Parameters:
-----------

inout_raw:  In: Pointer to buffer of raw samples Out: Pointer to next
sample to be read

inout_n_samps:  In: Number of samples available Out: Number of samples
remaining

full_utt:  If non-zero, this block represents a full utterance and
should be processed as such.

Number of frames of data processed. ";

%feature("docstring")  acmod_process_cep "

Feed acoustic feature data into the acoustic model for scoring.

Parameters:
-----------

inout_cep:  In: Pointer to buffer of features Out: Pointer to next
frame to be read

inout_n_frames:  In: Number of frames available Out: Number of frames
remaining

full_utt:  If non-zero, this block represents a full utterance and
should be processed as such.

Number of frames of data processed. ";

%feature("docstring")  acmod_process_feat "

Feed dynamic feature data into the acoustic model for scoring.

Unlike acmod_process_raw() and acmod_process_cep(), this function
accepts a single frame at a time. This is because there is no need to
do buffering when using dynamic features as input. However, if the
dynamic feature buffer is full, this function will fail, so you should
either always check the return value, or always pair a call to it with
a call to acmod_score().

Parameters:
-----------

feat:  Pointer to one frame of dynamic features.

Number of frames processed (either 0 or 1). ";

%feature("docstring")  acmod_read_senfh_header "";

%feature("docstring")  acmod_set_insenfh "

Set up a senone score dump file for input.

Parameters:
-----------

insenfh:  File handle of dump file

0 for success, <0 for failure ";

%feature("docstring")  acmod_rewind "

Rewind the current utterance, allowing it to be rescored.

After calling this function, the internal frame index is reset, and
acmod_score() will return scores starting at the first frame of the
current utterance. Currently, acmod_set_grow() must have been called
to enable growing the feature buffer in order for this to work. In the
future, senone scores may be cached instead.

0 for success, <0 for failure (if the utterance can't be rewound due
to no feature or score data available) ";

%feature("docstring")  acmod_advance "

Advance the frame index.

This function moves to the next frame of input data. Subsequent calls
to acmod_score() will return scores for that frame, until the next
call to acmod_advance().

New frame index. ";

%feature("docstring")  acmod_write_scores "

Write a frame of senone scores to a dump file. ";

%feature("docstring")  acmod_read_scores_internal "

Internal version, used for reading previous frames in acmod_score() ";

%feature("docstring")  acmod_read_scores "

Read one frame of scores from senone score dump file.

Number of frames read or <0 on error. ";

%feature("docstring")  calc_frame_idx "";

%feature("docstring")  calc_feat_idx "";

%feature("docstring")  acmod_get_frame "

Get a frame of dynamic feature data.

Parameters:
-----------

inout_frame_idx:  Input: frame index to get, or NULL to obtain
features for the most recent frame. Output: frame index corresponding
to this set of features.

Feature array, or NULL if requested frame is not available. ";

%feature("docstring")  acmod_score "

Score one frame of data.

Parameters:
-----------

inout_frame_idx:  Input: frame index to score, or NULL to obtain
scores for the most recent frame. Output: frame index corresponding to
this set of scores.

Array of senone scores for this frame, or NULL if no frame is
available for scoring (such as if a frame index is requested that is
not yet or no longer available). The data pointed to persists only
until the next call to acmod_score() or acmod_advance(). ";

%feature("docstring")  acmod_best_score "

Get best score and senone index for current frame. ";

%feature("docstring")  acmod_clear_active "

Clear set of active senones. ";

%feature("docstring")  acmod_activate_hmm "

Activate senones associated with an HMM. ";

%feature("docstring")  acmod_flags2list "

Build active list from. ";


// File: acmod_8h.xml
%feature("docstring")  acmod_init "

Initialize an acoustic model.

Parameters:
-----------

config:  a command-line object containing parameters. This pointer is
not retained by this object.

lmath:  global log-math parameters.

fe:  a previously-initialized acoustic feature module to use, or NULL
to create one automatically. If this is supplied and its parameters do
not match those in the acoustic model, this function will fail. This
pointer is not retained.

fe:  a previously-initialized dynamic feature module to use, or NULL
to create one automatically. If this is supplied and its parameters do
not match those in the acoustic model, this function will fail. This
pointer is not retained.

a newly initialized acmod_t, or NULL on failure. ";

%feature("docstring")  acmod_update_mllr "

Adapt acoustic model using a linear transform.

Parameters:
-----------

mllr:  The new transform to use, or NULL to update the existing
transform. The decoder retains ownership of this pointer, so you
should not attempt to free it manually. Use ps_mllr_retain() if you
wish to reuse it elsewhere.

The updated transform object for this decoder, or NULL on failure. ";

%feature("docstring")  acmod_set_senfh "

Start logging senone scores to a filehandle.

Parameters:
-----------

acmod:  Acoustic model object.

logfh:  Filehandle to log to.

0 for success, <0 on error. ";

%feature("docstring")  acmod_set_mfcfh "

Start logging MFCCs to a filehandle.

Parameters:
-----------

acmod:  Acoustic model object.

logfh:  Filehandle to log to.

0 for success, <0 on error. ";

%feature("docstring")  acmod_set_rawfh "

Start logging raw audio to a filehandle.

Parameters:
-----------

acmod:  Acoustic model object.

logfh:  Filehandle to log to.

0 for success, <0 on error. ";

%feature("docstring")  acmod_free "

Finalize an acoustic model. ";

%feature("docstring")  acmod_start_utt "

Mark the start of an utterance. ";

%feature("docstring")  acmod_end_utt "

Mark the end of an utterance. ";

%feature("docstring")  acmod_rewind "

Rewind the current utterance, allowing it to be rescored.

After calling this function, the internal frame index is reset, and
acmod_score() will return scores starting at the first frame of the
current utterance. Currently, acmod_set_grow() must have been called
to enable growing the feature buffer in order for this to work. In the
future, senone scores may be cached instead.

0 for success, <0 for failure (if the utterance can't be rewound due
to no feature or score data available) ";

%feature("docstring")  acmod_advance "

Advance the frame index.

This function moves to the next frame of input data. Subsequent calls
to acmod_score() will return scores for that frame, until the next
call to acmod_advance().

New frame index. ";

%feature("docstring")  acmod_set_grow "

Set memory allocation policy for utterance processing.

Parameters:
-----------

grow_feat:  If non-zero, the internal dynamic feature buffer will
expand as necessary to encompass any amount of data fed to the model.

previous allocation policy. ";

%feature("docstring")  acmod_process_raw "

TODO: Set queue length for utterance processing.

This function allows multiple concurrent passes of search to operate
on different parts of the utterance. Feed raw audio data to the
acoustic model for scoring.

Parameters:
-----------

inout_raw:  In: Pointer to buffer of raw samples Out: Pointer to next
sample to be read

inout_n_samps:  In: Number of samples available Out: Number of samples
remaining

full_utt:  If non-zero, this block represents a full utterance and
should be processed as such.

Number of frames of data processed. ";

%feature("docstring")  acmod_process_cep "

Feed acoustic feature data into the acoustic model for scoring.

Parameters:
-----------

inout_cep:  In: Pointer to buffer of features Out: Pointer to next
frame to be read

inout_n_frames:  In: Number of frames available Out: Number of frames
remaining

full_utt:  If non-zero, this block represents a full utterance and
should be processed as such.

Number of frames of data processed. ";

%feature("docstring")  acmod_process_feat "

Feed dynamic feature data into the acoustic model for scoring.

Unlike acmod_process_raw() and acmod_process_cep(), this function
accepts a single frame at a time. This is because there is no need to
do buffering when using dynamic features as input. However, if the
dynamic feature buffer is full, this function will fail, so you should
either always check the return value, or always pair a call to it with
a call to acmod_score().

Parameters:
-----------

feat:  Pointer to one frame of dynamic features.

Number of frames processed (either 0 or 1). ";

%feature("docstring")  acmod_set_insenfh "

Set up a senone score dump file for input.

Parameters:
-----------

insenfh:  File handle of dump file

0 for success, <0 for failure ";

%feature("docstring")  acmod_read_scores "

Read one frame of scores from senone score dump file.

Number of frames read or <0 on error. ";

%feature("docstring")  acmod_get_frame "

Get a frame of dynamic feature data.

Parameters:
-----------

inout_frame_idx:  Input: frame index to get, or NULL to obtain
features for the most recent frame. Output: frame index corresponding
to this set of features.

Feature array, or NULL if requested frame is not available. ";

%feature("docstring")  acmod_score "

Score one frame of data.

Parameters:
-----------

inout_frame_idx:  Input: frame index to score, or NULL to obtain
scores for the most recent frame. Output: frame index corresponding to
this set of scores.

Array of senone scores for this frame, or NULL if no frame is
available for scoring (such as if a frame index is requested that is
not yet or no longer available). The data pointed to persists only
until the next call to acmod_score() or acmod_advance(). ";

%feature("docstring")  acmod_write_senfh_header "

Write senone dump file header. ";

%feature("docstring")  acmod_write_scores "

Write a frame of senone scores to a dump file. ";

%feature("docstring")  acmod_best_score "

Get best score and senone index for current frame. ";

%feature("docstring")  acmod_clear_active "

Clear set of active senones. ";

%feature("docstring")  acmod_activate_hmm "

Activate senones associated with an HMM. ";

%feature("docstring")  acmod_flags2list "

Build active list from. ";


// File: bin__mdef_8c.xml
%feature("docstring")  bin_mdef_read_text "

Read a text mdef from a file (creating an in-memory binary mdef). ";

%feature("docstring")  bin_mdef_retain "

Retain a pointer to a bin_mdef_t. ";

%feature("docstring")  bin_mdef_free "

Release a pointer to a binary mdef. ";

%feature("docstring")  bin_mdef_read "

Read a binary mdef from a file. ";

%feature("docstring")  bin_mdef_write "

Write a binary mdef to a file. ";

%feature("docstring")  bin_mdef_write_text "

Write a binary mdef to a text file. ";

%feature("docstring")  bin_mdef_ciphone_id "

Context-independent phone lookup.

phone id for ciphone.In: ciphone for which id wanted ";

%feature("docstring")  bin_mdef_ciphone_id_nocase "

Case-insensitive context-independent phone lookup.

phone id for ciphone.In: ciphone for which id wanted ";

%feature("docstring")  bin_mdef_ciphone_str "

In: ciphone id for which name wanted. ";

%feature("docstring")  bin_mdef_phone_id "

In: Word position. ";

%feature("docstring")  bin_mdef_phone_id_nearest "";

%feature("docstring")  bin_mdef_phone_str "

Create a phone string for the given phone (base or triphone) id in the
given buf.

0 if successful, -1 if error.Out: On return, buf has the string ";


// File: bin__mdef_8h.xml
%feature("docstring")  bin_mdef_read "

Read a binary mdef from a file. ";

%feature("docstring")  bin_mdef_read_text "

Read a text mdef from a file (creating an in-memory binary mdef). ";

%feature("docstring")  bin_mdef_write "

Write a binary mdef to a file. ";

%feature("docstring")  bin_mdef_write_text "

Write a binary mdef to a text file. ";

%feature("docstring")  bin_mdef_retain "

Retain a pointer to a bin_mdef_t. ";

%feature("docstring")  bin_mdef_free "

Release a pointer to a binary mdef. ";

%feature("docstring")  bin_mdef_ciphone_id "

Context-independent phone lookup.

phone id for ciphone.In: ciphone for which id wanted ";

%feature("docstring")  bin_mdef_ciphone_id_nocase "

Case-insensitive context-independent phone lookup.

phone id for ciphone.In: ciphone for which id wanted ";

%feature("docstring")  bin_mdef_ciphone_str "

In: ciphone id for which name wanted. ";

%feature("docstring")  bin_mdef_phone_id "

In: Word position. ";

%feature("docstring")  bin_mdef_phone_id_nearest "";

%feature("docstring")  bin_mdef_phone_str "

Create a phone string for the given phone (base or triphone) id in the
given buf.

0 if successful, -1 if error.Out: On return, buf has the string ";


// File: blkarray__list_8c.xml
%feature("docstring")  _blkarray_list_init "";

%feature("docstring")  blkarray_list_init "";

%feature("docstring")  blkarray_list_free "

Completely finalize a blkarray_list. ";

%feature("docstring")  blkarray_list_append "";

%feature("docstring")  blkarray_list_reset "";


// File: blkarray__list_8h.xml
%feature("docstring")  _blkarray_list_init "";

%feature("docstring")  blkarray_list_init "";

%feature("docstring")  blkarray_list_free "

Completely finalize a blkarray_list. ";

%feature("docstring")  blkarray_list_append "";

%feature("docstring")  blkarray_list_reset "";


// File: dict_8c.xml
%feature("docstring")  dict_ciphone_id "";

%feature("docstring")  dict_ciphone_str "

Return value: CI phone string for the given word, phone position. ";

%feature("docstring")  dict_add_word "

Add a word with the given ciphone pronunciation list to the
dictionary.

Return value: Result word id if successful, BAD_S3WID otherwise ";

%feature("docstring")  dict_read "";

%feature("docstring")  dict_write "

Write dictionary to a file. ";

%feature("docstring")  dict_init "

Initialize a new dictionary.

If config and mdef are supplied, then the dictionary will be read from
the files specified by the -dict and -fdict options in config, with
case sensitivity determined by the -dictcase option.

Otherwise an empty case-sensitive dictionary will be created.

Return ptr to dict_t if successful, NULL otherwise. ";

%feature("docstring")  dict_wordid "

Return word id for given word string if present.

Otherwise return BAD_S3WID ";

%feature("docstring")  dict_filler_word "

Return 1 if w is a filler word, 0 if not.

A filler word is one that was read in from the filler dictionary;
however, sentence START and FINISH words are not filler words. ";

%feature("docstring")  dict_real_word "

Test if w is a \"real\" word, i.e.

neither a filler word nor START/FINISH. ";

%feature("docstring")  dict_word2basestr "

If the given word contains a trailing \"(....)\" (i.e., a Sphinx-II
style alternative pronunciation specification), strip that trailing
portion from it.

Note that the given string is modified. Return value: If string was
modified, the character position at which the original string was
truncated; otherwise -1. ";

%feature("docstring")  dict_retain "

Retain a pointer to an dict_t. ";

%feature("docstring")  dict_free "

Release a pointer to a dictionary. ";

%feature("docstring")  dict_report "

Report a dictionary structure. ";


// File: dict_8h.xml
%feature("docstring")  dict_init "

Initialize a new dictionary.

If config and mdef are supplied, then the dictionary will be read from
the files specified by the -dict and -fdict options in config, with
case sensitivity determined by the -dictcase option.

Otherwise an empty case-sensitive dictionary will be created.

Return ptr to dict_t if successful, NULL otherwise. ";

%feature("docstring")  dict_write "

Write dictionary to a file. ";

%feature("docstring")  dict_wordid "

Return word id for given word string if present.

Otherwise return BAD_S3WID ";

%feature("docstring")  dict_filler_word "

Return 1 if w is a filler word, 0 if not.

A filler word is one that was read in from the filler dictionary;
however, sentence START and FINISH words are not filler words. ";

%feature("docstring")  dict_real_word "

Test if w is a \"real\" word, i.e.

neither a filler word nor START/FINISH. ";

%feature("docstring")  dict_add_word "

Add a word with the given ciphone pronunciation list to the
dictionary.

Return value: Result word id if successful, BAD_S3WID otherwise ";

%feature("docstring")  dict_ciphone_str "

Return value: CI phone string for the given word, phone position. ";

%feature("docstring")  dict_word2basestr "

If the given word contains a trailing \"(....)\" (i.e., a Sphinx-II
style alternative pronunciation specification), strip that trailing
portion from it.

Note that the given string is modified. Return value: If string was
modified, the character position at which the original string was
truncated; otherwise -1. ";

%feature("docstring")  dict_retain "

Retain a pointer to an dict_t. ";

%feature("docstring")  dict_free "

Release a pointer to a dictionary. ";

%feature("docstring")  dict_report "

Report a dictionary structure. ";


// File: dict2pid_8c.xml
%feature("docstring")  compress_table "

Compress this map ";

%feature("docstring")  compress_right_context_tree "";

%feature("docstring")  compress_left_right_context_tree "";

%feature("docstring")  get_rc_nssid "

ARCHAN, A duplicate of get_rc_npid in ctxt_table.h.

Get number of rc.

I doubt whether it is correct because the compressed map has not been
checked. ";

%feature("docstring")  dict2pid_get_rcmap "

Get RC map. ";

%feature("docstring")  free_compress_map "";

%feature("docstring")  populate_lrdiph "";

%feature("docstring")  dict2pid_add_word "

Add a word to the dict2pid structure (after adding it to dict). ";

%feature("docstring")  dict2pid_internal "

Return the senone sequence ID for the given word position. ";

%feature("docstring")  dict2pid_build "

Build the dict2pid structure for the given model/dictionary. ";

%feature("docstring")  dict2pid_retain "

Retain a pointer to dict2pid. ";

%feature("docstring")  dict2pid_free "

Free the memory dict2pid structure. ";

%feature("docstring")  dict2pid_report "

Report a dict2pid data structure. ";

%feature("docstring")  dict2pid_dump "

For debugging. ";


// File: dict2pid_8h.xml
%feature("docstring")  dict2pid_build "

Build the dict2pid structure for the given model/dictionary. ";

%feature("docstring")  dict2pid_retain "

Retain a pointer to dict2pid. ";

%feature("docstring")  dict2pid_free "

Free the memory dict2pid structure. ";

%feature("docstring")  dict2pid_internal "

Return the senone sequence ID for the given word position. ";

%feature("docstring")  dict2pid_add_word "

Add a word to the dict2pid structure (after adding it to dict). ";

%feature("docstring")  dict2pid_dump "

For debugging. ";

%feature("docstring")  dict2pid_report "

Report a dict2pid data structure. ";

%feature("docstring")  get_rc_nssid "

Get number of rc.

Get number of rc.

I doubt whether it is correct because the compressed map has not been
checked. ";

%feature("docstring")  dict2pid_get_rcmap "

Get RC map. ";


// File: fsg__history_8c.xml
%feature("docstring")  fsg_history_init "";

%feature("docstring")  fsg_history_free "";

%feature("docstring")  fsg_history_set_fsg "";

%feature("docstring")  fsg_history_entry_add "";

%feature("docstring")  fsg_history_end_frame "";

%feature("docstring")  fsg_history_entry_get "";

%feature("docstring")  fsg_history_reset "";

%feature("docstring")  fsg_history_n_entries "";

%feature("docstring")  fsg_history_utt_start "";

%feature("docstring")  fsg_history_utt_end "";

%feature("docstring")  fsg_history_print "";


// File: fsg__history_8h.xml
%feature("docstring")  fsg_history_init "";

%feature("docstring")  fsg_history_utt_start "";

%feature("docstring")  fsg_history_utt_end "";

%feature("docstring")  fsg_history_entry_add "";

%feature("docstring")  fsg_history_end_frame "";

%feature("docstring")  fsg_history_reset "";

%feature("docstring")  fsg_history_n_entries "";

%feature("docstring")  fsg_history_entry_get "";

%feature("docstring")  fsg_history_set_fsg "";

%feature("docstring")  fsg_history_free "";

%feature("docstring")  fsg_history_print "";


// File: fsg__lextree_8c.xml
%feature("docstring")  fsg_psubtree_init "

Build the phone lextree for all transitions out of state from_state.

Return the root node of this tree. Also, return a linear linked list
of all allocated fsg_pnode_t nodes in *alloc_head (for memory
management purposes). ";

%feature("docstring")  fsg_psubtree_free "

Free the given lextree.

alloc_head: head of linear list of allocated nodes updated by
fsg_psubtree_init(). ";

%feature("docstring")  fsg_psubtree_dump "

Dump the list of nodes in the given lextree to the given file.

alloc_head: head of linear list of allocated nodes updated by
fsg_psubtree_init(). ";

%feature("docstring")  fsg_lextree_lc_rc "

Compute the left and right context CIphone sets for each state.

< Dictionary (not FSG) word ID!! ";

%feature("docstring")  fsg_lextree_init "

Create, initialize, and return a new phonetic lextree for the given
FSG. ";

%feature("docstring")  fsg_lextree_dump "

Print an FSG lextree to a file for debugging. ";

%feature("docstring")  fsg_lextree_free "

Free lextrees for an FSG. ";

%feature("docstring")  fsg_glist_linklist_free "";

%feature("docstring")  fsg_pnode_add_all_ctxt "

Set all flags on in the given context bitvector. ";

%feature("docstring")  fsg_pnode_ctxt_sub_generic "

Generic variant for arbitrary size. ";

%feature("docstring")  psubtree_add_trans "";

%feature("docstring")  fsg_psubtree_dump_node "";

%feature("docstring")  fsg_psubtree_pnode_deactivate "

Mark the given pnode as inactive (for search). ";


// File: fsg__lextree_8h.xml
%feature("docstring")  fsg_lextree_init "

Create, initialize, and return a new phonetic lextree for the given
FSG. ";

%feature("docstring")  fsg_lextree_free "

Free lextrees for an FSG. ";

%feature("docstring")  fsg_lextree_dump "

Print an FSG lextree to a file for debugging. ";

%feature("docstring")  fsg_psubtree_pnode_deactivate "

Mark the given pnode as inactive (for search). ";

%feature("docstring")  fsg_pnode_add_all_ctxt "

Set all flags on in the given context bitvector. ";

%feature("docstring")  fsg_pnode_ctxt_sub_generic "

Generic variant for arbitrary size. ";


// File: fsg__search_8c.xml
%feature("docstring")  fsg_search_seg_iter "";

%feature("docstring")  fsg_search_lattice "

Generate a lattice from FSG search results.

One might think that this is simply a matter of adding acoustic scores
to the FSG's edges. However, one would be wrong. The crucial
difference here is that the word lattice is acyclic, and it also
contains timing information. ";

%feature("docstring")  fsg_search_prob "";

%feature("docstring")  fsg_search_add_silences "";

%feature("docstring")  fsg_search_check_dict "";

%feature("docstring")  fsg_search_add_altpron "";

%feature("docstring")  fsg_search_init "

Create, initialize and return a search module. ";

%feature("docstring")  fsg_search_free "

Deallocate search structure. ";

%feature("docstring")  fsg_search_reinit "

Update FSG search module for new or updated FSGs. ";

%feature("docstring")  fsg_search_sen_active "";

%feature("docstring")  fsg_search_hmm_eval "";

%feature("docstring")  fsg_search_pnode_trans "";

%feature("docstring")  fsg_search_pnode_exit "";

%feature("docstring")  fsg_search_hmm_prune_prop "";

%feature("docstring")  fsg_search_null_prop "";

%feature("docstring")  fsg_search_word_trans "";

%feature("docstring")  fsg_search_step "

Step one frame forward through the Viterbi search. ";

%feature("docstring")  fsg_search_start "

Prepare the FSG search structure for beginning decoding of the next
utterance. ";

%feature("docstring")  fsg_search_finish "

Windup and clean the FSG search structure after utterance. ";

%feature("docstring")  fsg_search_find_exit "";

%feature("docstring")  fsg_search_bestpath "";

%feature("docstring")  fsg_search_hyp "

Get hypothesis string from the FSG search. ";

%feature("docstring")  Segment::fsg_seg_bp2itor "";

%feature("docstring")  Segment::fsg_seg_free "";

%feature("docstring")  Segment::fsg_seg_next "";

%feature("docstring")  Lattice::find_node "";

%feature("docstring")  Lattice::new_node "";

%feature("docstring")  Lattice::find_start_node "";

%feature("docstring")  Lattice::find_end_node "";

%feature("docstring")  Lattice::mark_reachable "";


// File: fsg__search__internal_8h.xml
%feature("docstring")  fsg_search_init "

Create, initialize and return a search module. ";

%feature("docstring")  fsg_search_free "

Deallocate search structure. ";

%feature("docstring")  fsg_search_reinit "

Update FSG search module for new or updated FSGs. ";

%feature("docstring")  fsg_search_start "

Prepare the FSG search structure for beginning decoding of the next
utterance. ";

%feature("docstring")  fsg_search_step "

Step one frame forward through the Viterbi search. ";

%feature("docstring")  fsg_search_finish "

Windup and clean the FSG search structure after utterance. ";

%feature("docstring")  fsg_search_hyp "

Get hypothesis string from the FSG search. ";


// File: hmm_8c.xml
%feature("docstring")  hmm_context_init "

Create an HMM context. ";

%feature("docstring")  hmm_context_free "

Free an HMM context.

The transition matrices, senone scores, and senone sequence mapping
are all assumed to be allocated externally, and will NOT be freed by
this function. ";

%feature("docstring")  hmm_init "

Populate a previously-allocated HMM structure, allocating internal
data. ";

%feature("docstring")  hmm_deinit "

Free an HMM structure, releasing internal data (but not the HMM
structure itself). ";

%feature("docstring")  hmm_dump "

For debugging, dump the whole HMM out. ";

%feature("docstring")  hmm_clear_scores "

Reset the scores of the HMM. ";

%feature("docstring")  hmm_clear "

Reset the states of the HMM to the invalid condition.

i.e., scores to WORST_SCORE and hist to undefined. ";

%feature("docstring")  hmm_enter "

Enter an HMM with the given path score and history ID. ";

%feature("docstring")  hmm_normalize "

Renormalize the scores in this HMM based on the given best score. ";

%feature("docstring")  hmm_vit_eval_5st_lr "";

%feature("docstring")  hmm_vit_eval_5st_lr_mpx "";

%feature("docstring")  hmm_vit_eval_3st_lr "";

%feature("docstring")  hmm_vit_eval_3st_lr_mpx "";

%feature("docstring")  hmm_vit_eval_anytopo "";

%feature("docstring")  hmm_vit_eval "

Viterbi evaluation of given HMM.

If this module were being used for tracking state segmentations, the
dummy, non-emitting exit state would have to be updated separately. In
the Viterbi DP diagram, transitions to the exit state occur from the
current time; they are vertical transitions. Hence they should be made
only after the history has been logged for the emitting states. But
we're not bothered with state segmentations, for now. So, we update
the exit state as well. ";

%feature("docstring")  hmm_dump_vit_eval "

Like hmm_vit_eval, but dump HMM state and relevant senscr to fp first,
for debugging;. ";


// File: hmm_8h.xml
%feature("docstring")  hmm_context_init "

Create an HMM context. ";

%feature("docstring")  hmm_context_free "

Free an HMM context.

The transition matrices, senone scores, and senone sequence mapping
are all assumed to be allocated externally, and will NOT be freed by
this function. ";

%feature("docstring")  hmm_init "

Populate a previously-allocated HMM structure, allocating internal
data. ";

%feature("docstring")  hmm_deinit "

Free an HMM structure, releasing internal data (but not the HMM
structure itself). ";

%feature("docstring")  hmm_clear "

Reset the states of the HMM to the invalid condition.

i.e., scores to WORST_SCORE and hist to undefined. ";

%feature("docstring")  hmm_clear_scores "

Reset the scores of the HMM. ";

%feature("docstring")  hmm_normalize "

Renormalize the scores in this HMM based on the given best score. ";

%feature("docstring")  hmm_enter "

Enter an HMM with the given path score and history ID. ";

%feature("docstring")  hmm_vit_eval "

Viterbi evaluation of given HMM.

If this module were being used for tracking state segmentations, the
dummy, non-emitting exit state would have to be updated separately. In
the Viterbi DP diagram, transitions to the exit state occur from the
current time; they are vertical transitions. Hence they should be made
only after the history has been logged for the emitting states. But
we're not bothered with state segmentations, for now. So, we update
the exit state as well. ";

%feature("docstring")  hmm_dump_vit_eval "

Like hmm_vit_eval, but dump HMM state and relevant senscr to fp first,
for debugging;. ";

%feature("docstring")  hmm_dump "

For debugging, dump the whole HMM out. ";


// File: kws__detections_8c.xml
%feature("docstring")  kws_detections_reset "

Reset history structure. ";

%feature("docstring")  kws_detections_add "

Add history entry. ";

%feature("docstring")  kws_detections_hyp_str "

Compose hypothesis. ";


// File: kws__detections_8h.xml
%feature("docstring")  kws_detections_reset "

Reset history structure. ";

%feature("docstring")  kws_detections_add "

Add history entry. ";

%feature("docstring")  kws_detections_hyp_str "

Compose hypothesis. ";


// File: kws__search_8c.xml
%feature("docstring")  kws_search_lattice "";

%feature("docstring")  kws_search_prob "";

%feature("docstring")  Segment::kws_seg_free "";

%feature("docstring")  kws_seg_fill "";

%feature("docstring")  Segment::kws_seg_next "";

%feature("docstring")  kws_search_seg_iter "";

%feature("docstring")  kws_search_check_dict "";

%feature("docstring")  kws_search_sen_active "";

%feature("docstring")  kws_search_hmm_eval "";

%feature("docstring")  kws_search_hmm_prune "";

%feature("docstring")  kws_search_trans "

Do phone transitions. ";

%feature("docstring")  kws_search_read_list "";

%feature("docstring")  kws_search_init "

Create, initialize and return a search module.

Gets keywords either from keyphrase or from a keyphrase file. ";

%feature("docstring")  kws_search_free "

Deallocate search structure. ";

%feature("docstring")  kws_search_reinit "

Update KWS search module for new key phrase. ";

%feature("docstring")  kws_search_start "

Prepare the KWS search structure for beginning decoding of the next
utterance. ";

%feature("docstring")  kws_search_step "

Step one frame forward through the Viterbi search. ";

%feature("docstring")  kws_search_finish "

Windup and clean the KWS search structure after utterance. ";

%feature("docstring")  kws_search_hyp "

Get hypothesis string from the KWS search. ";

%feature("docstring")  kws_search_get_keywords "

Get active keyphrases. ";


// File: kws__search_8h.xml
%feature("docstring")  kws_search_init "

Create, initialize and return a search module.

Gets keywords either from keyphrase or from a keyphrase file. ";

%feature("docstring")  kws_search_free "

Deallocate search structure. ";

%feature("docstring")  kws_search_reinit "

Update KWS search module for new key phrase. ";

%feature("docstring")  kws_search_start "

Prepare the KWS search structure for beginning decoding of the next
utterance. ";

%feature("docstring")  kws_search_step "

Step one frame forward through the Viterbi search. ";

%feature("docstring")  kws_search_finish "

Windup and clean the KWS search structure after utterance. ";

%feature("docstring")  kws_search_hyp "

Get hypothesis string from the KWS search. ";

%feature("docstring")  kws_search_get_keywords "

Get active keyphrases. ";


// File: mdef_8c.xml
%feature("docstring")  ciphone_add "";

%feature("docstring")  find_ph_lc "";

%feature("docstring")  find_ph_rc "";

%feature("docstring")  triphone_add "";

%feature("docstring")  mdef_ciphone_id "

Get the ciphone id given a string name.

ciphone id for the given ciphone string name ";

%feature("docstring")  mdef_ciphone_str "

Get the phone string given the ci phone id.

: READ-ONLY ciphone string name for the given ciphone id ";

%feature("docstring")  mdef_phone_str "

Create a phone string for the given phone (base or triphone) id in the
given buf.

0 if successful, -1 if error. ";

%feature("docstring")  mdef_phone_id "

Decide the phone id given the left, right and base phones.

: phone id for the given constituents if found, else BAD_S3PID ";

%feature("docstring")  mdef_is_ciphone "

Decide whether the phone is ci phone.

1 if given triphone argument is a ciphone, 0 if not, -1 if error ";

%feature("docstring")  mdef_is_cisenone "

Decide whether the senone is a senone for a ci phone, or a ci senone.

1 if a given senone is a ci senone ";

%feature("docstring")  parse_tmat_senmap "";

%feature("docstring")  parse_base_line "";

%feature("docstring")  parse_tri_line "";

%feature("docstring")  sseq_compress "";

%feature("docstring")  noncomment_line "";

%feature("docstring")  mdef_init "";

%feature("docstring")  mdef_report "

Report the model definition's parameters. ";

%feature("docstring")  mdef_free_recursive_lc "

RAH, For freeing memory. ";

%feature("docstring")  mdef_free_recursive_rc "";

%feature("docstring")  mdef_free "

Free an mdef_t. ";


// File: mdef_8h.xml
%feature("docstring")  mdef_init "

Initialize the phone structure from the given model definition file.

It should be treated as a READ-ONLY structure. pointer to the phone
structure created. ";

%feature("docstring")  mdef_ciphone_id "

Get the ciphone id given a string name.

ciphone id for the given ciphone string name ";

%feature("docstring")  mdef_ciphone_str "

Get the phone string given the ci phone id.

: READ-ONLY ciphone string name for the given ciphone id ";

%feature("docstring")  mdef_is_ciphone "

Decide whether the phone is ci phone.

1 if given triphone argument is a ciphone, 0 if not, -1 if error ";

%feature("docstring")  mdef_is_cisenone "

Decide whether the senone is a senone for a ci phone, or a ci senone.

1 if a given senone is a ci senone ";

%feature("docstring")  mdef_phone_id "

Decide the phone id given the left, right and base phones.

: phone id for the given constituents if found, else BAD_S3PID ";

%feature("docstring")  mdef_phone_str "

Create a phone string for the given phone (base or triphone) id in the
given buf.

0 if successful, -1 if error. ";

%feature("docstring")  mdef_hmm_cmp "

Compare the underlying HMMs for two given phones (i.e., compare the
two transition matrix IDs and the individual state(senone) IDs).

0 iff the HMMs are identical, -1 otherwise. ";

%feature("docstring")  mdef_report "

Report the model definition's parameters. ";

%feature("docstring")  mdef_free_recursive_lc "

RAH, For freeing memory. ";

%feature("docstring")  mdef_free_recursive_rc "";

%feature("docstring")  mdef_free "

Free an mdef_t. ";


// File: ms__gauden_8c.xml
%feature("docstring")  gauden_dump "

Dump the definitionn of Gaussian distribution. ";

%feature("docstring")  gauden_dump_ind "

Dump the definition of Gaussian distribution of a particular index to
the standard output stream. ";

%feature("docstring")  gauden_param_read "";

%feature("docstring")  gauden_param_free "";

%feature("docstring")  gauden_dist_precompute "";

%feature("docstring")  gauden_init "

Read mixture gaussian codebooks from the given files.

Allocate memory space needed for them. Apply the specified variance
floor value. Return value: ptr to the model created; NULL if error.
(See Sphinx3 model file-format documentation.) ";

%feature("docstring")  gauden_free "

Release memory allocated by gauden_init.

In: The gauden_t to free ";

%feature("docstring")  compute_dist_all "";

%feature("docstring")  compute_dist "";

%feature("docstring")  gauden_dist "";

%feature("docstring")  gauden_mllr_transform "

Transform Gaussians according to an MLLR matrix (or, eventually,
more). ";


// File: ms__gauden_8h.xml
%feature("docstring")  gauden_init "

Read mixture gaussian codebooks from the given files.

Allocate memory space needed for them. Apply the specified variance
floor value. Return value: ptr to the model created; NULL if error.
(See Sphinx3 model file-format documentation.) ";

%feature("docstring")  gauden_free "

Release memory allocated by gauden_init.

In: The gauden_t to free ";

%feature("docstring")  gauden_mllr_transform "

Transform Gaussians according to an MLLR matrix (or, eventually,
more). ";

%feature("docstring")  gauden_dist "

Compute gaussian density values for the given input observation vector
wrt the specified mixture gaussian codebook (which may consist of
several feature streams).

Density values are left UNnormalized. 0 if successful, -1 otherwise.
";

%feature("docstring")  gauden_dump "

Dump the definitionn of Gaussian distribution. ";

%feature("docstring")  gauden_dump_ind "

Dump the definition of Gaussian distribution of a particular index to
the standard output stream. ";


// File: ms__mgau_8c.xml
%feature("docstring")  ms_mgau_init "";

%feature("docstring")  ms_mgau_free "";

%feature("docstring")  ms_mgau_mllr_transform "";

%feature("docstring")  ms_cont_mgau_frame_eval "";


// File: ms__mgau_8h.xml
%feature("docstring")  ms_mgau_init "";

%feature("docstring")  ms_mgau_free "";

%feature("docstring")  ms_cont_mgau_frame_eval "";

%feature("docstring")  ms_mgau_mllr_transform "";


// File: ms__senone_8c.xml
%feature("docstring")  senone_mgau_map_read "";

%feature("docstring")  senone_mixw_read "";

%feature("docstring")  senone_init "

Load a set of senones (mixing weights and mixture gaussian codebook
mappings) from the given files.

Normalize weights for each codebook, apply the given floor, convert
PDF values to logs3 domain and quantize to 8-bits. pointer to senone
structure created. Caller MUST NOT change its contents. ";

%feature("docstring")  senone_free "

Release memory allocated by senone_init.

In: The senone_t to free ";

%feature("docstring")  senone_eval "";


// File: ms__senone_8h.xml
%feature("docstring")  senone_init "

Load a set of senones (mixing weights and mixture gaussian codebook
mappings) from the given files.

Normalize weights for each codebook, apply the given floor, convert
PDF values to logs3 domain and quantize to 8-bits. pointer to senone
structure created. Caller MUST NOT change its contents. ";

%feature("docstring")  senone_free "

Release memory allocated by senone_init.

In: The senone_t to free ";

%feature("docstring")  senone_eval "

Evaluate the score for the given senone wrt to the given top N
gaussian codewords.

senone score (in logs3 domain). ";


// File: ngram__search_8c.xml
%feature("docstring")  ngram_search_start "";

%feature("docstring")  ngram_search_step "";

%feature("docstring")  ngram_search_finish "";

%feature("docstring")  ngram_search_reinit "";

%feature("docstring")  ngram_search_hyp "";

%feature("docstring")  ngram_search_prob "";

%feature("docstring")  ngram_search_seg_iter "";

%feature("docstring")  ngram_search_update_widmap "";

%feature("docstring")  ngram_search_calc_beams "";

%feature("docstring")  ngram_search_init "

Initialize the N-Gram search module. ";

%feature("docstring")  ngram_search_free "

Finalize the N-Gram search module. ";

%feature("docstring")  ngram_search_mark_bptable "

Record the current frame's index in the backpointer table.

the current backpointer index. ";

%feature("docstring")  set_real_wid "";

%feature("docstring")  ngram_search_save_bp "

Enter a word in the backpointer table. ";

%feature("docstring")  ngram_search_find_exit "

Find the best word exit for the current frame in the backpointer
table.

the backpointer index of the best word exit. ";

%feature("docstring")  ngram_search_bp_hyp "

Backtrace from a given backpointer index to obtain a word hypothesis.

a read-only string with the best hypothesis. ";

%feature("docstring")  ngram_search_alloc_all_rc "

Allocate last phone channels for all possible right contexts for word
w. ";

%feature("docstring")  ngram_search_free_all_rc "

Allocate last phone channels for all possible right contexts for word
w. ";

%feature("docstring")  ngram_search_exit_score "

Get the exit score for a backpointer entry with a given right context.
";

%feature("docstring")  ngram_compute_seg_score "";

%feature("docstring")  dump_bptable "";

%feature("docstring")  ngram_search_bestpath "";

%feature("docstring")  Segment::ngram_search_bp2itor "";

%feature("docstring")  Segment::ngram_bp_seg_free "";

%feature("docstring")  Segment::ngram_bp_seg_next "";

%feature("docstring")  ngram_search_bp_iter "";

%feature("docstring")  Lattice::create_dag_nodes "";

%feature("docstring")  Lattice::find_start_node "";

%feature("docstring")  Lattice::find_end_node "";

%feature("docstring")  ngram_search_lattice "

Construct a word lattice from the current hypothesis. ";

%feature("docstring")  ngram_search_set_lm "

Sets the global language model.

Sets the language model to use if nothing was passed in configuration
";


// File: ngram__search_8h.xml
%feature("docstring")  ngram_search_init "

Initialize the N-Gram search module. ";

%feature("docstring")  ngram_search_free "

Finalize the N-Gram search module. ";

%feature("docstring")  ngram_search_mark_bptable "

Record the current frame's index in the backpointer table.

the current backpointer index. ";

%feature("docstring")  ngram_search_save_bp "

Enter a word in the backpointer table. ";

%feature("docstring")  ngram_search_alloc_all_rc "

Allocate last phone channels for all possible right contexts for word
w. ";

%feature("docstring")  ngram_search_free_all_rc "

Allocate last phone channels for all possible right contexts for word
w. ";

%feature("docstring")  ngram_search_find_exit "

Find the best word exit for the current frame in the backpointer
table.

the backpointer index of the best word exit. ";

%feature("docstring")  ngram_search_bp_hyp "

Backtrace from a given backpointer index to obtain a word hypothesis.

a read-only string with the best hypothesis. ";

%feature("docstring")  ngram_compute_seg_scores "

Compute language and acoustic scores for backpointer table entries. ";

%feature("docstring")  ngram_search_lattice "

Construct a word lattice from the current hypothesis. ";

%feature("docstring")  ngram_search_exit_score "

Get the exit score for a backpointer entry with a given right context.
";

%feature("docstring")  ngram_search_set_lm "

Sets the global language model.

Sets the language model to use if nothing was passed in configuration
";


// File: ngram__search__fwdflat_8c.xml
%feature("docstring")  ngram_fwdflat_expand_all "";

%feature("docstring")  ngram_fwdflat_allocate_1ph "";

%feature("docstring")  ngram_fwdflat_free_1ph "";

%feature("docstring")  ngram_fwdflat_init "

Initialize N-Gram search for fwdflat decoding. ";

%feature("docstring")  ngram_fwdflat_deinit "

Release memory associated with fwdflat decoding. ";

%feature("docstring")  ngram_fwdflat_reinit "

Rebuild search structures for updated language models. ";

%feature("docstring")  build_fwdflat_wordlist "

Find all active words in backpointer table and sort by frame. ";

%feature("docstring")  build_fwdflat_chan "

Build HMM network for one utterance of fwdflat search. ";

%feature("docstring")  ngram_fwdflat_start "

Start fwdflat decoding for an utterance. ";

%feature("docstring")  compute_fwdflat_sen_active "";

%feature("docstring")  fwdflat_eval_chan "";

%feature("docstring")  fwdflat_prune_chan "";

%feature("docstring")  get_expand_wordlist "";

%feature("docstring")  fwdflat_word_transition "";

%feature("docstring")  fwdflat_renormalize_scores "";

%feature("docstring")  ngram_fwdflat_search "

Search one frame forward in an utterance. ";

%feature("docstring")  destroy_fwdflat_wordlist "

Destroy wordlist from the current utterance. ";

%feature("docstring")  destroy_fwdflat_chan "

Free HMM network for one utterance of fwdflat search. ";

%feature("docstring")  ngram_fwdflat_finish "

Finish fwdflat decoding for an utterance. ";


// File: ngram__search__fwdflat_8h.xml
%feature("docstring")  ngram_fwdflat_init "

Initialize N-Gram search for fwdflat decoding. ";

%feature("docstring")  ngram_fwdflat_deinit "

Release memory associated with fwdflat decoding. ";

%feature("docstring")  ngram_fwdflat_reinit "

Rebuild search structures for updated language models. ";

%feature("docstring")  ngram_fwdflat_start "

Start fwdflat decoding for an utterance. ";

%feature("docstring")  ngram_fwdflat_search "

Search one frame forward in an utterance. ";

%feature("docstring")  ngram_fwdflat_finish "

Finish fwdflat decoding for an utterance. ";


// File: ngram__search__fwdtree_8c.xml
%feature("docstring")  init_search_tree "";

%feature("docstring")  init_nonroot_chan "";

%feature("docstring")  create_search_tree "";

%feature("docstring")  reinit_search_subtree "";

%feature("docstring")  reinit_search_tree "";

%feature("docstring")  ngram_fwdtree_init "

Initialize N-Gram search for fwdtree decoding. ";

%feature("docstring")  deinit_search_tree "";

%feature("docstring")  ngram_fwdtree_deinit "

Release memory associated with fwdtree decoding. ";

%feature("docstring")  ngram_fwdtree_reinit "

Rebuild search structures for updated language models. ";

%feature("docstring")  ngram_fwdtree_start "

Start fwdtree decoding for an utterance. ";

%feature("docstring")  compute_sen_active "";

%feature("docstring")  renormalize_scores "";

%feature("docstring")  eval_root_chan "";

%feature("docstring")  eval_nonroot_chan "";

%feature("docstring")  eval_word_chan "";

%feature("docstring")  evaluate_channels "";

%feature("docstring")  prune_root_chan "";

%feature("docstring")  prune_nonroot_chan "";

%feature("docstring")  last_phone_transition "";

%feature("docstring")  prune_word_chan "";

%feature("docstring")  prune_channels "";

%feature("docstring")  bptable_maxwpf "";

%feature("docstring")  word_transition "";

%feature("docstring")  deactivate_channels "";

%feature("docstring")  ngram_fwdtree_search "

Search one frame forward in an utterance.

Number of frames searched (either 0 or 1). ";

%feature("docstring")  ngram_fwdtree_finish "

Finish fwdtree decoding for an utterance. ";


// File: ngram__search__fwdtree_8h.xml
%feature("docstring")  ngram_fwdtree_init "

Initialize N-Gram search for fwdtree decoding. ";

%feature("docstring")  ngram_fwdtree_deinit "

Release memory associated with fwdtree decoding. ";

%feature("docstring")  ngram_fwdtree_reinit "

Rebuild search structures for updated language models. ";

%feature("docstring")  ngram_fwdtree_start "

Start fwdtree decoding for an utterance. ";

%feature("docstring")  ngram_fwdtree_search "

Search one frame forward in an utterance.

Number of frames searched (either 0 or 1). ";

%feature("docstring")  ngram_fwdtree_finish "

Finish fwdtree decoding for an utterance. ";


// File: phone__loop__search_8c.xml
%feature("docstring")  phone_loop_search_start "";

%feature("docstring")  phone_loop_search_step "";

%feature("docstring")  phone_loop_search_finish "";

%feature("docstring")  phone_loop_search_reinit "";

%feature("docstring")  phone_loop_search_free "";

%feature("docstring")  phone_loop_search_hyp "";

%feature("docstring")  phone_loop_search_prob "";

%feature("docstring")  phone_loop_search_seg_iter "";

%feature("docstring")  phone_loop_search_init "";

%feature("docstring")  phone_loop_search_free_renorm "";

%feature("docstring")  renormalize_hmms "";

%feature("docstring")  evaluate_hmms "";

%feature("docstring")  prune_hmms "";

%feature("docstring")  phone_transition "";


// File: phone__loop__search_8h.xml
%feature("docstring")  phone_loop_search_init "";


// File: pocketsphinx_8c.xml
%feature("docstring")  file_exists "";

%feature("docstring")  hmmdir_exists "";

%feature("docstring")  Decoder::add_file "";

%feature("docstring")  Decoder::init_defaults "";

%feature("docstring")  Decoder::free_searches "";

%feature("docstring")  Decoder::find_search "";

%feature("docstring")  ps_default_search_args "

Sets default grammar and language model if they are not set explicitly
and are present in the default search path. ";

%feature("docstring")  Decoder::reinit "

Reinitialize the decoder with updated configuration.

This function allows you to switch the acoustic model, dictionary, or
other configuration without creating an entirely new decoding object.

The decoder retains ownership of the pointer config, so you must not
attempt to free it manually. If you wish to reuse it elsewhere, call
cmd_ln_retain() on it.

Parameters:
-----------

ps:  Decoder.

config:  An optional new configuration to use. If this is NULL, the
previous configuration will be reloaded, with any changes applied.

0 for success, <0 for failure. ";

%feature("docstring")  ps_init "

Initialize the decoder from a configuration object.

The decoder retains ownership of the pointer config, so you must not
attempt to free it manually. If you wish to reuse it elsewhere, call
cmd_ln_retain() on it.

Parameters:
-----------

config:  a command-line structure, as created by cmd_ln_parse_r() or
cmd_ln_parse_file_r(). ";

%feature("docstring")  ps_args "

Returns the argument definitions used in ps_init().

This is here to avoid exporting global data, which is problematic on
Win32 and Symbian (and possibly other platforms). ";

%feature("docstring")  Decoder::retain "

Retain a pointer to the decoder.

This increments the reference count on the decoder, allowing it to be
shared between multiple parent objects. In general you will not need
to use this function, ever. It is mainly here for the convenience of
scripting language bindings.

pointer to retained decoder. ";

%feature("docstring")  Decoder::free "

Finalize the decoder.

This releases all resources associated with the decoder, including any
language models or grammars which have been added to it, and the
initial configuration object passed to ps_init().

Parameters:
-----------

ps:  Decoder to be freed.

New reference count (0 if freed). ";

%feature("docstring")  Decoder::get_uttid "

Get current utterance ID.

Parameters:
-----------

ps:  Decoder to query.

Read-only string of the current utterance ID. This is valid only until
the beginning of the next utterance. ";

%feature("docstring")  Decoder::get_config "

Get the configuration object for this decoder.

The configuration object for this decoder. The decoder retains
ownership of this pointer, so you should not attempt to free it
manually. Use cmd_ln_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Decoder::get_logmath "

Get the log-math computation object for this decoder.

The log-math object for this decoder. The decoder retains ownership of
this pointer, so you should not attempt to free it manually. Use
logmath_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Decoder::get_fe "

Get the feature extraction object for this decoder.

The feature extraction object for this decoder. The decoder retains
ownership of this pointer, so you should not attempt to free it
manually. Use fe_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Decoder::get_feat "

Get the dynamic feature computation object for this decoder.

The dynamic feature computation object for this decoder. The decoder
retains ownership of this pointer, so you should not attempt to free
it manually. Use feat_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Decoder::update_mllr "

Adapt current acoustic model using a linear transform.

Parameters:
-----------

mllr:  The new transform to use, or NULL to update the existing
transform. The decoder retains ownership of this pointer, so you
should not attempt to free it manually. Use ps_mllr_retain() if you
wish to reuse it elsewhere.

The updated transform object for this decoder, or NULL on failure. ";

%feature("docstring")  Decoder::set_search "

Actives search with the provided name.

Activates search with the provided name. The search must be added
before using either ps_set_fsg(), ps_set_lm() or ps_set_kws().

0 on success, 1 on failure ";

%feature("docstring")  Decoder::get_search "

Returns name of curent search in decoder.

See:   ps_set_search ";

%feature("docstring")  Decoder::unset_search "

Unsets the search and releases related resources.

Unsets the search previously added with using either ps_set_fsg(),
ps_set_lm() or ps_set_kws().

See:   ps_set_fsg

ps_set_lm

ps_set_kws ";

%feature("docstring")  Decoder::search_iter "

Returns iterator over current searches.

See:   ps_set_search ";

%feature("docstring")  ps_search_iter_next "

Updates search iterator to point to the next position.

This function automatically frees the iterator object upon reaching
the final entry. See:   ps_set_search ";

%feature("docstring")  ps_search_iter_val "

Retrieves the name of the search the iterator points to.

Updates search iterator to point to the next position.

See:   ps_set_search  This function automatically frees the iterator
object upon reaching the final entry. See:   ps_set_search ";

%feature("docstring")  ps_search_iter_free "

Delete an unfinished search iterator.

See:   ps_set_search ";

%feature("docstring")  Decoder::get_lm "

Get the language model set object for this decoder.

If N-Gram decoding is not enabled, this will return NULL. You will
need to enable it using ps_update_lmset().

The language model set object for this decoder. The decoder retains
ownership of this pointer, so you should not attempt to free it
manually. Use ngram_model_retain() if you wish to reuse it elsewhere.
";

%feature("docstring")  Decoder::get_fsg "

Get the finite-state grammar set object for this decoder.

If FSG decoding is not enabled, this returns NULL. Call
ps_update_fsgset() to enable it.

The current FSG set object for this decoder, or NULL if none is
available. ";

%feature("docstring")  Decoder::get_kws "

Get the current Key phrase to spot.

If KWS is not enabled, this returns NULL. Call ps_update_kws() to
enable it.

The current keyphrase to spot ";

%feature("docstring")  Decoder::set_search_internal "";

%feature("docstring")  Decoder::set_lm "

Adds new search based on N-gram language model.

Associates N-gram search with the provided name. The search can be
activated using ps_set_search().

See:   ps_set_search. ";

%feature("docstring")  Decoder::set_lm_file "

Adds new search based on N-gram language model.

Convenient method to load N-gram model and create a search.

See:   ps_set_lm ";

%feature("docstring")  Decoder::set_kws "

Adds keywords from a file to spotting.

Associates KWS search with the provided name. The search can be
activated using ps_set_search().

See:   ps_set_search ";

%feature("docstring")  Decoder::set_keyphrase "

Adds new keyword to spot.

Associates KWS search with the provided name. The search can be
activated using ps_set_search().

See:   ps_set_search ";

%feature("docstring")  Decoder::set_fsg "

Adds new search based on finite state grammar.

Associates FSG search with the provided name. The search can be
activated using ps_set_search().

See:   ps_set_search ";

%feature("docstring")  Decoder::set_jsgf_file "

Adds new search using JSGF model.

Convenient method to load JSGF model and create a search.

See:   ps_set_fsg ";

%feature("docstring")  Decoder::load_dict "

Reload the pronunciation dictionary from a file.

This function replaces the current pronunciation dictionary with the
one stored in dictfile. This also causes the active search module(s)
to be reinitialized, in the same manner as calling ps_add_word() with
update=TRUE.

Parameters:
-----------

dictfile:  Path to dictionary file to load.

fdictfile:  Path to filler dictionary to load, or NULL to keep the
existing filler dictionary.

format:  Format of the dictionary file, or NULL to determine
automatically (currently unused,should be NULL) ";

%feature("docstring")  Decoder::save_dict "

Dump the current pronunciation dictionary to a file.

This function dumps the current pronunciation dictionary to a tex

Parameters:
-----------

dictfile:  Path to file where dictionary will be written.

format:  Format of the dictionary file, or NULL for the default (text)
format (currently unused, should be NULL) ";

%feature("docstring")  Decoder::add_word "

Add a word to the pronunciation dictionary.

This function adds a word to the pronunciation dictionary and the
current language model (but, obviously, not to the current FSG if FSG
mode is enabled). If the word is already present in one or the other,
it does whatever is necessary to ensure that the word can be
recognized.

Parameters:
-----------

word:  Word string to add.

phones:  Whitespace-separated list of phoneme strings describing
pronunciation of word.

update:  If TRUE, update the search module (whichever one is currently
active) to recognize the newly added word. If adding multiple words,
it is more efficient to pass FALSE here in all but the last word.

The internal ID (>= 0) of the newly added word, or <0 on failure. ";

%feature("docstring")  Decoder::decode_raw "

Decode a raw audio stream.

No headers are recognized in this files. The configuration parameters
-samprate and -input_endian are used to determine the sampling rate
and endianness of the stream, respectively. Audio is always assumed to
be 16-bit signed PCM.

Parameters:
-----------

ps:  Decoder.

rawfh:  Previously opened file stream.

uttid:  Utterance ID (or NULL to generate automatically).

maxsamps:  Maximum number of samples to read from rawfh, or -1 to read
until end- of-file.

Number of samples of audio. ";

%feature("docstring")  Decoder::start_utt "

Start utterance processing.

This function should be called before any utterance data is passed to
the decoder. It marks the start of a new utterance and reinitializes
internal data structures.

Parameters:
-----------

ps:  Decoder to be started.

uttid:  String uniquely identifying this utterance. If NULL, one will
be created.

0 for success, <0 on error. ";

%feature("docstring")  Decoder::search_forward "";

%feature("docstring")  Decoder::decode_senscr "

Decode a senone score dump file.

Parameters:
-----------

ps:  Decoder

fh:  Previously opened file handle positioned at start of file.

uttid:  Utterance ID (or NULL to generate automatically).

Number of frames read. ";

%feature("docstring")  Decoder::process_raw "

Decode raw audio data.

Parameters:
-----------

ps:  Decoder.

no_search:  If non-zero, perform feature extraction but don't do any
recognition yet. This may be necessary if your processor has trouble
doing recognition in real-time.

full_utt:  If non-zero, this block of data is a full utterance worth
of data. This may allow the recognizer to produce more accurate
results.

Number of frames of data searched, or <0 for error. ";

%feature("docstring")  Decoder::process_cep "";

%feature("docstring")  Decoder::end_utt "

End utterance processing.

Parameters:
-----------

ps:  Decoder.

0 for success, <0 on error ";

%feature("docstring")  Decoder::get_hyp "

Get hypothesis string and path score.

Parameters:
-----------

ps:  Decoder.

out_best_score:  Output: path score corresponding to returned string.

out_uttid:  Output: utterance ID for this utterance.

String containing best hypothesis at this point in decoding. NULL if
no hypothesis is available. ";

%feature("docstring")  Decoder::get_hyp_final "

Get hypothesis string and final flag.

Parameters:
-----------

ps:  Decoder.

out_is_best_score:  Output: if hypothesis is reached final state in
the grammar.

String containing best hypothesis at this point in decoding. NULL if
no hypothesis is available. ";

%feature("docstring")  Decoder::get_prob "

Get posterior probability.

Unless the -bestpath option is enabled, this function will always
return zero (corresponding to a posterior probability of 1.0). Even if
-bestpath is enabled, it will also return zero when called on a
partial result. Ongoing research into effective confidence annotation
for partial hypotheses may result in these restrictions being lifted
in future versions.

Parameters:
-----------

ps:  Decoder.

out_uttid:  Output: utterance ID for this utterance.

Posterior probability of the best hypothesis. ";

%feature("docstring")  Decoder::seg_iter "

Get an iterator over the word segmentation for the best hypothesis.

Parameters:
-----------

ps:  Decoder.

out_best_score:  Output: path score corresponding to hypothesis.

Iterator over the best hypothesis at this point in decoding. NULL if
no hypothesis is available. ";

%feature("docstring")  Segment::next "

Get the next segment in a word segmentation.

Parameters:
-----------

seg:  Segment iterator.

Updated iterator with the next segment. NULL at end of utterance (the
iterator will be freed in this case). ";

%feature("docstring")  Segment::word "

Get word string from a segmentation iterator.

Parameters:
-----------

seg:  Segment iterator.

Read-only string giving string name of this segment. This is only
valid until the next call to ps_seg_next(). ";

%feature("docstring")  Segment::frames "

Get inclusive start and end frames from a segmentation iterator.

These frame numbers are inclusive, i.e. the end frame refers to the
last frame in which the given word or other segment was active.
Therefore, the actual duration is *out_ef - *out_sf + 1.

Parameters:
-----------

seg:  Segment iterator.

out_sf:  Output: First frame index in segment.

out_sf:  Output: Last frame index in segment. ";

%feature("docstring")  Segment::prob "

Get language, acoustic, and posterior probabilities from a
segmentation iterator.

Unless the -bestpath option is enabled, this function will always
return zero (corresponding to a posterior probability of 1.0). Even if
-bestpath is enabled, it will also return zero when called on a
partial result. Ongoing research into effective confidence annotation
for partial hypotheses may result in these restrictions being lifted
in future versions.

Parameters:
-----------

out_ascr:  Output: acoustic model score for this segment.

out_lscr:  Output: language model score for this segment.

out_lback:  Output: language model backoff mode for this segment (i.e.
the number of words used in calculating lscr). This field is, of
course, only meaningful for N-Gram models.

Log posterior probability of current segment. Log is expressed in the
log-base used in the decoder. To convert to linear floating-point, use
logmath_exp( ps_get_logmath(), pprob). ";

%feature("docstring")  Segment::free "

Finish iterating over a word segmentation early, freeing resources. ";

%feature("docstring")  Decoder::get_lattice "

Get word lattice.

There isn't much you can do with this so far, a public API will appear
in the future.

Parameters:
-----------

ps:  Decoder.

Word lattice object containing all hypotheses so far. NULL if no
hypotheses are available. This pointer is owned by the decoder and you
should not attempt to free it manually. It is only valid until the
next utterance, unless you use ps_lattice_retain() to retain it. ";

%feature("docstring")  Decoder::nbest "

Get an iterator over the best hypotheses, optionally within a selected
region of the utterance.

Iterator is empty now, it must be advanced with ps_nbest_next first.
The function may also return a NULL which means that there is no
hypothesis available for this utterance.

Parameters:
-----------

ps:  Decoder.

sf:  Start frame for N-best search (0 for whole utterance)

ef:  End frame for N-best search (-1 for whole utterance)

ctx1:  First word of trigram context (NULL for whole utterance)

ctx2:  First word of trigram context (NULL for whole utterance)

Iterator over N-best hypotheses or NULL if no hypothesis is available
";

%feature("docstring")  NBest::free "

Finish N-best search early, releasing resources.

Parameters:
-----------

nbest:  N-best iterator. ";

%feature("docstring")  NBest::next "

Move an N-best list iterator forward.

Parameters:
-----------

nbest:  N-best iterator.

Updated N-best iterator, or NULL if no more hypotheses are available
(iterator is freed ni this case). ";

%feature("docstring")  NBest::hyp "

Get the hypothesis string from an N-best list iterator.

Parameters:
-----------

nbest:  N-best iterator.

out_score:  Output: Path score for this hypothesis.

String containing next best hypothesis. ";

%feature("docstring")  NBest::seg "

Get the word segmentation from an N-best list iterator.

Parameters:
-----------

nbest:  N-best iterator.

out_score:  Output: Path score for this hypothesis.

Iterator over the next best hypothesis. ";

%feature("docstring")  Decoder::get_n_frames "

Get the number of frames of data searched.

Note that there is a delay between this and the number of frames of
audio which have been input to the system. This is due to the fact
that acoustic features are computed using a sliding window of audio,
and dynamic features are computed over a sliding window of acoustic
features.

Parameters:
-----------

ps:  Decoder.

Number of frames of speech data which have been recognized so far. ";

%feature("docstring")  Decoder::get_utt_time "

Get performance information for the current utterance.

Parameters:
-----------

ps:  Decoder.

out_nspeech:  Output: Number of seconds of speech.

out_ncpu:  Output: Number of seconds of CPU time used.

out_nwall:  Output: Number of seconds of wall time used. ";

%feature("docstring")  Decoder::get_all_time "

Get overall performance information.

Parameters:
-----------

ps:  Decoder.

out_nspeech:  Output: Number of seconds of speech.

out_ncpu:  Output: Number of seconds of CPU time used.

out_nwall:  Output: Number of seconds of wall time used. ";

%feature("docstring")  Decoder::get_vad_state "

Checks if the last feed audio buffer contained speech.

Parameters:
-----------

ps:  Decoder.

1 if last buffer contained speech, 0 - otherwise ";

%feature("docstring")  ps_search_init "

Initialize base structure. ";

%feature("docstring")  ps_search_base_reinit "

Re-initialize base structure with new dictionary. ";

%feature("docstring")  ps_search_deinit "

De-initialize base structure. ";


// File: pocketsphinx__internal_8h.xml
%feature("docstring")  ps_search_init "

Initialize base structure. ";

%feature("docstring")  ps_search_base_reinit "

Re-initialize base structure with new dictionary. ";

%feature("docstring")  ps_search_deinit "

De-initialize base structure. ";


// File: ps__alignment_8c.xml
%feature("docstring")  ps_alignment_init "

Create a new, empty alignment. ";

%feature("docstring")  ps_alignment_free "

Release an alignment. ";

%feature("docstring")  vector_grow_one "";

%feature("docstring")  ps_alignment_vector_grow_one "";

%feature("docstring")  ps_alignment_vector_empty "";

%feature("docstring")  ps_alignment_add_word "

Append a word. ";

%feature("docstring")  ps_alignment_populate "

Populate lower layers using available word information. ";

%feature("docstring")  ps_alignment_populate_ci "

Populate lower layers using context-independent phones. ";

%feature("docstring")  ps_alignment_propagate "

Propagate timing information up from state sequence. ";

%feature("docstring")  ps_alignment_n_words "

Number of words. ";

%feature("docstring")  ps_alignment_n_phones "

Number of phones. ";

%feature("docstring")  ps_alignment_n_states "

Number of states. ";

%feature("docstring")  ps_alignment_words "

Iterate over the alignment starting at the first word. ";

%feature("docstring")  ps_alignment_phones "

Iterate over the alignment starting at the first phone. ";

%feature("docstring")  ps_alignment_states "

Iterate over the alignment starting at the first state. ";

%feature("docstring")  ps_alignment_iter_get "

Get the alignment entry pointed to by an iterator. ";

%feature("docstring")  ps_alignment_iter_free "

Release an iterator before completing all iterations. ";

%feature("docstring")  ps_alignment_iter_goto "

Move alignment iterator to given index. ";

%feature("docstring")  ps_alignment_iter_next "

Move an alignment iterator forward. ";

%feature("docstring")  ps_alignment_iter_prev "

Move an alignment iterator back. ";

%feature("docstring")  ps_alignment_iter_up "

Get a new iterator starting at the parent of the current node. ";

%feature("docstring")  ps_alignment_iter_down "

Get a new iterator starting at the first child of the current node. ";


// File: ps__alignment_8h.xml
%feature("docstring")  ps_alignment_init "

Create a new, empty alignment. ";

%feature("docstring")  ps_alignment_free "

Release an alignment. ";

%feature("docstring")  ps_alignment_add_word "

Append a word. ";

%feature("docstring")  ps_alignment_populate "

Populate lower layers using available word information. ";

%feature("docstring")  ps_alignment_populate_ci "

Populate lower layers using context-independent phones. ";

%feature("docstring")  ps_alignment_propagate "

Propagate timing information up from state sequence. ";

%feature("docstring")  ps_alignment_n_words "

Number of words. ";

%feature("docstring")  ps_alignment_n_phones "

Number of phones. ";

%feature("docstring")  ps_alignment_n_states "

Number of states. ";

%feature("docstring")  ps_alignment_words "

Iterate over the alignment starting at the first word. ";

%feature("docstring")  ps_alignment_phones "

Iterate over the alignment starting at the first phone. ";

%feature("docstring")  ps_alignment_states "

Iterate over the alignment starting at the first state. ";

%feature("docstring")  ps_alignment_iter_get "

Get the alignment entry pointed to by an iterator. ";

%feature("docstring")  ps_alignment_iter_goto "

Move alignment iterator to given index. ";

%feature("docstring")  ps_alignment_iter_next "

Move an alignment iterator forward. ";

%feature("docstring")  ps_alignment_iter_prev "

Move an alignment iterator back. ";

%feature("docstring")  ps_alignment_iter_up "

Get a new iterator starting at the parent of the current node. ";

%feature("docstring")  ps_alignment_iter_down "

Get a new iterator starting at the first child of the current node. ";

%feature("docstring")  ps_alignment_iter_free "

Release an iterator before completing all iterations. ";


// File: ps__lattice_8c.xml
%feature("docstring")  Lattice::link "

Create a directed link between \"from\" and \"to\" nodes, but if a
link already exists, choose one with the best link_scr. ";

%feature("docstring")  Lattice::bypass_fillers "

Bypass filler words. ";

%feature("docstring")  Lattice::delete_node "";

%feature("docstring")  Lattice::remove_dangling_links "";

%feature("docstring")  Lattice::delete_unreachable "

Remove nodes marked as unreachable. ";

%feature("docstring")  Lattice::write "

Write a lattice to disk.

0 for success, <0 on failure. ";

%feature("docstring")  Lattice::write_htk "

Write a lattice to disk in HTK format.

0 for success, <0 on failure. ";

%feature("docstring")  dag_param_read "";

%feature("docstring")  dag_mark_reachable "";

%feature("docstring")  ps_lattice_read "

Read a lattice from a file on disk.

Parameters:
-----------

ps:  Decoder to use for processing this lattice, or NULL.

file:  Path to lattice file.

Newly created lattice, or NULL for failure. ";

%feature("docstring")  Lattice::n_frames "

Get the number of frames in the lattice.

Parameters:
-----------

dag:  The lattice in question.

Number of frames in this lattice. ";

%feature("docstring")  ps_lattice_init_search "

Construct an empty word graph with reference to a search structure. ";

%feature("docstring")  Lattice::retain "

Retain a lattice.

This function retains ownership of a lattice for the caller,
preventing it from being freed automatically. You must call
ps_lattice_free() to free it after having called this function.

pointer to the retained lattice. ";

%feature("docstring")  Lattice::free "

Free a lattice.

new reference count (0 if dag was freed) ";

%feature("docstring")  Lattice::get_logmath "

Get the log-math computation object for this lattice.

The log-math object for this lattice. The lattice retains ownership of
this pointer, so you should not attempt to free it manually. Use
logmath_retain() if you wish to reuse it elsewhere. ";

%feature("docstring")  Lattice::ps_latnode_iter "

Start iterating over nodes in the lattice.

No particular order of traversal is guaranteed, and you should not
depend on this.

Parameters:
-----------

dag:  Lattice to iterate over.

Iterator over lattice nodes. ";

%feature("docstring")  ps_latnode_iter_next "

Move to next node in iteration.

Parameters:
-----------

itor:  Node iterator.

Updated node iterator, or NULL if finished ";

%feature("docstring")  ps_latnode_iter_free "

Stop iterating over nodes.

Parameters:
-----------

itor:  Node iterator. ";

%feature("docstring")  ps_latnode_iter_node "

Get node from iterator. ";

%feature("docstring")  ps_latnode_times "

Get start and end time range for a node.

Parameters:
-----------

node:  Node inquired about.

out_fef:  Output: End frame of first exit from this node.

out_lef:  Output: End frame of last exit from this node.

Start frame for all edges exiting this node. ";

%feature("docstring")  Lattice::ps_latnode_word "

Get word string for this node.

Parameters:
-----------

dag:  Lattice to which node belongs.

node:  Node inquired about.

Word string for this node (possibly a pronunciation variant). ";

%feature("docstring")  Lattice::ps_latnode_baseword "

Get base word string for this node.

Parameters:
-----------

dag:  Lattice to which node belongs.

node:  Node inquired about.

Base word string for this node. ";

%feature("docstring")  Lattice::ps_latnode_prob "

Get best posterior probability and associated acoustic score from a
lattice node.

Parameters:
-----------

dag:  Lattice to which node belongs.

node:  Node inquired about.

out_link:  Output: exit link with highest posterior probability

Posterior probability of the best link exiting this node. Log is
expressed in the log-base used in the decoder. To convert to linear
floating-point, use logmath_exp(ps_lattice_get_logmath(), pprob). ";

%feature("docstring")  ps_latnode_exits "

Iterate over exits from this node.

Parameters:
-----------

node:  Node inquired about.

Iterator over exit links from this node. ";

%feature("docstring")  ps_latnode_entries "

Iterate over entries to this node.

Parameters:
-----------

node:  Node inquired about.

Iterator over entry links to this node. ";

%feature("docstring")  ps_latlink_iter_next "

Get next link from a lattice link iterator.

Parameters:
-----------

itor:  Iterator.

Updated iterator, or NULL if finished. ";

%feature("docstring")  ps_latlink_iter_free "

Stop iterating over links.

Parameters:
-----------

itor:  Link iterator. ";

%feature("docstring")  ps_latlink_iter_link "

Get link from iterator. ";

%feature("docstring")  ps_latlink_times "

Get start and end times from a lattice link.

these are inclusive - i.e. the last frame of this word is ef, not
ef-1.

Parameters:
-----------

link:  Link inquired about.

out_sf:  Output: (optional) start frame of this link.

End frame of this link. ";

%feature("docstring")  ps_latlink_nodes "

Get destination and source nodes from a lattice link.

Parameters:
-----------

link:  Link inquired about

out_src:  Output: (optional) source node.

destination node ";

%feature("docstring")  Lattice::ps_latlink_word "

Get word string from a lattice link.

Parameters:
-----------

dag:  Lattice to which node belongs.

link:  Link inquired about

Word string for this link (possibly a pronunciation variant). ";

%feature("docstring")  Lattice::ps_latlink_baseword "

Get base word string from a lattice link.

Parameters:
-----------

dag:  Lattice to which node belongs.

link:  Link inquired about

Base word string for this link ";

%feature("docstring")  ps_latlink_pred "

Get predecessor link in best path.

Parameters:
-----------

link:  Link inquired about

Best previous link from bestpath search, if any. Otherwise NULL ";

%feature("docstring")  Lattice::ps_latlink_prob "

Get acoustic score and posterior probability from a lattice link.

Parameters:
-----------

dag:  Lattice to which node belongs.

link:  Link inquired about

out_ascr:  Output: (optional) acoustic score.

Posterior probability for this link. Log is expressed in the log-base
used in the decoder. To convert to linear floating-point, use
logmath_exp(ps_lattice_get_logmath(), pprob). ";

%feature("docstring")  Lattice::hyp "

Get hypothesis string after bestpath search. ";

%feature("docstring")  Segment::ps_lattice_compute_lscr "";

%feature("docstring")  Segment::ps_lattice_link2itor "";

%feature("docstring")  Segment::ps_lattice_seg_free "";

%feature("docstring")  Segment::ps_lattice_seg_next "";

%feature("docstring")  Lattice::seg_iter "

Get hypothesis segmentation iterator after bestpath search. ";

%feature("docstring")  Lattice::latlink_list_new "

Create a new lattice link element. ";

%feature("docstring")  Lattice::pushq "

Add an edge to the traversal queue. ";

%feature("docstring")  Lattice::popq "

Remove an edge from the traversal queue. ";

%feature("docstring")  Lattice::delq "

Clear and reset the traversal queue. ";

%feature("docstring")  Lattice::traverse_edges "

Start a forward traversal of edges in a word graph.

A keen eye will notice an inconsistency in this API versus other types
of iterators in PocketSphinx. The reason for this is that the
traversal algorithm is much more efficient when it is able to modify
the lattice structure. Therefore, to avoid giving the impression that
multiple traversals are possible at once, no separate iterator
structure is provided.

Parameters:
-----------

dag:  Lattice to be traversed.

start:  Start node (source) of traversal.

end:  End node (goal) of traversal.

First link in traversal. ";

%feature("docstring")  Lattice::traverse_next "

Get the next link in forward traversal.

Parameters:
-----------

dag:  Lattice to be traversed.

end:  End node (goal) of traversal.

Next link in traversal. ";

%feature("docstring")  Lattice::reverse_edges "

Start a reverse traversal of edges in a word graph.

See ps_lattice_traverse_edges() for why this API is the way it is.

Parameters:
-----------

dag:  Lattice to be traversed.

start:  Start node (goal) of traversal.

end:  End node (source) of traversal.

First link in traversal. ";

%feature("docstring")  Lattice::reverse_next "

Get the next link in reverse traversal.

Parameters:
-----------

dag:  Lattice to be traversed.

start:  Start node (goal) of traversal.

Next link in traversal. ";

%feature("docstring")  Lattice::bestpath "

Do N-Gram based best-path search on a word graph.

This function calculates both the best path as well as the forward
probability used in confidence estimation.

Final link in best path, NULL on error. ";

%feature("docstring")  Lattice::joint "";

%feature("docstring")  Lattice::posterior "

Calculate link posterior probabilities on a word graph.

This function assumes that bestpath search has already been done.

Posterior probability of the utterance as a whole. ";

%feature("docstring")  Lattice::posterior_prune "

Prune all links (and associated nodes) below a certain posterior
probability.

This function assumes that ps_lattice_posterior() has already been
called.

Parameters:
-----------

beam:  Minimum posterior probability for links. This is expressed in
the log- base used in the decoder. To convert from linear floating-
point, use logmath_log(ps_lattice_get_logmath(), prob).

number of arcs removed. ";

%feature("docstring")  best_rem_score "";

%feature("docstring")  path_insert "";

%feature("docstring")  path_extend "";

%feature("docstring")  Lattice::ps_astar_start "

Begin N-Gram based A* search on a word graph.

Parameters:
-----------

sf:  Starting frame for N-best search.

ef:  Ending frame for N-best search, or -1 for last frame.

w1:  First context word, or -1 for none.

w2:  Second context word, or -1 for none.

0 for success, <0 on error. ";

%feature("docstring")  ps_astar_next "

Find next best hypothesis of A* on a word graph.

a complete path, or NULL if no more hypotheses exist. ";

%feature("docstring")  ps_astar_hyp "

Get hypothesis string from A* search. ";

%feature("docstring")  ps_astar_node2itor "";

%feature("docstring")  Segment::ps_astar_seg_free "";

%feature("docstring")  Segment::ps_astar_seg_next "";

%feature("docstring")  ps_astar_seg_iter "

Get hypothesis segmentation from A* search. ";

%feature("docstring")  ps_astar_finish "

Finish N-best search, releasing resources associated with it. ";


// File: ps__lattice__internal_8h.xml
%feature("docstring")  ps_lattice_init_search "

Construct an empty word graph with reference to a search structure. ";

%feature("docstring")  Lattice::bypass_fillers "

Bypass filler words. ";

%feature("docstring")  Lattice::delete_unreachable "

Remove nodes marked as unreachable. ";

%feature("docstring")  Lattice::pushq "

Add an edge to the traversal queue. ";

%feature("docstring")  Lattice::popq "

Remove an edge from the traversal queue. ";

%feature("docstring")  Lattice::delq "

Clear and reset the traversal queue. ";

%feature("docstring")  Lattice::latlink_list_new "

Create a new lattice link element. ";

%feature("docstring")  Lattice::hyp "

Get hypothesis string after bestpath search. ";

%feature("docstring")  Lattice::seg_iter "

Get hypothesis segmentation iterator after bestpath search. ";

%feature("docstring")  Lattice::ps_astar_start "

Begin N-Gram based A* search on a word graph.

Parameters:
-----------

sf:  Starting frame for N-best search.

ef:  Ending frame for N-best search, or -1 for last frame.

w1:  First context word, or -1 for none.

w2:  Second context word, or -1 for none.

0 for success, <0 on error. ";

%feature("docstring")  ps_astar_next "

Find next best hypothesis of A* on a word graph.

a complete path, or NULL if no more hypotheses exist. ";

%feature("docstring")  ps_astar_finish "

Finish N-best search, releasing resources associated with it. ";

%feature("docstring")  ps_astar_hyp "

Get hypothesis string from A* search. ";

%feature("docstring")  ps_astar_seg_iter "

Get hypothesis segmentation from A* search. ";


// File: ps__mllr_8c.xml
%feature("docstring")  ps_mllr_read "

Read a speaker-adaptive linear transform from a file. ";

%feature("docstring")  ps_mllr_retain "

Retain a pointer to a linear transform. ";

%feature("docstring")  ps_mllr_free "

Release a pointer to a linear transform. ";


// File: ptm__mgau_8c.xml
%feature("docstring")  insertion_sort_topn "";

%feature("docstring")  eval_topn "";

%feature("docstring")  insertion_sort_cb "";

%feature("docstring")  eval_cb "";

%feature("docstring")  ptm_mgau_codebook_eval "

Compute top-N densities for active codebooks (and prune) ";

%feature("docstring")  ptm_mgau_calc_cb_active "";

%feature("docstring")  ptm_mgau_senone_eval "

Compute senone scores from top-N densities for active codebooks. ";

%feature("docstring")  ptm_mgau_frame_eval "

Compute senone scores for the active senones. ";

%feature("docstring")  read_sendump "";

%feature("docstring")  read_mixw "";

%feature("docstring")  ptm_mgau_init "";

%feature("docstring")  ptm_mgau_mllr_transform "";

%feature("docstring")  ptm_mgau_free "";


// File: ptm__mgau_8h.xml
%feature("docstring")  ptm_mgau_init "";

%feature("docstring")  ptm_mgau_free "";

%feature("docstring")  ptm_mgau_frame_eval "

Compute senone scores for the active senones. ";

%feature("docstring")  ptm_mgau_mllr_transform "";


// File: s2__semi__mgau_8c.xml
%feature("docstring")  eval_topn "";

%feature("docstring")  eval_cb "";

%feature("docstring")  mgau_dist "";

%feature("docstring")  mgau_norm "";

%feature("docstring")  get_scores_8b_feat_6 "";

%feature("docstring")  get_scores_8b_feat_5 "";

%feature("docstring")  get_scores_8b_feat_4 "";

%feature("docstring")  get_scores_8b_feat_3 "";

%feature("docstring")  get_scores_8b_feat_2 "";

%feature("docstring")  get_scores_8b_feat_1 "";

%feature("docstring")  get_scores_8b_feat_any "";

%feature("docstring")  get_scores_8b_feat "";

%feature("docstring")  get_scores_8b_feat_all "";

%feature("docstring")  get_scores_4b_feat_6 "";

%feature("docstring")  get_scores_4b_feat_5 "";

%feature("docstring")  get_scores_4b_feat_4 "";

%feature("docstring")  get_scores_4b_feat_3 "";

%feature("docstring")  get_scores_4b_feat_2 "";

%feature("docstring")  get_scores_4b_feat_1 "";

%feature("docstring")  get_scores_4b_feat_any "";

%feature("docstring")  get_scores_4b_feat "";

%feature("docstring")  get_scores_4b_feat_all "";

%feature("docstring")  s2_semi_mgau_frame_eval "";

%feature("docstring")  read_sendump "";

%feature("docstring")  read_mixw "";

%feature("docstring")  split_topn "";

%feature("docstring")  s2_semi_mgau_init "";

%feature("docstring")  s2_semi_mgau_mllr_transform "";

%feature("docstring")  s2_semi_mgau_free "";


// File: s2__semi__mgau_8h.xml
%feature("docstring")  s2_semi_mgau_init "";

%feature("docstring")  s2_semi_mgau_free "";

%feature("docstring")  s2_semi_mgau_frame_eval "";

%feature("docstring")  s2_semi_mgau_mllr_transform "";


// File: s3types_8h.xml


// File: state__align__search_8c.xml
%feature("docstring")  state_align_search_start "";

%feature("docstring")  renormalize_hmms "";

%feature("docstring")  evaluate_hmms "";

%feature("docstring")  prune_hmms "";

%feature("docstring")  phone_transition "";

%feature("docstring")  extend_tokenstack "";

%feature("docstring")  record_transitions "";

%feature("docstring")  state_align_search_step "";

%feature("docstring")  state_align_search_finish "";

%feature("docstring")  state_align_search_reinit "";

%feature("docstring")  state_align_search_free "";

%feature("docstring")  state_align_search_init "";


// File: state__align__search_8h.xml
%feature("docstring")  state_align_search_init "";


// File: tied__mgau__common_8h.xml
%feature("docstring")  fast_logmath_add "

Quickly log-add two negated log probabilities.

Parameters:
-----------

lmath:  The log-math object

mlx:  A negative log probability (0 < mlx < 255)

mly:  A negative log probability (0 < mly < 255)

-log(exp(-mlx)+exp(-mly))  We can do some extra-fast log addition
since we know that mixw+ascr is always less than 256 and hence x-y is
also always less than 256. This relies on some cooperation from
logmath_t which will never produce a logmath table smaller than 256
entries.

Note that the parameters are negated log probabilities (and hence, are
positive numbers), as is the return value. This is the key to the
\"fastness\" of this function. ";


// File: tmat_8c.xml
%feature("docstring")  tmat_chk_uppertri "

Checks that no transition matrix in the given object contains backward
arcs.

0 if successful, -1 if check failed. ";

%feature("docstring")  tmat_chk_1skip "

Checks that transition matrix arcs in the given object skip over at
most 1 state.

0 if successful, -1 if check failed. ";

%feature("docstring")  tmat_dump "

Dumping the transition matrix for debugging. ";

%feature("docstring")  tmat_init "

Initialize transition matrix. ";

%feature("docstring")  tmat_report "

Report the detail of the transition matrix structure. ";

%feature("docstring")  tmat_free "

RAH, add code to remove memory allocated by tmat_init. ";


// File: tmat_8h.xml
%feature("docstring")  tmat_init "

Initialize transition matrix. ";

%feature("docstring")  tmat_dump "

Dumping the transition matrix for debugging. ";

%feature("docstring")  tmat_free "

RAH, add code to remove memory allocated by tmat_init. ";

%feature("docstring")  tmat_report "

Report the detail of the transition matrix structure. ";


// File: vector_8c.xml
%feature("docstring")  vector_sum_norm "";

%feature("docstring")  vector_floor "";

%feature("docstring")  vector_nz_floor "";

%feature("docstring")  vector_print "";

%feature("docstring")  vector_is_zero "";


// File: vector_8h.xml
%feature("docstring")  vector_floor "";

%feature("docstring")  vector_nz_floor "";

%feature("docstring")  vector_sum_norm "";

%feature("docstring")  vector_print "";

%feature("docstring")  vector_is_zero "";


// File: dir_d44c64559bbebec7f509842c48db8b23.xml


// File: dir_8d034a1e03e98d9b7ac467250bbebdea.xml


// File: dir_68267d1309a1af8e8297ef4c3efbcdba.xml


// File: indexpage.xml


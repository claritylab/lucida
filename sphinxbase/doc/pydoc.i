
// File: index.xml

// File: cmd__ln_8h.xml
/*  Values for arg_t::type  */

%feature("docstring")  Config::init "

Create a cmd_ln_t from NULL-terminated list of arguments.

This function creates a cmd_ln_t from a NULL-terminated list of
argument strings. For example, to create the equivalent of passing
\"-hmm foodir -dsratio 2 -lm bar.lm\" on the command-line:

config = cmd_ln_init(NULL, defs, TRUE, \"-hmm\", \"foodir\",
\"-dsratio\", \"2\", \"-lm\", \"bar.lm\", NULL);

Note that for simplicity, all arguments are passed as strings,
regardless of the actual underlying type.

Parameters:
-----------

inout_cmdln:  Previous command-line to update, or NULL to create a new
one.

defn:  Array of argument name definitions, or NULL to allow any
arguments.

strict:  Whether to fail on duplicate or unknown arguments.

A cmd_ln_t* containing the results of command line parsing, or NULL on
failure. ";

%feature("docstring")  Config::retain "

Retain ownership of a command-line argument set.

pointer to retained command-line argument set. ";

%feature("docstring")  Config::free_r "

Release a command-line argument set and all associated strings.

new reference count (0 if freed completely) ";

%feature("docstring")  Config::parse_r "

Parse a list of strings into argumetns.

Parse the given list of arguments (name-value pairs) according to the
given definitions. Argument values can be retrieved in future using
cmd_ln_access(). argv[0] is assumed to be the program name and
skipped. Any unknown argument name causes a fatal error. The routine
also prints the prevailing argument values (to stderr) after parsing.

It is currently assumed that the strings in argv are allocated
statically, or at least that they will be valid as long as the
cmd_ln_t returned from this function. Unpredictable behaviour will
result if they are freed or otherwise become invalidated.

A cmd_ln_t containing the results of command line parsing, or NULL on
failure. ";

%feature("docstring")  Config::parse_file_r "

Parse an arguments file by deliminating on \" \\\\r\\\\t\\\\n\" and
putting each tokens into an argv[] for cmd_ln_parse().

A cmd_ln_t containing the results of command line parsing, or NULL on
failure. ";

%feature("docstring")  Config::access_r "

Access the generic type union for a command line argument. ";

%feature("docstring")  Config::str_r "

Retrieve a string from a command-line object.

The command-line object retains ownership of this string, so you
should not attempt to free it manually.

Parameters:
-----------

cmdln:  Command-line object.

name:  the command-line flag to retrieve.

the string value associated with name, or NULL if name does not exist.
You must use cmd_ln_exists_r() to distinguish between cases where a
value is legitimately NULL and where the corresponding flag is
unknown. ";

%feature("docstring")  Config::str_list_r "

Retrieve an array of strings from a command-line object.

The command-line object retains ownership of this array, so you should
not attempt to free it manually.

Parameters:
-----------

cmdln:  Command-line object.

name:  the command-line flag to retrieve.

the array of strings associated with name, or NULL if name does not
exist. You must use cmd_ln_exists_r() to distinguish between cases
where a value is legitimately NULL and where the corresponding flag is
unknown. ";

%feature("docstring")  Config::int_r "

Retrieve an integer from a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  the command-line flag to retrieve.

the integer value associated with name, or 0 if name does not exist.
You must use cmd_ln_exists_r() to distinguish between cases where a
value is legitimately zero and where the corresponding flag is
unknown. ";

%feature("docstring")  Config::float_r "

Retrieve a floating-point number from a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  the command-line flag to retrieve.

the float value associated with name, or 0.0 if name does not exist.
You must use cmd_ln_exists_r() to distinguish between cases where a
value is legitimately zero and where the corresponding flag is
unknown. ";

%feature("docstring")  Config::set_str_r "

Set a string in a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  The command-line flag to set.

str:  String value to set. The command-line object does not retain
ownership of this pointer. ";

%feature("docstring")  Config::set_int_r "

Set an integer in a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  The command-line flag to set.

iv:  Integer value to set. ";

%feature("docstring")  Config::set_float_r "

Set a floating-point number in a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  The command-line flag to set.

fv:  Integer value to set. ";

%feature("docstring")  Config::exists_r "

Re-entrant version of cmd_ln_exists().

True if the command line argument exists (i.e. it was one of the
arguments defined in the call to cmd_ln_parse_r(). ";

%feature("docstring")  Config::print_help_r "

Print a help message listing the valid argument names, and the
associated attributes as given in defn.

Parameters:
-----------

fp:  output stream

defn:  Array of argument name definitions. ";

%feature("docstring")  cmd_ln_parse "

Non-reentrant version of cmd_ln_parse().

Deprecated This is deprecated in favor of the re-entrant API function
cmd_ln_parse_r(). 0 if successful, <0 if error. ";

%feature("docstring")  cmd_ln_parse_file "

Parse an arguments file by deliminating on \" \\\\r\\\\t\\\\n\" and
putting each tokens into an argv[] for cmd_ln_parse().

Deprecated This is deprecated in favor of the re-entrant API function
cmd_ln_parse_file_r().

0 if successful, <0 on error. ";

%feature("docstring")  cmd_ln_appl_enter "

Old application initialization routine for Sphinx3 code.

Deprecated This is deprecated in favor of the re-entrant API. ";

%feature("docstring")  cmd_ln_appl_exit "

Finalization routine corresponding to cmd_ln_appl_enter().

Deprecated This is deprecated in favor of the re-entrant API. ";

%feature("docstring")  cmd_ln_get "

Retrieve the global cmd_ln_t object used by non-re-entrant functions.

Deprecated This is deprecated in favor of the re-entrant API. global
cmd_ln_t object. ";

%feature("docstring")  cmd_ln_free "

Free the global command line, if any exists.

Deprecated Use the re-entrant API instead. ";


// File: fe_8h.xml
%feature("docstring")  fe_init_auto "

Initialize a front-end object from global command-line.

This is equivalent to calling fe_init_auto_r(cmd_ln_get()).

Newly created front-end object. ";

%feature("docstring")  fe_get_args "

Get the default set of arguments for fe_init_auto_r().

Pointer to an argument structure which can be passed to cmd_ln_init()
in friends to create argument structures for fe_init_auto_r(). ";

%feature("docstring")  Config::fe_init_auto_r "

Initialize a front-end object from a command-line parse.

Parameters:
-----------

config:  Command-line object, as returned by cmd_ln_parse_r() or
cmd_ln_parse_file(). Ownership of this object is claimed by the fe_t,
so you must not attempt to free it manually. Use cmd_ln_retain() if
you wish to reuse it.

Newly created front-end object. ";

%feature("docstring")  FrontEnd::get_config "

Retrieve the command-line object used to initialize this front-end.

command-line object for this front-end. This pointer is retained by
the fe_t, so you should not attempt to free it manually. ";

%feature("docstring")  FrontEnd::start_utt "

Start processing an utterance.

0 for success, <0 for error (see enum fe_error_e) ";

%feature("docstring")  FrontEnd::get_output_size "

Get the dimensionality of the output of this front-end object.

This is guaranteed to be the number of values in one frame of output
from fe_end_utt() and fe_process_frames(). It is usually the number of
MFCC coefficients, but it might be the number of log-spectrum bins, if
the -logspec or -smoothspec options to fe_init_auto() were true.

Dimensionality of front-end output. ";

%feature("docstring")  FrontEnd::get_input_size "

Get the dimensionality of the input to this front-end object.

This function retrieves the number of input samples consumed by one
frame of processing. To obtain one frame of output, you must have at
least *out_frame_size samples. To obtain N frames of output, you must
have at least (N-1) * *out_frame_shift + *out_frame_size input
samples.

Parameters:
-----------

out_frame_shift:  Output: Number of samples between each frame start.

out_frame_size:  Output: Number of samples in each frame. ";

%feature("docstring")  FrontEnd::get_vad_state "

Get vad state for the last processed frame.

1 if speech, 0 if silence ";

%feature("docstring")  FrontEnd::end_utt "

Finish processing an utterance.

This function also collects any remaining samples and calculates a
final cepstral vector. If there are overflow samples remaining, it
will pad with zeros to make a complete frame.

Parameters:
-----------

fe:  Front-end object.

out_cepvector:  Buffer to hold a residual cepstral vector, or NULL if
you wish to ignore it. Must be large enough

out_nframes:  Number of frames of residual cepstra created (either 0
or 1).

0 for success, <0 for error (see enum fe_error_e) ";

%feature("docstring")  FrontEnd::retain "

Retain ownership of a front end object.

pointer to the retained front end. ";

%feature("docstring")  FrontEnd::free "

Free the front end.

Releases resources associated with the front-end object.

new reference count (0 if freed completely) ";

%feature("docstring")  FrontEnd::process_frames_ext "";

%feature("docstring")  FrontEnd::process_frames "

Process a block of samples.

This function generates up to *inout_nframes of features, or as many
as can be generated from *inout_nsamps samples.

On exit, the inout_spch, inout_nsamps, and inout_nframes parameters
are updated to point to the remaining sample data, the number of
remaining samples, and the number of frames processed, respectively.
This allows you to call this repeatedly to process a large block of
audio in small (say, 5-frame) chunks:

int16 *bigbuf, *p; mfcc_t **cepstra; int32 nsamps; int32 nframes = 5;

cepstra = (mfcc_t **) ckd_calloc_2d(nframes, fe_get_output_size(fe),
sizeof(**cepstra)); p = bigbuf; while (nsamps) { nframes = 5;
fe_process_frames(fe, &p, &nsamps, cepstra, &nframes); // Now do
something with these frames... if (nframes) do_some_stuff(cepstra,
nframes); }

Parameters:
-----------

inout_spch:  Input: Pointer to pointer to speech samples (signed
16-bit linear PCM). Output: Pointer to remaining samples.

inout_nsamps:  Input: Pointer to maximum number of samples to process.
Output: Number of samples remaining in input buffer.

buf_cep:  Two-dimensional buffer (allocated with ckd_calloc_2d())
which will receive frames of output data. If NULL, no actual
processing will be done, and the maximum number of output frames which
would be generated is returned in *inout_nframes.

inout_nframes:  Input: Pointer to maximum number of frames to
generate. Output: Number of frames actually generated.

0 for success, <0 for failure (see enum fe_error_e) ";

%feature("docstring")  FrontEnd::process_utt "

Process a block of samples, returning as many frames as possible.

This function processes all the samples in a block of data and returns
a newly allocated block of feature vectors. This block needs to be
freed with fe_free_2d() after use.

It is possible for there to be some left-over data which could not fit
in a complete frame. This data can be processed with fe_end_utt().

This function is deprecated in favor of fe_process_frames().

0 for success, <0 for failure (see enum fe_error_e) ";

%feature("docstring")  fe_free_2d "

Free the output pointer returned by fe_process_utt(). ";

%feature("docstring")  FrontEnd::mfcc_to_float "

Convert a block of mfcc_t to float32 (can be done in-place) ";

%feature("docstring")  FrontEnd::float_to_mfcc "

Convert a block of float32 to mfcc_t (can be done in-place) ";

%feature("docstring")  FrontEnd::logspec_to_mfcc "

Process one frame of log spectra into MFCC using discrete cosine
transform.

This uses a variant of the DCT-II where the first frequency bin is
scaled by 0.5. Unless somebody misunderstood the DCT-III equations and
thought that's what they were implementing here, this is ostensibly
done to account for the symmetry properties of the DCT-II versus the
DFT - the first coefficient of the input is assumed to be repeated in
the negative frequencies, which is not the case for the DFT. (This
begs the question, why not just use the DCT-I, since it has the
appropriate symmetry properties...) Moreover, this is bogus since the
mel-frequency bins on which we are doing the DCT don't extend to the
edge of the DFT anyway.

This also means that the matrix used in computing this DCT can not be
made orthogonal, and thus inverting the transform is difficult.
Therefore if you want to do cepstral smoothing or have some other
reason to invert your MFCCs, use fe_logspec_dct2() and its inverse
fe_logspec_dct3() instead.

Also, it normalizes by 1/nfilt rather than 2/nfilt, for some reason.
";

%feature("docstring")  FrontEnd::logspec_dct2 "

Convert log spectra to MFCC using DCT-II.

This uses the \"unitary\" form of the DCT-II, i.e. with a scaling
factor of sqrt(2/N) and a \"beta\" factor of sqrt(1/2) applied to the
cos(0) basis vector (i.e. the one corresponding to the DC coefficient
in the output). ";

%feature("docstring")  FrontEnd::mfcc_dct3 "

Convert MFCC to log spectra using DCT-III.

This uses the \"unitary\" form of the DCT-III, i.e. with a scaling
factor of sqrt(2/N) and a \"beta\" factor of sqrt(1/2) applied to the
cos(0) basis vector (i.e. the one corresponding to the DC coefficient
in the input). ";


// File: feat_8h.xml
%feature("docstring")  parse_subvecs "

Parse subvector specification string.

Format of specification: '/' separated list of subvectors

each subvector is a ',' separated list of subranges

each subrange is a single<number> or<number>-<number> (inclusive),
where<number> is a feature vector dimension specifier.  E.g.,
\"24,0-11/25,12-23/26,27-38\" has: 3 subvectors

the 1st subvector has feature dims: 24, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
10, and 11.

etc.

Parameters:
-----------

str:  subvector specification string.

allocated 2-D array of subvector specs (free with subvecs_free()). If
there are N subvectors specified, subvec[N] = NULL; and each
subvec[0]..subvec[N-1] is -1 terminated vector of feature dims. ";

%feature("docstring")  subvecs_free "

Free array of subvector specs. ";

%feature("docstring")  Feature::array_alloc "

Allocate an array to hold several frames worth of feature vectors.

The returned value is the mfcc_t ***data array, organized as follows:

data[0][0] = frame 0 stream 0 vector, data[0][1] = frame 0 stream 1
vector, ...

data[1][0] = frame 1 stream 0 vector, data[0][1] = frame 1 stream 1
vector, ...

data[2][0] = frame 2 stream 0 vector, data[0][1] = frame 2 stream 1
vector, ...

...

NOTE: For I/O convenience, the entire data area is allocated as one
contiguous block. pointer to the allocated space if successful, NULL
if any error. ";

%feature("docstring")  Feature::array_realloc "

Realloate the array of features.

Requires us to know the old size ";

%feature("docstring")  feat_array_free "

Free a buffer allocated with feat_array_alloc() ";

%feature("docstring")  feat_init "

Initialize feature module to use the selected type of feature stream.

One-time only initialization at the beginning of the program. Input
type is a string defining the kind of input->feature conversion
desired:

\"s2_4x\": s2mfc->Sphinx-II 4-feature stream,

\"1s_c_d_dd\": s2mfc->Sphinx 3.x single feature stream,

\"s3_1x39\": s2mfc->Sphinx 3.0 single feature stream,

\"n1,n2,n3,...\": Explicit feature vector layout spec. with comma-
separated feature stream lengths. In this case, the input data is
already in the feature format and there is no conversion necessary.

( feat_t *) descriptor if successful, NULL if error. Caller must not
directly modify the contents of the returned value. ";

%feature("docstring")  Feature::read_lda "

Add an LDA transformation to the feature module from a file.

0 for success or -1 if reading the LDA file failed. ";

%feature("docstring")  Feature::lda_transform "

Transform a block of features using the feature module's LDA
transform. ";

%feature("docstring")  Feature::set_subvecs "

Add a subvector specification to the feature module.

The subvector splitting will be performed after dynamic feature
computation, CMN, AGC, and any LDA transformation. The number of
streams in the dynamic feature type must be one, as with LDA.

After adding a subvector specification, the output of feature
computation will be split into multiple subvectors, and
feat_array_alloc() will allocate pointers accordingly. The number of
streams will remain the

Parameters:
-----------

fcb:  the feature descriptor.

subvecs:  subvector specification. This pointer is retained by the
feat_t and should not be freed manually.

0 for success or -1 if the subvector specification was invalid. ";

%feature("docstring")  Feature::print "

Print the given block of feature vectors to the given FILE. ";

%feature("docstring")  Feature::s2mfc2feat "

Read a specified MFC file (or given segment within it), perform
CMN/AGC as indicated by fcb, and compute feature vectors.

Feature vectors are computed for the entire segment specified, by
including additional surrounding or padding frames to accommodate the
feature windows.

Number of frames of feature vectors computed if successful; -1 if any
error. If feat is NULL, then no actual computation will be done, and
the number of frames which must be allocated will be returned.  A note
on how the file path is constructed: If the control file already
specifies extension or absolute path, then these are not applied. The
default extension is defined by the application. ";

%feature("docstring")  Feature::s2mfc2live "

Feature computation routine for live mode decoder.

This function computes features for blocks of incoming data. It
retains an internal buffer for computing deltas, which means that the
number of output frames will not necessarily equal the number of input
frames.

It is very important to realize that the number of output frames can
be greater than the number of input frames, specifically when endutt
is true. It is guaranteed to never exceed *inout_ncep +
feat_window_size(fcb). You MUST have allocated at least that many
frames in ofeat, or you will experience a buffer overflow.

If beginutt and endutt are both true, CMN_CURRENT and AGC_MAX will be
done. Otherwise only CMN_PRIOR and AGC_EMAX will be done.

If beginutt is false, endutt is true, and the number of input frames
exceeds the input size, then end-of-utterance processing won't
actually be done. This condition can easily be checked, because
*inout_ncep will equal the return value on exit, and will also be
smaller than the value of *inout_ncep on entry.

The number of output frames actually computed. ";

%feature("docstring")  Feature::retain "

Retain ownership of feat_t.

pointer to retained feat_t. ";

%feature("docstring")  Feature::free "

Release resource associated with feat_t.

new reference count (0 if freed) ";

%feature("docstring")  Feature::report "

Report the feat_t data structure. ";


// File: fsg__model_8h.xml
%feature("docstring")  fsg_model_init "

Create a new FSG. ";

%feature("docstring")  fsg_model_readfile "

Read a word FSG from the given file and return a pointer to the
structure created.

Return NULL if any error occurred.

File format:

Any number of comment lines; ignored   FSG_BEGIN [<fsgname>]   N
<#states>   S <start-state ID>   F <final-state ID>   T <from-state>
<to-state> <prob> [<word-string>]   T ...   ... (any number of state
transitions)   FSG_END   Any number of comment lines; ignored

The FSG spec begins with the line containing the keyword FSG_BEGIN. It
has an optional fsg name string. If not present, the FSG has the empty
string as its name.

Following the FSG_BEGIN declaration is the number of states, the start
state, and the final state, each on a separate line. States are
numbered in the range [0 .. <numberofstate>-1].

These are followed by all the state transitions, each on a separate
line, and terminated by the FSG_END line. A state transition has the
given probability of being taken, and emits the given word. The word
emission is optional; if word-string omitted, it is an epsilon or null
transition.

Comments can also be embedded within the FSG body proper (i.e. between
FSG_BEGIN and FSG_END): any line with a # character in col 1 is
treated as a comment line.

Return value: a new fsg_model_t structure if the file is successfully
read, NULL otherwise. ";

%feature("docstring")  fsg_model_read "

Like fsg_model_readfile(), but from an already open stream. ";

%feature("docstring")  FsgModel::retain "

Retain ownership of an FSG.

Pointer to retained FSG. ";

%feature("docstring")  FsgModel::free "

Free the given word FSG.

new reference count (0 if freed completely) ";

%feature("docstring")  FsgModel::word_add "

Add a word to the FSG vocabulary.

Word ID for this new word. ";

%feature("docstring")  FsgModel::word_id "

Look up a word in the FSG vocabulary.

Word ID for this word ";

%feature("docstring")  FsgModel::trans_add "

Add the given transition to the FSG transition matrix.

Duplicates (i.e., two transitions between the same states, with the
same word label) are flagged and only the highest prob retained. ";

%feature("docstring")  FsgModel::null_trans_add "

Add a null transition between the given states.

There can be at most one null transition between the given states;
duplicates are flagged and only the best prob retained. Transition
probs must be <= 1 (i.e., logprob <= 0).

1 if a new transition was added, 0 if the prob of an existing
transition was upgraded; -1 if nothing was changed. ";

%feature("docstring")  FsgModel::tag_trans_add "

Add a \"tag\" transition between the given states.

A \"tag\" transition is a null transition with a non-null word ID,
which corresponds to a semantic tag or other symbol to be output when
this transition is taken.

As above, there can be at most one null or tag transition between the
given states; duplicates are flagged and only the best prob retained.
Transition probs must be <= 1 (i.e., logprob <= 0).

1 if a new transition was added, 0 if the prob of an existing
transition was upgraded; -1 if nothing was changed. ";

%feature("docstring")  FsgModel::null_trans_closure "

Obtain transitive closure of null transitions in the given FSG.

Parameters:
-----------

nulls:  List of null transitions, or NULL to find them automatically.

Updated list of null transitions. ";

%feature("docstring")  FsgModel::trans "

Get the list of transitions (if any) from state i to j. ";

%feature("docstring")  FsgModel::arcs "

Get an iterator over the outgoing transitions from state i. ";

%feature("docstring")  fsg_arciter_get "

Get the current arc from the arc iterator. ";

%feature("docstring")  fsg_arciter_next "

Move the arc iterator forward. ";

%feature("docstring")  fsg_arciter_free "

Free the arc iterator (early termination) ";

%feature("docstring")  FsgModel::null_trans "

Get the null transition (if any) from state i to j. ";

%feature("docstring")  FsgModel::add_silence "

Add silence word transitions to each state in given FSG.

Parameters:
-----------

state:  state to add a self-loop to, or -1 for all states.

silprob:  probability of silence transition. ";

%feature("docstring")  FsgModel::add_alt "

Add alternate pronunciation transitions for a word in given FSG. ";

%feature("docstring")  FsgModel::write "

Write FSG to a file. ";

%feature("docstring")  FsgModel::writefile "

Write FSG to a file. ";

%feature("docstring")  FsgModel::write_fsm "

Write FSG to a file in AT&T FSM format. ";

%feature("docstring")  FsgModel::writefile_fsm "

Write FSG to a file in AT&T FSM format. ";

%feature("docstring")  FsgModel::write_symtab "

Write FSG symbol table to a file (for AT&T FSM) ";

%feature("docstring")  FsgModel::writefile_symtab "

Write FSG symbol table to a file (for AT&T FSM) ";


// File: jsgf_8h.xml
%feature("docstring")  Jsgf::grammar_new "

Create a new JSGF grammar.

Parameters:
-----------

parent:  optional parent grammar for this one (NULL, usually).

new JSGF grammar object, or NULL on failure. ";

%feature("docstring")  Jsgf::parse_file "

Parse a JSGF grammar from a file.

Parameters:
-----------

filename:  the name of the file to parse.

parent:  optional parent grammar for this one (NULL, usually).

new JSGF grammar object, or NULL on failure. ";

%feature("docstring")  Jsgf::grammar_name "

Get the grammar name from the file. ";

%feature("docstring")  Jsgf::grammar_free "

Free a JSGF grammar. ";

%feature("docstring")  Jsgf::rule_iter "

Get an iterator over all rules in a grammar. ";

%feature("docstring")  Jsgf::get_rule "

Get a rule by name from a grammar. ";

%feature("docstring")  jsgf_rule_name "

Get the rule name from a rule. ";

%feature("docstring")  jsgf_rule_public "

Test if a rule is public or not. ";

%feature("docstring")  Jsgf::build_fsg "

Build a Sphinx FSG object from a JSGF rule. ";

%feature("docstring")  Jsgf::build_fsg_raw "

Build a Sphinx FSG object from a JSGF rule.

This differs from jsgf_build_fsg() in that it does not do closure on
epsilon transitions or any other postprocessing. For the time being
this is necessary in order to write it to a file - the FSG code will
be fixed soon. ";

%feature("docstring")  jsgf_read_file "

Read JSGF from file and return FSG object from it.

This function looks for a first public rule in jsgf and constructs
JSGF from it. ";

%feature("docstring")  Jsgf::write_fsg "

Convert a JSGF rule to Sphinx FSG text form.

This does a direct conversion without doing transitive closure on null
transitions and so forth. ";


// File: ngram__model_8h.xml
%feature("docstring")  Config::ngram_model_read "

Read an N-Gram model from a file on disk.

Parameters:
-----------

config:  Optional pointer to a set of command-line arguments.
Recognized arguments are:

-mmap (boolean) whether to use memory-mapped I/O

-lw (float32) language weight to apply to the model

-wip (float32) word insertion penalty to apply to the model

-uw (float32) unigram weight to apply to the model

Parameters:
-----------

file_name:  path to the file to read.

file_type:  type of the file, or NGRAM_AUTO to determine
automatically.

lmath:  Log-math parameters to use for probability calculations.
Ownership of this object is assumed by the newly created
ngram_model_t, and you should not attempt to free it manually. If you
wish to reuse it elsewhere, you must retain it with logmath_retain().

newly created ngram_model_t. ";

%feature("docstring")  NGramModel::write "

Write an N-Gram model to disk.

0 for success, <0 on error ";

%feature("docstring")  ngram_file_name_to_type "

Guess the file type for an N-Gram model from the filename.

the guessed file type, or NGRAM_INVALID if none could be guessed. ";

%feature("docstring")  ngram_str_to_type "

Get the N-Gram file type from a string.

file type, or NGRAM_INVALID if no such file type exists. ";

%feature("docstring")  ngram_type_to_str "

Get the canonical name for an N-Gram file type.

read-only string with the name for this file type, or NULL if no such
type exists. ";

%feature("docstring")  NGramModel::retain "

Retain ownership of an N-Gram model.

Pointer to retained model. ";

%feature("docstring")  NGramModel::free "

Release memory associated with an N-Gram model.

new reference count (0 if freed completely) ";

%feature("docstring")  NGramModel::recode "

Re-encode word strings in an N-Gram model.

Character set names are the same as those passed to iconv(1). If your
system does not have iconv, this function may fail. Also, because all
file formats consist of 8-bit character streams, attempting to convert
to or from UTF-16 (or any other encoding which contains null bytes) is
a recipe for total desaster.

We have no interest in supporting UTF-16, so don't ask.

Note that this does not affect any pronunciation dictionary you might
currently be using in conjunction with this N-Gram model, so the
effect of calling this during decoding is undefined. That's a bug! ";

%feature("docstring")  NGramModel::casefold "

Case-fold word strings in an N-Gram model.

WARNING: This is not Unicode aware, so any non-ASCII characters will
not be converted. ";

%feature("docstring")  NGramModel::apply_weights "

Apply a language weight, insertion penalty, and unigram weight to a
language model.

This will change the values output by ngram_score() and friends. This
is done for efficiency since in decoding, these are the only values we
actually need. Call ngram_prob() if you want the \"raw\" N-Gram
probability estimate.

To remove all weighting, call ngram_apply_weights(model, 1.0, 1.0,
1.0). ";

%feature("docstring")  NGramModel::get_weights "

Get the current weights from a language model.

Parameters:
-----------

model:  The model in question.

out_log_wip:  Output: (optional) logarithm of word insertion penalty.

out_log_uw:  Output: (optional) logarithm of unigram weight.

language weight. ";

%feature("docstring")  NGramModel::ngram_score "

Get the score (scaled, interpolated log-probability) for a general
N-Gram.

The argument list consists of the history words (as null-terminated
strings) of the N-Gram, in reverse order, followed by NULL. Therefore,
if you wanted to get the N-Gram score for \"a whole joy\", you would
call:

score = ngram_score(model, \"joy\", \"whole\", \"a\", NULL);

This is not the function to use in decoding, because it has some
overhead for looking up words. Use ngram_ng_score(), ngram_tg_score(),
or ngram_bg_score() instead. In the future there will probably be a
version that takes a general language model state object, to support
suffix-array LM and things like that.

If one of the words is not in the LM's vocabulary, the result will
depend on whether this is an open or closed vocabulary language model.
For an open-vocabulary model, unknown words are all mapped to the
unigram <UNK> which has a non-zero probability and also participates
in higher-order N-Grams. Therefore, you will get a score of some sort
in this case.

For a closed-vocabulary model, unknown words are impossible and thus
have zero probability. Therefore, if word is unknown, this function
will return a \"zero\" log-probability, i.e. a large negative number.
To obtain this number for comparison, call ngram_zero(). ";

%feature("docstring")  NGramModel::ngram_tg_score "

Quick trigram score lookup. ";

%feature("docstring")  NGramModel::ngram_bg_score "

Quick bigram score lookup. ";

%feature("docstring")  NGramModel::ngram_ng_score "

Quick general N-Gram score lookup. ";

%feature("docstring")  NGramModel::ngram_probv "

Get the \"raw\" log-probability for a general N-Gram.

This returns the log-probability of an N-Gram, as defined in the
language model file, before any language weighting, interpolation, or
insertion penalty has been applied.

When backing off to a unigram from a bigram or trigram, the unigram
weight (interpolation with uniform) is not removed. ";

%feature("docstring")  NGramModel::ngram_prob "

Get the \"raw\" log-probability for a general N-Gram.

This returns the log-probability of an N-Gram, as defined in the
language model file, before any language weighting, interpolation, or
insertion penalty has been applied.

When backing off to a unigram from a bigram or trigram, the unigram
weight (interpolation with uniform) is not removed. ";

%feature("docstring")  NGramModel::ngram_ng_prob "

Quick \"raw\" probability lookup for a general N-Gram.

See documentation for ngram_ng_score() and ngram_apply_weights() for
an explanation of this. ";

%feature("docstring")  NGramModel::ngram_score_to_prob "

Convert score to \"raw\" log-probability.

The unigram weight (interpolation with uniform) is not removed, since
there is no way to know which order of N-Gram generated score.

Parameters:
-----------

model:  The N-Gram model from which score was obtained.

score:  The N-Gram score to convert

The raw log-probability value. ";

%feature("docstring")  NGramModel::ngram_wid "

Look up numerical word ID. ";

%feature("docstring")  NGramModel::ngram_word "

Look up word string for numerical word ID. ";

%feature("docstring")  NGramModel::ngram_unknown_wid "

Get the unknown word ID for a language model.

Language models can be either \"open vocabulary\" or \"closed
vocabulary\". The difference is that the former assigns a fixed non-
zero unigram probability to unknown words, while the latter does not
allow unknown words (or, equivalently, it assigns them zero
probability). If this is a closed vocabulary model, this function will
return NGRAM_INVALID_WID.

The ID for the unknown word, or NGRAM_INVALID_WID if none exists. ";

%feature("docstring")  NGramModel::ngram_zero "

Get the \"zero\" log-probability value for a language model. ";

%feature("docstring")  NGramModel::get_size "

Get the order of the N-gram model (i.e.

the \"N\" in \"N-gram\") ";

%feature("docstring")  NGramModel::get_counts "

Get the counts of the various N-grams in the model. ";

%feature("docstring")  NGramModel::mgrams "

Iterate over all M-grams.

Parameters:
-----------

model:  Language model to query.

m:  Order of the M-Grams requested minus one (i.e. order of the
history)

An iterator over the requested M, or NULL if no N-grams of order M+1
exist. ";

%feature("docstring")  NGramModel::ngram_iter "

Get an iterator over M-grams pointing to the specified M-gram. ";

%feature("docstring")  NGramModel::ngram_ng_iter "

Get an iterator over M-grams pointing to the specified M-gram. ";

%feature("docstring")  ngram_iter_get "

Get information from the current M-gram in an iterator.

Parameters:
-----------

out_score:  Output: Score for this M-gram (including any word penalty
and language weight).

out_bowt:  Output: Backoff weight for this M-gram.

read-only array of word IDs. ";

%feature("docstring")  ngram_iter_successors "

Iterate over all M-gram successors of an M-1-gram.

Parameters:
-----------

itor:  Iterator pointing to the M-1-gram to get successors of. ";

%feature("docstring")  ngram_iter_next "

Advance an M-gram iterator. ";

%feature("docstring")  ngram_iter_free "

Terminate an M-gram iterator. ";

%feature("docstring")  NGramModel::add_word "

Add a word (unigram) to the language model.

The semantics of this are not particularly well-defined for model
sets, and may be subject to change. Currently this will add the word
to all of the submodels

Parameters:
-----------

model:  The model to add a word to.

word:  Text of the word to add.

weight:  Weight of this word relative to the uniform distribution.

The word ID for the new word. ";

%feature("docstring")  NGramModel::read_classdef "

Read a class definition file and add classes to a language model.

This function assumes that the class tags have already been defined as
unigrams in the language model. All words in the class definition will
be added to the vocabulary as special in-class words. For this reason
is is necessary that they not have the same names as any words in the
general unigram distribution. The convention is to suffix them with
\":class_tag\", where class_tag is the class tag minus the enclosing
square brackets.

0 for success, <0 for error ";

%feature("docstring")  NGramModel::add_class "

Add a new class to a language model.

If classname already exists in the unigram set for model, then it will
be converted to a class tag, and classweight will be ignored.
Otherwise, a new unigram will be created as in ngram_model_add_word().
";

%feature("docstring")  NGramModel::add_class_word "

Add a word to a class in a language model.

Parameters:
-----------

model:  The model to add a word to.

classname:  Name of the class to add this word to.

word:  Text of the word to add.

weight:  Weight of this word relative to the within-class uniform
distribution.

The word ID for the new word. ";

%feature("docstring")  Config::ngram_model_set_init "

Create a set of language models sharing a common space of word IDs.

This function creates a meta-language model which groups together a
set of language models, synchronizing word IDs between them. To use
this language model, you can either select a submodel to use
exclusively using ngram_model_set_select(), or interpolate between
scores from all models. To do the latter, you can either pass a non-
NULL value of the weights parameter, or re-activate interpolation
later on by calling ngram_model_set_interp().

In order to make this efficient, there are some restrictions on the
models that can be grouped together. The most important (and currently
the only) one is that they must all share the same log-math
parameters.

Parameters:
-----------

config:  Any configuration parameters to be shared between models.

models:  Array of pointers to previously created language models.

names:  Array of strings to use as unique identifiers for LMs.

weights:  Array of weights to use in interpolating LMs, or NULL for no
interpolation.

n_models:  Number of elements in the arrays passed to this function.
";

%feature("docstring")  Config::ngram_model_set_read "

Read a set of language models from a control file.

This file creates a language model set from a \"control file\" of the
type used in Sphinx-II and Sphinx-III. File format (optional stuff is
indicated by enclosing in []):

[{ LMClassFileName LMClassFilename ... }]   TrigramLMFileName LMName
[{ LMClassName LMClassName ... }]   TrigramLMFileName LMName [{
LMClassName LMClassName ... }]   ... (There should be whitespace
around the { and } delimiters.)

This is an extension of the older format that had only
TrigramLMFilenName and LMName pairs. The new format allows a set of
LMClass files to be read in and referred to by the trigram LMs.

No \"comments\" allowed in this file.

Parameters:
-----------

config:  Configuration parameters.

lmctlfile:  Path to the language model control file.

lmath:  Log-math parameters to use for probability calculations.
Ownership of this object is assumed by the newly created
ngram_model_t, and you should not attempt to free it manually. If you
wish to reuse it elsewhere, you must retain it with logmath_retain().

newly created language model set. ";

%feature("docstring")  NGramModel::set_count "

Returns the number of language models in a set. ";

%feature("docstring")  NGramModel::set_iter "

Begin iterating over language models in a set.

iterator pointing to the first language model, or NULL if no models
remain. ";

%feature("docstring")  ngram_model_set_iter_next "

Move to the next language model in a set.

iterator pointing to the next language model, or NULL if no models
remain. ";

%feature("docstring")  ngram_model_set_iter_free "

Finish iteration over a langauge model set. ";

%feature("docstring")  ngram_model_set_iter_model "

Get language model and associated name from an iterator.

Parameters:
-----------

itor:  the iterator

lmname:  Output: string name associated with this language model.

Language model pointed to by this iterator. ";

%feature("docstring")  NGramModel::set_select "

Select a single language model from a set for scoring.

the newly selected language model, or NULL if no language model by
that name exists. ";

%feature("docstring")  NGramModel::set_lookup "

Look up a language model by name from a set.

language model corresponding to name, or NULL if no language model by
that name exists. ";

%feature("docstring")  NGramModel::set_current "

Get the current language model name, if any. ";

%feature("docstring")  NGramModel::set_interp "

Set interpolation weights for a set and enables interpolation.

If weights is NULL, any previously initialized set of weights will be
used. If no weights were specified to ngram_model_set_init(), then a
uniform distribution will be used. ";

%feature("docstring")  NGramModel::set_add "

Add a language model to a set.

Parameters:
-----------

set:  The language model set to add to.

model:  The language model to add.

name:  The name to associate with this model.

weight:  Interpolation weight for this model, relative to the uniform
distribution. 1.0 is a safe value.

reuse_widmap:  Reuse the existing word-ID mapping in set. Any new
words present in model will not be added to the word-ID mapping in
this case. ";

%feature("docstring")  NGramModel::set_remove "

Remove a language model from a set.

Parameters:
-----------

set:  The language model set to remove from.

name:  The name associated with the model to remove.

reuse_widmap:  Reuse the existing word-ID mapping in set. ";

%feature("docstring")  NGramModel::set_map_words "

Set the word-to-ID mapping for this model set. ";

%feature("docstring")  NGramModel::set_current_wid "

Query the word-ID mapping for the current language model.

the local word ID in the current language model, or NGRAM_INVALID_WID
if set_wid is invalid or interpolation is enabled. ";

%feature("docstring")  NGramModel::set_known_wid "

Test whether a word ID corresponds to a known word in the current
state of the language model set.

If there is a current language model, returns non-zero if set_wid
corresponds to a known word in that language model. Otherwise, returns
non-zero if set_wid corresponds to a known word in any language model.
";

%feature("docstring")  NGramModel::flush "

Flush any cached N-Gram information.

Some types of models cache trigram or other N-Gram information to
speed repeated access to N-Grams with shared histories. This function
flushes the cache so as to avoid dynamic memory leaks. ";


// File: feat_8c.xml
%feature("docstring")  parse_subvecs "

Parse subvector specification string.

Format of specification: '/' separated list of subvectors

each subvector is a ',' separated list of subranges

each subrange is a single<number> or<number>-<number> (inclusive),
where<number> is a feature vector dimension specifier.  E.g.,
\"24,0-11/25,12-23/26,27-38\" has: 3 subvectors

the 1st subvector has feature dims: 24, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
10, and 11.

etc.

Parameters:
-----------

str:  subvector specification string.

allocated 2-D array of subvector specs (free with subvecs_free()). If
there are N subvectors specified, subvec[N] = NULL; and each
subvec[0]..subvec[N-1] is -1 terminated vector of feature dims. ";

%feature("docstring")  subvecs_free "

Free array of subvector specs. ";

%feature("docstring")  Feature::set_subvecs "

Add a subvector specification to the feature module.

The subvector splitting will be performed after dynamic feature
computation, CMN, AGC, and any LDA transformation. The number of
streams in the dynamic feature type must be one, as with LDA.

After adding a subvector specification, the output of feature
computation will be split into multiple subvectors, and
feat_array_alloc() will allocate pointers accordingly. The number of
streams will remain the

Parameters:
-----------

fcb:  the feature descriptor.

subvecs:  subvector specification. This pointer is retained by the
feat_t and should not be freed manually.

0 for success or -1 if the subvector specification was invalid. ";

%feature("docstring")  Feature::subvec_project "

Project feature components to subvectors (if any). ";

%feature("docstring")  Feature::array_alloc "

Allocate an array to hold several frames worth of feature vectors.

The returned value is the mfcc_t ***data array, organized as follows:

data[0][0] = frame 0 stream 0 vector, data[0][1] = frame 0 stream 1
vector, ...

data[1][0] = frame 1 stream 0 vector, data[0][1] = frame 1 stream 1
vector, ...

data[2][0] = frame 2 stream 0 vector, data[0][1] = frame 2 stream 1
vector, ...

...

NOTE: For I/O convenience, the entire data area is allocated as one
contiguous block. pointer to the allocated space if successful, NULL
if any error. ";

%feature("docstring")  Feature::array_realloc "

Realloate the array of features.

Requires us to know the old size ";

%feature("docstring")  feat_array_free "

Free a buffer allocated with feat_array_alloc() ";

%feature("docstring")  Feature::s2_4x_cep2feat "";

%feature("docstring")  Feature::s3_1x39_cep2feat "";

%feature("docstring")  Feature::s3_cep "";

%feature("docstring")  Feature::s3_cep_dcep "";

%feature("docstring")  feat_1s_c_d_dd_cep2feat "";

%feature("docstring")  feat_1s_c_d_ld_dd_cep2feat "";

%feature("docstring")  Feature::copy "";

%feature("docstring")  feat_init "

Initialize feature module to use the selected type of feature stream.

One-time only initialization at the beginning of the program. Input
type is a string defining the kind of input->feature conversion
desired:

\"s2_4x\": s2mfc->Sphinx-II 4-feature stream,

\"1s_c_d_dd\": s2mfc->Sphinx 3.x single feature stream,

\"s3_1x39\": s2mfc->Sphinx 3.0 single feature stream,

\"n1,n2,n3,...\": Explicit feature vector layout spec. with comma-
separated feature stream lengths. In this case, the input data is
already in the feature format and there is no conversion necessary.

( feat_t *) descriptor if successful, NULL if error. Caller must not
directly modify the contents of the returned value. ";

%feature("docstring")  Feature::print "

Print the given block of feature vectors to the given FILE. ";

%feature("docstring")  Feature::cmn "";

%feature("docstring")  Feature::agc "";

%feature("docstring")  Feature::compute_utt "";

%feature("docstring")  Feature::s2mfc_read_norm_pad "

Read Sphinx-II format mfc file (s2mfc = Sphinx-II format MFC data).

If out_mfc is NULL, no actual reading will be done, and the number of
frames (plus padding) that would be read is returned.

It's important that normalization is done before padding because
frames outside the data we are interested in shouldn't be taken into
normalization stats.

# frames read (plus padding) if successful, -1 if error (e.g., mfc
array too small). ";

%feature("docstring")  Feature::s2mfc2feat "

Read a specified MFC file (or given segment within it), perform
CMN/AGC as indicated by fcb, and compute feature vectors.

Feature vectors are computed for the entire segment specified, by
including additional surrounding or padding frames to accommodate the
feature windows.

Number of frames of feature vectors computed if successful; -1 if any
error. If feat is NULL, then no actual computation will be done, and
the number of frames which must be allocated will be returned.  A note
on how the file path is constructed: If the control file already
specifies extension or absolute path, then these are not applied. The
default extension is defined by the application. ";

%feature("docstring")  Feature::s2mfc2block_utt "";

%feature("docstring")  Feature::s2mfc2live "

Feature computation routine for live mode decoder.

This function computes features for blocks of incoming data. It
retains an internal buffer for computing deltas, which means that the
number of output frames will not necessarily equal the number of input
frames.

It is very important to realize that the number of output frames can
be greater than the number of input frames, specifically when endutt
is true. It is guaranteed to never exceed *inout_ncep +
feat_window_size(fcb). You MUST have allocated at least that many
frames in ofeat, or you will experience a buffer overflow.

If beginutt and endutt are both true, CMN_CURRENT and AGC_MAX will be
done. Otherwise only CMN_PRIOR and AGC_EMAX will be done.

If beginutt is false, endutt is true, and the number of input frames
exceeds the input size, then end-of-utterance processing won't
actually be done. This condition can easily be checked, because
*inout_ncep will equal the return value on exit, and will also be
smaller than the value of *inout_ncep on entry.

The number of output frames actually computed. ";

%feature("docstring")  Feature::retain "

Retain ownership of feat_t.

pointer to retained feat_t. ";

%feature("docstring")  Feature::free "

Release resource associated with feat_t.

new reference count (0 if freed) ";

%feature("docstring")  Feature::report "

Report the feat_t data structure. ";


// File: fsg__model_8c.xml
%feature("docstring")  nextline_str2words "";

%feature("docstring")  FsgModel::trans_add "

Add the given transition to the FSG transition matrix.

Duplicates (i.e., two transitions between the same states, with the
same word label) are flagged and only the highest prob retained. ";

%feature("docstring")  FsgModel::tag_trans_add "

Add a \"tag\" transition between the given states.

A \"tag\" transition is a null transition with a non-null word ID,
which corresponds to a semantic tag or other symbol to be output when
this transition is taken.

As above, there can be at most one null or tag transition between the
given states; duplicates are flagged and only the best prob retained.
Transition probs must be <= 1 (i.e., logprob <= 0).

1 if a new transition was added, 0 if the prob of an existing
transition was upgraded; -1 if nothing was changed. ";

%feature("docstring")  FsgModel::null_trans_add "

Add a null transition between the given states.

There can be at most one null transition between the given states;
duplicates are flagged and only the best prob retained. Transition
probs must be <= 1 (i.e., logprob <= 0).

1 if a new transition was added, 0 if the prob of an existing
transition was upgraded; -1 if nothing was changed. ";

%feature("docstring")  FsgModel::null_trans_closure "

Obtain transitive closure of null transitions in the given FSG.

Parameters:
-----------

nulls:  List of null transitions, or NULL to find them automatically.

Updated list of null transitions. ";

%feature("docstring")  FsgModel::trans "

Get the list of transitions (if any) from state i to j. ";

%feature("docstring")  FsgModel::null_trans "

Get the null transition (if any) from state i to j. ";

%feature("docstring")  FsgModel::arcs "

Get an iterator over the outgoing transitions from state i. ";

%feature("docstring")  fsg_arciter_get "

Get the current arc from the arc iterator. ";

%feature("docstring")  fsg_arciter_next "

Move the arc iterator forward. ";

%feature("docstring")  fsg_arciter_free "

Free the arc iterator (early termination) ";

%feature("docstring")  FsgModel::word_id "

Look up a word in the FSG vocabulary.

Word ID for this word ";

%feature("docstring")  FsgModel::word_add "

Add a word to the FSG vocabulary.

Word ID for this new word. ";

%feature("docstring")  FsgModel::add_silence "

Add silence word transitions to each state in given FSG.

Parameters:
-----------

state:  state to add a self-loop to, or -1 for all states.

silprob:  probability of silence transition. ";

%feature("docstring")  FsgModel::add_alt "

Add alternate pronunciation transitions for a word in given FSG. ";

%feature("docstring")  fsg_model_init "

Create a new FSG. ";

%feature("docstring")  fsg_model_read "

Like fsg_model_readfile(), but from an already open stream. ";

%feature("docstring")  fsg_model_readfile "

Read a word FSG from the given file and return a pointer to the
structure created.

Return NULL if any error occurred.

File format:

Any number of comment lines; ignored   FSG_BEGIN [<fsgname>]   N
<#states>   S <start-state ID>   F <final-state ID>   T <from-state>
<to-state> <prob> [<word-string>]   T ...   ... (any number of state
transitions)   FSG_END   Any number of comment lines; ignored

The FSG spec begins with the line containing the keyword FSG_BEGIN. It
has an optional fsg name string. If not present, the FSG has the empty
string as its name.

Following the FSG_BEGIN declaration is the number of states, the start
state, and the final state, each on a separate line. States are
numbered in the range [0 .. <numberofstate>-1].

These are followed by all the state transitions, each on a separate
line, and terminated by the FSG_END line. A state transition has the
given probability of being taken, and emits the given word. The word
emission is optional; if word-string omitted, it is an epsilon or null
transition.

Comments can also be embedded within the FSG body proper (i.e. between
FSG_BEGIN and FSG_END): any line with a # character in col 1 is
treated as a comment line.

Return value: a new fsg_model_t structure if the file is successfully
read, NULL otherwise. ";

%feature("docstring")  FsgModel::retain "

Retain ownership of an FSG.

Pointer to retained FSG. ";

%feature("docstring")  FsgModel::trans_list_free "";

%feature("docstring")  FsgModel::free "

Free the given word FSG.

new reference count (0 if freed completely) ";

%feature("docstring")  FsgModel::write "

Write FSG to a file. ";

%feature("docstring")  FsgModel::writefile "

Write FSG to a file. ";

%feature("docstring")  FsgModel::write_fsm_trans "";

%feature("docstring")  FsgModel::write_fsm "

Write FSG to a file in AT&T FSM format. ";

%feature("docstring")  FsgModel::writefile_fsm "

Write FSG to a file in AT&T FSM format. ";

%feature("docstring")  FsgModel::write_symtab "

Write FSG symbol table to a file (for AT&T FSM) ";

%feature("docstring")  FsgModel::writefile_symtab "

Write FSG symbol table to a file (for AT&T FSM) ";


// File: jsgf_8c.xml
%feature("docstring")  Jsgf::yyparse "";

%feature("docstring")  Jsgf::expand_rule "";

%feature("docstring")  jsgf_atom_new "";

%feature("docstring")  jsgf_atom_free "";

%feature("docstring")  Jsgf::grammar_new "

Create a new JSGF grammar.

Parameters:
-----------

parent:  optional parent grammar for this one (NULL, usually).

new JSGF grammar object, or NULL on failure. ";

%feature("docstring")  Jsgf::grammar_free "

Free a JSGF grammar. ";

%feature("docstring")  jsgf_rhs_free "";

%feature("docstring")  Jsgf::kleene_new "";

%feature("docstring")  Jsgf::optional_new "";

%feature("docstring")  Jsgf::add_link "";

%feature("docstring")  extract_grammar_name "";

%feature("docstring")  Jsgf::grammar_name "

Get the grammar name from the file. ";

%feature("docstring")  Jsgf::fullname "";

%feature("docstring")  jsgf_fullname_from_rule "";

%feature("docstring")  importname2rulename "";

%feature("docstring")  Jsgf::expand_rhs "

Expand a right-hand-side of a rule (i.e.

a single alternate).

the FSG state at the end of this rule, NO_NODE if there's an error,
and RECURSIVE_NODE if the right-hand-side ended in right-recursion
(i.e. a link to an earlier FSG state). ";

%feature("docstring")  Jsgf::rule_iter "

Get an iterator over all rules in a grammar. ";

%feature("docstring")  Jsgf::get_rule "

Get a rule by name from a grammar. ";

%feature("docstring")  jsgf_rule_name "

Get the rule name from a rule. ";

%feature("docstring")  jsgf_rule_public "

Test if a rule is public or not. ";

%feature("docstring")  Jsgf::build_fsg_internal "";

%feature("docstring")  Jsgf::build_fsg "

Build a Sphinx FSG object from a JSGF rule. ";

%feature("docstring")  Jsgf::build_fsg_raw "

Build a Sphinx FSG object from a JSGF rule.

This differs from jsgf_build_fsg() in that it does not do closure on
epsilon transitions or any other postprocessing. For the time being
this is necessary in order to write it to a file - the FSG code will
be fixed soon. ";

%feature("docstring")  jsgf_read_file "

Read JSGF from file and return FSG object from it.

This function looks for a first public rule in jsgf and constructs
JSGF from it. ";

%feature("docstring")  Jsgf::write_fsg "

Convert a JSGF rule to Sphinx FSG text form.

This does a direct conversion without doing transitive closure on null
transitions and so forth. ";

%feature("docstring")  Jsgf::define_rule "";

%feature("docstring")  jsgf_rule_retain "";

%feature("docstring")  jsgf_rule_free "";

%feature("docstring")  path_list_search "";

%feature("docstring")  Jsgf::import_rule "";

%feature("docstring")  Jsgf::parse_file "

Parse a JSGF grammar from a file.

Parameters:
-----------

filename:  the name of the file to parse.

parent:  optional parent grammar for this one (NULL, usually).

new JSGF grammar object, or NULL on failure. ";


// File: ngram__model_8c.xml
%feature("docstring")  ngram_file_name_to_type "

Guess the file type for an N-Gram model from the filename.

the guessed file type, or NGRAM_INVALID if none could be guessed. ";

%feature("docstring")  ngram_str_to_type "

Get the N-Gram file type from a string.

file type, or NGRAM_INVALID if no such file type exists. ";

%feature("docstring")  ngram_type_to_str "

Get the canonical name for an N-Gram file type.

read-only string with the name for this file type, or NULL if no such
type exists. ";

%feature("docstring")  Config::ngram_model_read "

Read an N-Gram model from a file on disk.

Parameters:
-----------

config:  Optional pointer to a set of command-line arguments.
Recognized arguments are:

-mmap (boolean) whether to use memory-mapped I/O

-lw (float32) language weight to apply to the model

-wip (float32) word insertion penalty to apply to the model

-uw (float32) unigram weight to apply to the model

Parameters:
-----------

file_name:  path to the file to read.

file_type:  type of the file, or NGRAM_AUTO to determine
automatically.

lmath:  Log-math parameters to use for probability calculations.
Ownership of this object is assumed by the newly created
ngram_model_t, and you should not attempt to free it manually. If you
wish to reuse it elsewhere, you must retain it with logmath_retain().

newly created ngram_model_t. ";

%feature("docstring")  NGramModel::write "

Write an N-Gram model to disk.

0 for success, <0 on error ";

%feature("docstring")  NGramModel::init "

Initialize the base ngram_model_t structure. ";

%feature("docstring")  NGramModel::retain "

Retain ownership of an N-Gram model.

Pointer to retained model. ";

%feature("docstring")  NGramModel::flush "

Flush any cached N-Gram information.

Some types of models cache trigram or other N-Gram information to
speed repeated access to N-Grams with shared histories. This function
flushes the cache so as to avoid dynamic memory leaks. ";

%feature("docstring")  NGramModel::free "

Release memory associated with an N-Gram model.

new reference count (0 if freed completely) ";

%feature("docstring")  NGramModel::casefold "

Case-fold word strings in an N-Gram model.

WARNING: This is not Unicode aware, so any non-ASCII characters will
not be converted. ";

%feature("docstring")  NGramModel::recode "

Re-encode word strings in an N-Gram model.

Character set names are the same as those passed to iconv(1). If your
system does not have iconv, this function may fail. Also, because all
file formats consist of 8-bit character streams, attempting to convert
to or from UTF-16 (or any other encoding which contains null bytes) is
a recipe for total desaster.

We have no interest in supporting UTF-16, so don't ask.

Note that this does not affect any pronunciation dictionary you might
currently be using in conjunction with this N-Gram model, so the
effect of calling this during decoding is undefined. That's a bug! ";

%feature("docstring")  NGramModel::apply_weights "

Apply a language weight, insertion penalty, and unigram weight to a
language model.

This will change the values output by ngram_score() and friends. This
is done for efficiency since in decoding, these are the only values we
actually need. Call ngram_prob() if you want the \"raw\" N-Gram
probability estimate.

To remove all weighting, call ngram_apply_weights(model, 1.0, 1.0,
1.0). ";

%feature("docstring")  NGramModel::get_weights "

Get the current weights from a language model.

Parameters:
-----------

model:  The model in question.

out_log_wip:  Output: (optional) logarithm of word insertion penalty.

out_log_uw:  Output: (optional) logarithm of unigram weight.

language weight. ";

%feature("docstring")  NGramModel::ngram_ng_score "

Quick general N-Gram score lookup. ";

%feature("docstring")  NGramModel::ngram_score "

Get the score (scaled, interpolated log-probability) for a general
N-Gram.

The argument list consists of the history words (as null-terminated
strings) of the N-Gram, in reverse order, followed by NULL. Therefore,
if you wanted to get the N-Gram score for \"a whole joy\", you would
call:

score = ngram_score(model, \"joy\", \"whole\", \"a\", NULL);

This is not the function to use in decoding, because it has some
overhead for looking up words. Use ngram_ng_score(), ngram_tg_score(),
or ngram_bg_score() instead. In the future there will probably be a
version that takes a general language model state object, to support
suffix-array LM and things like that.

If one of the words is not in the LM's vocabulary, the result will
depend on whether this is an open or closed vocabulary language model.
For an open-vocabulary model, unknown words are all mapped to the
unigram <UNK> which has a non-zero probability and also participates
in higher-order N-Grams. Therefore, you will get a score of some sort
in this case.

For a closed-vocabulary model, unknown words are impossible and thus
have zero probability. Therefore, if word is unknown, this function
will return a \"zero\" log-probability, i.e. a large negative number.
To obtain this number for comparison, call ngram_zero(). ";

%feature("docstring")  NGramModel::ngram_tg_score "

Quick trigram score lookup. ";

%feature("docstring")  NGramModel::ngram_bg_score "

Quick bigram score lookup. ";

%feature("docstring")  NGramModel::ngram_ng_prob "

Quick \"raw\" probability lookup for a general N-Gram.

See documentation for ngram_ng_score() and ngram_apply_weights() for
an explanation of this. ";

%feature("docstring")  NGramModel::ngram_probv "

Get the \"raw\" log-probability for a general N-Gram.

This returns the log-probability of an N-Gram, as defined in the
language model file, before any language weighting, interpolation, or
insertion penalty has been applied.

When backing off to a unigram from a bigram or trigram, the unigram
weight (interpolation with uniform) is not removed. ";

%feature("docstring")  NGramModel::ngram_prob "

Get the \"raw\" log-probability for a general N-Gram.

This returns the log-probability of an N-Gram, as defined in the
language model file, before any language weighting, interpolation, or
insertion penalty has been applied.

When backing off to a unigram from a bigram or trigram, the unigram
weight (interpolation with uniform) is not removed. ";

%feature("docstring")  NGramModel::ngram_score_to_prob "

Convert score to \"raw\" log-probability.

The unigram weight (interpolation with uniform) is not removed, since
there is no way to know which order of N-Gram generated score.

Parameters:
-----------

model:  The N-Gram model from which score was obtained.

score:  The N-Gram score to convert

The raw log-probability value. ";

%feature("docstring")  NGramModel::ngram_unknown_wid "

Get the unknown word ID for a language model.

Language models can be either \"open vocabulary\" or \"closed
vocabulary\". The difference is that the former assigns a fixed non-
zero unigram probability to unknown words, while the latter does not
allow unknown words (or, equivalently, it assigns them zero
probability). If this is a closed vocabulary model, this function will
return NGRAM_INVALID_WID.

The ID for the unknown word, or NGRAM_INVALID_WID if none exists. ";

%feature("docstring")  NGramModel::ngram_zero "

Get the \"zero\" log-probability value for a language model. ";

%feature("docstring")  NGramModel::get_size "

Get the order of the N-gram model (i.e.

the \"N\" in \"N-gram\") ";

%feature("docstring")  NGramModel::get_counts "

Get the counts of the various N-grams in the model. ";

%feature("docstring")  NGramModel::ngram_iter_init "

Initialize base M-Gram iterator structure. ";

%feature("docstring")  NGramModel::mgrams "

Iterate over all M-grams.

Parameters:
-----------

model:  Language model to query.

m:  Order of the M-Grams requested minus one (i.e. order of the
history)

An iterator over the requested M, or NULL if no N-grams of order M+1
exist. ";

%feature("docstring")  NGramModel::ngram_iter "

Get an iterator over M-grams pointing to the specified M-gram. ";

%feature("docstring")  NGramModel::ngram_ng_iter "

Get an iterator over M-grams pointing to the specified M-gram. ";

%feature("docstring")  ngram_iter_successors "

Iterate over all M-gram successors of an M-1-gram.

Parameters:
-----------

itor:  Iterator pointing to the M-1-gram to get successors of. ";

%feature("docstring")  ngram_iter_get "

Get information from the current M-gram in an iterator.

Parameters:
-----------

out_score:  Output: Score for this M-gram (including any word penalty
and language weight).

out_bowt:  Output: Backoff weight for this M-gram.

read-only array of word IDs. ";

%feature("docstring")  ngram_iter_next "

Advance an M-gram iterator. ";

%feature("docstring")  ngram_iter_free "

Terminate an M-gram iterator. ";

%feature("docstring")  NGramModel::ngram_wid "

Look up numerical word ID. ";

%feature("docstring")  NGramModel::ngram_word "

Look up word string for numerical word ID. ";

%feature("docstring")  NGramModel::ngram_add_word_internal "

Add a word to the word string and ID mapping. ";

%feature("docstring")  NGramModel::add_word "

Add a word (unigram) to the language model.

The semantics of this are not particularly well-defined for model
sets, and may be subject to change. Currently this will add the word
to all of the submodels

Parameters:
-----------

model:  The model to add a word to.

word:  Text of the word to add.

weight:  Weight of this word relative to the uniform distribution.

The word ID for the new word. ";

%feature("docstring")  NGramModel::ngram_class_new "

Allocate and initialize an N-Gram class. ";

%feature("docstring")  ngram_class_add_word "

< Next available bucket. ";

%feature("docstring")  ngram_class_free "

Deallocate an N-Gram class. ";

%feature("docstring")  NGramModel::add_class_word "

Add a word to a class in a language model.

Parameters:
-----------

model:  The model to add a word to.

classname:  Name of the class to add this word to.

word:  Text of the word to add.

weight:  Weight of this word relative to the within-class uniform
distribution.

The word ID for the new word. ";

%feature("docstring")  NGramModel::add_class "

Add a new class to a language model.

If classname already exists in the unigram set for model, then it will
be converted to a class tag, and classweight will be ignored.
Otherwise, a new unigram will be created as in ngram_model_add_word().
";

%feature("docstring")  ngram_class_prob "

Get the in-class log probability for a word in an N-Gram class.

This probability, or 1 if word not found. ";

%feature("docstring")  read_classdef_file "

Read a probdef file.

< Are we currently reading a list of class words? ";

%feature("docstring")  classdef_free "

Free a class definition. ";

%feature("docstring")  NGramModel::read_classdef "

Read a class definition file and add classes to a language model.

This function assumes that the class tags have already been defined as
unigrams in the language model. All words in the class definition will
be added to the vocabulary as special in-class words. For this reason
is is necessary that they not have the same names as any words in the
general unigram distribution. The convention is to suffix them with
\":class_tag\", where class_tag is the class tag minus the enclosing
square brackets.

0 for success, <0 for error ";


// File: ngram__model__arpa_8c.xml
%feature("docstring")  ReadNgramCounts "";

%feature("docstring")  ReadUnigrams "";

%feature("docstring")  ReadBigrams "";

%feature("docstring")  ReadTrigrams "";

%feature("docstring")  new_unigram_table "";

%feature("docstring")  Config::ngram_model_arpa_read "

Read an N-Gram model from an ARPABO text file. ";

%feature("docstring")  NGramModel::arpa_write "

Write an N-Gram model to an ARPABO text file. ";

%feature("docstring")  NGramModel::arpa_apply_weights "";

%feature("docstring")  NGramModel::arpa_free "";


// File: ngram__model__arpa_8h.xml


// File: ngram__model__dmp_8c.xml
%feature("docstring")  new_unigram_table "";

%feature("docstring")  Config::ngram_model_dmp_read "

Read an N-Gram model from a Sphinx .DMP binary file. ";

%feature("docstring")  NGramModel::dmp_build "

Construct a DMP format model from a generic base model.

Note: If base is already a DMP format model, this just calls
ngram_model_retain(), and any changes will also be made in the base
model. ";

%feature("docstring")  fwrite_int32 "";

%feature("docstring")  fwrite_ug "";

%feature("docstring")  fwrite_bg "";

%feature("docstring")  fwrite_tg "";

%feature("docstring")  ngram_model_dmp_write_header "";

%feature("docstring")  ngram_model_dmp_write_lm_filename "";

%feature("docstring")  ngram_model_dmp_write_version "";

%feature("docstring")  NGramModel::dmp_write_ngram_counts "";

%feature("docstring")  ngram_model_dmp_write_fmtdesc "";

%feature("docstring")  NGramModel::dmp_write_unigram "";

%feature("docstring")  NGramModel::dmp_write_bigram "";

%feature("docstring")  NGramModel::dmp_write_trigram "";

%feature("docstring")  NGramModel::dmp_write_bgprob "";

%feature("docstring")  NGramModel::dmp_write_tgbowt "";

%feature("docstring")  NGramModel::dmp_write_tgprob "";

%feature("docstring")  NGramModel::dmp_write_tg_segbase "";

%feature("docstring")  NGramModel::dmp_write_wordstr "";

%feature("docstring")  NGramModel::dmp_write "

Write an N-Gram model to a Sphinx .DMP binary file. ";

%feature("docstring")  NGramModel::dmp_apply_weights "";

%feature("docstring")  NGramModel::dmp_free "";


// File: ngram__model__dmp_8h.xml
%feature("docstring")  NGramModel::dmp_build "

Construct a DMP format model from a generic base model.

Note: If base is already a DMP format model, this just calls
ngram_model_retain(), and any changes will also be made in the base
model. ";


// File: ngram__model__dmp32_8c.xml
%feature("docstring")  Config::ngram_model_dmp32_read "

Read an N-Gram model from a Sphinx .DMP32 binary file. ";

%feature("docstring")  NGramModel::dmp32_write "";

%feature("docstring")  NGramModel::dmp32_apply_weights "";

%feature("docstring")  NGramModel::dmp32_score "";

%feature("docstring")  NGramModel::dmp32_raw_score "";

%feature("docstring")  NGramModel::dmp32_free "";


// File: ngram__model__internal_8h.xml
%feature("docstring")  NGramModel::init "

Initialize the base ngram_model_t structure. ";

%feature("docstring")  Config::ngram_model_arpa_read "

Read an N-Gram model from an ARPABO text file. ";

%feature("docstring")  Config::ngram_model_dmp_read "

Read an N-Gram model from a Sphinx .DMP binary file. ";

%feature("docstring")  Config::ngram_model_dmp32_read "

Read an N-Gram model from a Sphinx .DMP32 binary file. ";

%feature("docstring")  NGramModel::arpa_write "

Write an N-Gram model to an ARPABO text file. ";

%feature("docstring")  NGramModel::dmp_write "

Write an N-Gram model to a Sphinx .DMP binary file. ";

%feature("docstring")  read_classdef_file "

Read a probdef file.

< Are we currently reading a list of class words? ";

%feature("docstring")  classdef_free "

Free a class definition. ";

%feature("docstring")  NGramModel::ngram_class_new "

Allocate and initialize an N-Gram class. ";

%feature("docstring")  ngram_class_free "

Deallocate an N-Gram class. ";

%feature("docstring")  ngram_class_prob "

Get the in-class log probability for a word in an N-Gram class.

This probability, or 1 if word not found. ";

%feature("docstring")  NGramModel::ngram_iter_init "

Initialize base M-Gram iterator structure. ";


// File: ngram__model__set_8c.xml
%feature("docstring")  my_compare "";

%feature("docstring")  NGramModel::build_widmap "";

%feature("docstring")  Config::ngram_model_set_init "

Create a set of language models sharing a common space of word IDs.

This function creates a meta-language model which groups together a
set of language models, synchronizing word IDs between them. To use
this language model, you can either select a submodel to use
exclusively using ngram_model_set_select(), or interpolate between
scores from all models. To do the latter, you can either pass a non-
NULL value of the weights parameter, or re-activate interpolation
later on by calling ngram_model_set_interp().

In order to make this efficient, there are some restrictions on the
models that can be grouped together. The most important (and currently
the only) one is that they must all share the same log-math
parameters.

Parameters:
-----------

config:  Any configuration parameters to be shared between models.

models:  Array of pointers to previously created language models.

names:  Array of strings to use as unique identifiers for LMs.

weights:  Array of weights to use in interpolating LMs, or NULL for no
interpolation.

n_models:  Number of elements in the arrays passed to this function.
";

%feature("docstring")  Config::ngram_model_set_read "

Read a set of language models from a control file.

This file creates a language model set from a \"control file\" of the
type used in Sphinx-II and Sphinx-III. File format (optional stuff is
indicated by enclosing in []):

[{ LMClassFileName LMClassFilename ... }]   TrigramLMFileName LMName
[{ LMClassName LMClassName ... }]   TrigramLMFileName LMName [{
LMClassName LMClassName ... }]   ... (There should be whitespace
around the { and } delimiters.)

This is an extension of the older format that had only
TrigramLMFilenName and LMName pairs. The new format allows a set of
LMClass files to be read in and referred to by the trigram LMs.

No \"comments\" allowed in this file.

Parameters:
-----------

config:  Configuration parameters.

lmctlfile:  Path to the language model control file.

lmath:  Log-math parameters to use for probability calculations.
Ownership of this object is assumed by the newly created
ngram_model_t, and you should not attempt to free it manually. If you
wish to reuse it elsewhere, you must retain it with logmath_retain().

newly created language model set. ";

%feature("docstring")  NGramModel::set_count "

Returns the number of language models in a set. ";

%feature("docstring")  NGramModel::set_iter "

Begin iterating over language models in a set.

iterator pointing to the first language model, or NULL if no models
remain. ";

%feature("docstring")  ngram_model_set_iter_next "

Move to the next language model in a set.

iterator pointing to the next language model, or NULL if no models
remain. ";

%feature("docstring")  ngram_model_set_iter_free "

Finish iteration over a langauge model set. ";

%feature("docstring")  ngram_model_set_iter_model "

Get language model and associated name from an iterator.

Parameters:
-----------

itor:  the iterator

lmname:  Output: string name associated with this language model.

Language model pointed to by this iterator. ";

%feature("docstring")  NGramModel::set_lookup "

Look up a language model by name from a set.

language model corresponding to name, or NULL if no language model by
that name exists. ";

%feature("docstring")  NGramModel::set_select "

Select a single language model from a set for scoring.

the newly selected language model, or NULL if no language model by
that name exists. ";

%feature("docstring")  NGramModel::set_current "

Get the current language model name, if any. ";

%feature("docstring")  NGramModel::set_current_wid "

Query the word-ID mapping for the current language model.

the local word ID in the current language model, or NGRAM_INVALID_WID
if set_wid is invalid or interpolation is enabled. ";

%feature("docstring")  NGramModel::set_known_wid "

Test whether a word ID corresponds to a known word in the current
state of the language model set.

If there is a current language model, returns non-zero if set_wid
corresponds to a known word in that language model. Otherwise, returns
non-zero if set_wid corresponds to a known word in any language model.
";

%feature("docstring")  NGramModel::set_interp "

Set interpolation weights for a set and enables interpolation.

If weights is NULL, any previously initialized set of weights will be
used. If no weights were specified to ngram_model_set_init(), then a
uniform distribution will be used. ";

%feature("docstring")  NGramModel::set_add "

Add a language model to a set.

Parameters:
-----------

set:  The language model set to add to.

model:  The language model to add.

name:  The name to associate with this model.

weight:  Interpolation weight for this model, relative to the uniform
distribution. 1.0 is a safe value.

reuse_widmap:  Reuse the existing word-ID mapping in set. Any new
words present in model will not be added to the word-ID mapping in
this case. ";

%feature("docstring")  NGramModel::set_remove "

Remove a language model from a set.

Parameters:
-----------

set:  The language model set to remove from.

name:  The name associated with the model to remove.

reuse_widmap:  Reuse the existing word-ID mapping in set. ";

%feature("docstring")  NGramModel::set_map_words "

Set the word-to-ID mapping for this model set. ";

%feature("docstring")  NGramModel::set_apply_weights "";

%feature("docstring")  NGramModel::set_score "";

%feature("docstring")  NGramModel::set_raw_score "";

%feature("docstring")  NGramModel::set_add_ug "";

%feature("docstring")  NGramModel::set_free "";

%feature("docstring")  NGramModel::set_flush "";


// File: ngram__model__set_8h.xml


// File: cmd__ln_8c.xml
%feature("docstring")  Config::arg_dump_r "";

%feature("docstring")  Config::parse_options "";

%feature("docstring")  arg_strlen "";

%feature("docstring")  cmp_name "";

%feature("docstring")  arg_sort "";

%feature("docstring")  strnappend "";

%feature("docstring")  strappend "";

%feature("docstring")  arg_resolve_env "";

%feature("docstring")  parse_string_list "";

%feature("docstring")  cmd_ln_val_init "";

%feature("docstring")  cmd_ln_val_free "";

%feature("docstring")  cmd_ln_get "

Retrieve the global cmd_ln_t object used by non-re-entrant functions.

Deprecated This is deprecated in favor of the re-entrant API. global
cmd_ln_t object. ";

%feature("docstring")  cmd_ln_appl_enter "

Old application initialization routine for Sphinx3 code.

Deprecated This is deprecated in favor of the re-entrant API. ";

%feature("docstring")  cmd_ln_appl_exit "

Finalization routine corresponding to cmd_ln_appl_enter().

Deprecated This is deprecated in favor of the re-entrant API. ";

%feature("docstring")  Config::parse_r "

Parse a list of strings into argumetns.

Parse the given list of arguments (name-value pairs) according to the
given definitions. Argument values can be retrieved in future using
cmd_ln_access(). argv[0] is assumed to be the program name and
skipped. Any unknown argument name causes a fatal error. The routine
also prints the prevailing argument values (to stderr) after parsing.

It is currently assumed that the strings in argv are allocated
statically, or at least that they will be valid as long as the
cmd_ln_t returned from this function. Unpredictable behaviour will
result if they are freed or otherwise become invalidated.

A cmd_ln_t containing the results of command line parsing, or NULL on
failure. ";

%feature("docstring")  Config::init "

Create a cmd_ln_t from NULL-terminated list of arguments.

This function creates a cmd_ln_t from a NULL-terminated list of
argument strings. For example, to create the equivalent of passing
\"-hmm foodir -dsratio 2 -lm bar.lm\" on the command-line:

config = cmd_ln_init(NULL, defs, TRUE, \"-hmm\", \"foodir\",
\"-dsratio\", \"2\", \"-lm\", \"bar.lm\", NULL);

Note that for simplicity, all arguments are passed as strings,
regardless of the actual underlying type.

Parameters:
-----------

inout_cmdln:  Previous command-line to update, or NULL to create a new
one.

defn:  Array of argument name definitions, or NULL to allow any
arguments.

strict:  Whether to fail on duplicate or unknown arguments.

A cmd_ln_t* containing the results of command line parsing, or NULL on
failure. ";

%feature("docstring")  cmd_ln_parse "

Non-reentrant version of cmd_ln_parse().

Deprecated This is deprecated in favor of the re-entrant API function
cmd_ln_parse_r(). 0 if successful, <0 if error. ";

%feature("docstring")  Config::parse_file_r "

Parse an arguments file by deliminating on \" \\\\r\\\\t\\\\n\" and
putting each tokens into an argv[] for cmd_ln_parse().

A cmd_ln_t containing the results of command line parsing, or NULL on
failure. ";

%feature("docstring")  cmd_ln_parse_file "

Parse an arguments file by deliminating on \" \\\\r\\\\t\\\\n\" and
putting each tokens into an argv[] for cmd_ln_parse().

Deprecated This is deprecated in favor of the re-entrant API function
cmd_ln_parse_file_r().

0 if successful, <0 on error. ";

%feature("docstring")  Config::print_help_r "

Print a help message listing the valid argument names, and the
associated attributes as given in defn.

Parameters:
-----------

fp:  output stream

defn:  Array of argument name definitions. ";

%feature("docstring")  Config::exists_r "

Re-entrant version of cmd_ln_exists().

True if the command line argument exists (i.e. it was one of the
arguments defined in the call to cmd_ln_parse_r(). ";

%feature("docstring")  Config::access_r "

Access the generic type union for a command line argument. ";

%feature("docstring")  Config::str_r "

Retrieve a string from a command-line object.

The command-line object retains ownership of this string, so you
should not attempt to free it manually.

Parameters:
-----------

cmdln:  Command-line object.

name:  the command-line flag to retrieve.

the string value associated with name, or NULL if name does not exist.
You must use cmd_ln_exists_r() to distinguish between cases where a
value is legitimately NULL and where the corresponding flag is
unknown. ";

%feature("docstring")  Config::str_list_r "

Retrieve an array of strings from a command-line object.

The command-line object retains ownership of this array, so you should
not attempt to free it manually.

Parameters:
-----------

cmdln:  Command-line object.

name:  the command-line flag to retrieve.

the array of strings associated with name, or NULL if name does not
exist. You must use cmd_ln_exists_r() to distinguish between cases
where a value is legitimately NULL and where the corresponding flag is
unknown. ";

%feature("docstring")  Config::int_r "

Retrieve an integer from a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  the command-line flag to retrieve.

the integer value associated with name, or 0 if name does not exist.
You must use cmd_ln_exists_r() to distinguish between cases where a
value is legitimately zero and where the corresponding flag is
unknown. ";

%feature("docstring")  Config::float_r "

Retrieve a floating-point number from a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  the command-line flag to retrieve.

the float value associated with name, or 0.0 if name does not exist.
You must use cmd_ln_exists_r() to distinguish between cases where a
value is legitimately zero and where the corresponding flag is
unknown. ";

%feature("docstring")  Config::set_str_r "

Set a string in a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  The command-line flag to set.

str:  String value to set. The command-line object does not retain
ownership of this pointer. ";

%feature("docstring")  Config::set_int_r "

Set an integer in a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  The command-line flag to set.

iv:  Integer value to set. ";

%feature("docstring")  Config::set_float_r "

Set a floating-point number in a command-line object.

Parameters:
-----------

cmdln:  Command-line object.

name:  The command-line flag to set.

fv:  Integer value to set. ";

%feature("docstring")  Config::retain "

Retain ownership of a command-line argument set.

pointer to retained command-line argument set. ";

%feature("docstring")  Config::free_r "

Release a command-line argument set and all associated strings.

new reference count (0 if freed completely) ";

%feature("docstring")  cmd_ln_free "

Free the global command line, if any exists.

Deprecated Use the re-entrant API instead. ";


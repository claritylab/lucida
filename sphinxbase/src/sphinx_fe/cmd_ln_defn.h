/* ====================================================================
 * Copyright (c) 1998-2000 Carnegie Mellon University.  All rights 
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
 */
/*********************************************************************
 *
 * File: cmd_ln_defn.h
 * 
 * Description: 
 *      Command line argument definition
 *
 * Author: 
 *      
 *********************************************************************/

#ifndef CMD_LN_DEFN_H
#define CMD_LN_DEFN_H

#include <sphinxbase/cmd_ln.h>
#include <sphinxbase/fe.h>

const char helpstr[] =
  "Description: \n\
Extract acoustic features form from audio file.\n\
\n\
The main parameters that affect the final output, with typical values, are:\n\
\n\
samprate, typically 8000, 11025, or 16000\n\
lowerf, 130, 200, 130, for the respective sampling rates above\n\
upperf, 3700, 5200, 6800, for the respective sampling rates above\n\
nfilt, 31, 37, 40, for the respective sampling rates above\n\
nfft, 256 or 512\n\
format, raw or nist or mswav\n\
\"";

const char examplestr[] =
  "Example: \n\
This example creates a cepstral file named \"output.mfc\" from an input audio file named \"input.raw\", which is a raw audio file (no header information), which was originally sampled at 16kHz. \n\
\n\
sphinx_fe -i  input.raw \n\
        -o   output.mfc \n\
        -input_endian little \n\
        -samprate  16000 \n\
        -lowerf    130 \n\
        -upperf    6800 \n\
        -nfilt     40 \n\
        -nfft      512";

static arg_t defn[] = {
  { "-help",
    ARG_BOOLEAN,
    "no",
    "Shows the usage of the tool"},
  
  { "-example",
    ARG_BOOLEAN,
    "no",
    "Shows example of how to use the tool"},

  waveform_to_cepstral_command_line_macro(),

  { "-argfile",
    ARG_STRING,
    NULL,
    "Argument file (e.g. feat.params from an acoustic model) to read parameters from.  This will override anything set in other command line arguments." },
  
  { "-i",
    ARG_STRING,
    NULL,
    "Single audio input file" },
  
  { "-o",
    ARG_STRING,
    NULL,
    "Single cepstral output file" },
  
  { "-c",
    ARG_STRING,
    NULL,
    "Control file for batch processing" },
  
  { "-nskip",
    ARG_INT32,
    "0",
    "If a control file was specified, the number of utterances to skip at the head of the file" },
  
  { "-runlen",
    ARG_INT32,
    "-1",
    "If a control file was specified, the number of utterances to process, or -1 for all" },

  { "-part",
    ARG_INT32,
    "0",
    "Index of the part to run (supersedes -nskip and -runlen if non-zero)" },
  
  { "-npart",
    ARG_INT32,
    "0",
    "Number of parts to run in (supersedes -nskip and -runlen if non-zero)" },
  
  { "-di",
    ARG_STRING,
    NULL,
    "Input directory, input file names are relative to this, if defined" },
  
  { "-ei",
    ARG_STRING,
    NULL,
    "Input extension to be applied to all input files" },
  
  { "-do",
    ARG_STRING,
    NULL,
    "Output directory, output files are relative to this" },
  
  { "-eo",
    ARG_STRING,
    NULL,
    "Output extension to be applied to all output files" },
  
  { "-build_outdirs",
    ARG_BOOLEAN,
    "yes",
    "Create missing subdirectories in output directory" },

  { "-sph2pipe",
    ARG_BOOLEAN,
    "no",
    "Input is NIST sphere (possibly with Shorten), use sph2pipe to convert" },

  { "-nist",
    ARG_BOOLEAN,
    "no",
    "Defines input format as NIST sphere" },
  
  { "-raw",
    ARG_BOOLEAN,
    "no",
    "Defines input format as raw binary data" },
  
  { "-mswav",
    ARG_BOOLEAN,
    "no",
    "Defines input format as Microsoft Wav (RIFF)" },
  
#ifdef HAVE_SNDFILE_H
  { "-sndfile",
    ARG_BOOLEAN,
    "no",
    "Use libsndfile to read input data" },
#endif
  
  { "-nchans",
    ARG_INT32,
    "1",
    "Number of channels of data (interlaced samples assumed)" },
  
  { "-whichchan",
    ARG_INT32,
    "0",
    "Channel to process (numbered from 1), or 0 to mix all channels" },
  
  { "-ofmt",
    ARG_STRING,
    "sphinx",
    "Format of output files - one of sphinx, htk, text." },
  
  { "-mach_endian",
    ARG_STRING,
#ifdef WORDS_BIGENDIAN
    "big",
#else
    "little",
#endif
    "Endianness of machine, big or little" },
  
  { "-blocksize",
    ARG_INT32,
    "2048",
    "Number of samples to read at a time." },

  { "-spec2cep",
    ARG_BOOLEAN,
    "no",
    "Input is log spectral files, output is cepstral files" },

  { "-cep2spec",
    ARG_BOOLEAN,
    "no",
    "Input is cepstral files, output is log spectral files" },

  { NULL, 0, NULL, NULL }
};

    
#define CMD_LN_DEFN_H

#endif /* CMD_LN_DEFN_H */ 

/*
 * Log record.  Maintained by RCS.
 *
 * $Log: cmd_ln_defn.h,v $
 * Revision 1.7  2006/02/25 00:53:48  egouvea
 * Added the flag "-seed". If dither is being used and the seed is less
 * than zero, the random number generator is initialized with time(). If
 * it is at least zero, it's initialized with the provided seed. This way
 * we have the benefit of having dither, and the benefit of being
 * repeatable.
 *
 * This is consistent with what sphinx3 does. Well, almost. The random
 * number generator is still what the compiler provides.
 *
 * Also, moved fe_init_params to fe_interface.c, so one can initialize a
 * variable of type param_t with meaningful values.
 *
 * Revision 1.6  2006/02/17 00:31:34  egouvea
 * Removed switch -melwarp. Changed the default for window length to
 * 0.025625 from 0.256 (so that a window at 16kHz sampling rate has
 * exactly 410 samples). Cleaned up include's. Replaced some E_FATAL()
 * with E_WARN() and return.
 *
 * Revision 1.5  2006/02/16 00:18:26  egouvea
 * Implemented flexible warping function. The user can specify at run
 * time which of several shapes they want to use. Currently implemented
 * are an affine function (y = ax + b), an inverse linear (y = a/x) and a
 * piecewise linear (y = ax, up to a frequency F, and then it "breaks" so
 * Nyquist frequency matches in both scales.
 *
 * Added two switches, -warp_type and -warp_params. The first specifies
 * the type, which valid values:
 *
 * -inverse or inverse_linear
 * -linear or affine
 * -piecewise or piecewise_linear
 *
 * The inverse_linear is the same as implemented by EHT. The -mel_warp
 * switch was kept for compatibility (maybe remove it in the
 * future?). The code is compatible with EHT's changes: cepstra created
 * from code after his changes should be the same as now. Scripts that
 * worked with his changes should work now without changes. Tested a few
 * cases, same results.
 *
 * Revision 1.4  2006/02/14 20:56:54  eht
 * Implement an argument -melwarp that changes the standard mel-scale
 * equation from:
 *      M(f) = 2595 * log10( 1 + f/700 )
 * to:
 *      M(f,w) = 2595 * log10( 1 + f/(700*w))
 *
 * So, 1.0 means no warp,  w > 1.0 means linear compression w < 1.0 means
 * linear expansion.
 *
 * Implement argument -nskip and -runlen arguments so that a subset of the
 * utterances in the control file can be executed.  Allows a simple
 * distribution of wave2feat processing over N processors.
 *
 * Revision 1.3  2005/05/19 21:21:55  egouvea
 * Bug #1176394: example bug
 *
 * Revision 1.2  2004/11/23 04:14:06  egouvea
 * Fixed bug in cmd_ln.c in which a wrong boolean argument led into an
 * infinite loop, and fixed the help and example strings, getting rid of
 * spaces, so that the appearance is better.
 *
 * Revision 1.1  2004/09/09 17:59:30  egouvea
 * Adding missing files to wave2feat
 *
 *
 *
 */

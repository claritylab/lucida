
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pocketsphinx.h"

#include <sphinxbase/err.h>
#include <sphinxbase/pio.h>
#include <sphinxbase/err.h>
#include <sphinxbase/strfuncs.h>
#include <sphinxbase/filename.h>
#include <sphinxbase/byteorder.h>

#define PCM_BUF_LEN 2048

static const arg_t cont_args_def[] = {
    POCKETSPHINX_OPTIONS,
    {"-adcin",
     ARG_BOOLEAN,
     "yes",
     "Input file is raw audio data."},
    {"-argfile",
     ARG_STRING,
     NULL,
     "Argument file giving extra arguments."},
    {"-infile",
     ARG_STRING,
     NULL,
     "Audio file to transcribe."},
    CMDLN_EMPTY_OPTION
};

static mfcc_t **
read_mfc_file(FILE *infh, int *out_nfr, int ceplen)
{
    long flen;
    int32 nmfc, nfr;
    float32 *floats;
    mfcc_t **mfcs;
    int swap, i;

    fseek(infh, 0, SEEK_END);
    flen = ftell(infh);
    fseek(infh, 0, SEEK_SET);
    if (fread(&nmfc, 4, 1, infh) != 1) {
        E_ERROR_SYSTEM("Failed to read 4 bytes from MFCC file");
        return NULL;
    }
    swap = 0;
    if (nmfc != flen / 4 - 1) {
        SWAP_INT32(&nmfc);
        swap = 1;
        if (nmfc != flen / 4 - 1) {
            E_ERROR("File length mismatch: 0x%x != 0x%x, maybe it's not MFCC file\n",
                    nmfc, flen / 4 - 1);
            return NULL;
        }
    }
    nfr = nmfc / ceplen;
    mfcs = (mfcc_t **)ckd_calloc_2d(nfr, ceplen, sizeof(**mfcs));
    floats = (float32 *)mfcs[0];
    if (fread(floats, 4, nfr * ceplen, infh) != nfr * ceplen) {
        E_ERROR_SYSTEM("Failed to read %d items from mfcfile");
        ckd_free_2d(mfcs);
        return NULL;
    }
    if (swap) {
        for (i = 0; i < nfr * ceplen; ++i)
            SWAP_FLOAT32(&floats[i]);
    }
#ifdef FIXED_POINT
    for (i = 0; i < nfr * ceplen; ++i)
        mfcs[0][i] = FLOAT2MFCC(floats[i]);
#endif
    *out_nfr = nfr;
    return mfcs;
}

int
main(int argc, char *argv[])
{
    cmd_ln_t *config;
    ps_decoder_t *ps;

    int32 out_score;

    const char *input_file_path;
    const char *cfg;
    const char *utt_id;
    const char *hyp;
    FILE *input_file;
    int16 buf[PCM_BUF_LEN];
    int k;

    config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, TRUE);

    /* Handle argument file as -argfile. */
    if (config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
    }
    if (config == NULL)
        return 1;

    if (cmd_ln_str_r(config, "-kws") == NULL && cmd_ln_str_r(config, "-keyphrase") == NULL) {
        E_ERROR("Keyword is missing. Use -keyphrase <keyphrase> or -kws <kws_file> to specify the phrase to look for.");
        return 1;
    }

    input_file_path = cmd_ln_str_r(config, "-infile");
    if (input_file_path == NULL) {
        E_ERROR("Input file is missing. Use -infile <input_file> to specify the file to look in.\n");
        return 1;
    }

    ps_default_search_args(config);
    ps = ps_init(config);
    
    if (ps == NULL) {
        E_ERROR("Failed to create the decoder\n");
        return 1;
    }
    
    input_file = fopen(input_file_path, "rb");
    if (input_file == NULL) {
        E_FATAL_SYSTEM("Failed to open input file '%s'", input_file_path);
    }
    
    ps_start_utt(ps, NULL);
    if (cmd_ln_boolean_r(config, "-adcin")) {
        fread(buf, 1, 44, input_file);
        while ((k = fread(buf, sizeof(int16), PCM_BUF_LEN, input_file)) > 0) {
            ps_process_raw(ps, buf, k, FALSE, FALSE);
        }
    } else {
        mfcc_t **mfcs;
        int nfr;

        if (NULL == (mfcs = read_mfc_file(input_file, &nfr, cmd_ln_int32_r(config, "-ceplen")))) {
            E_ERROR("Failed to read MFCC from the file '%s'\n", input_file_path);
            fclose(input_file);
            return -1;
        }
        ps_process_cep(ps, mfcs, nfr, FALSE, TRUE);
        ckd_free_2d(mfcs);
    }
    ps_end_utt(ps);

    hyp = ps_get_hyp(ps, &out_score, &utt_id);
    printf("hypothesis: %s\n", hyp);
    fflush(stdout);

    fclose(input_file);
    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}

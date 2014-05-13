#include <string.h>
#include <stdio.h>
#include <sbthread.h>
#include <strfuncs.h>
#include <fe.h>
#include <ckd_alloc.h>
#include <err.h>

#include <pthread.h>

#include "test_macros.h"

static const arg_t fe_args[] = {
    waveform_to_cepstral_command_line_macro(),
    { NULL, 0, NULL, NULL }
};

static pthread_key_t logfp_index;

void
err_threaded_cb(void * user_data, err_lvl_t level, const char *fmt, ...)
{
    FILE* logfp = (FILE *)pthread_getspecific(logfp_index);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(logfp, fmt, ap);
    va_end(ap);
}

static int
process(sbthread_t *th)
{
    FILE *raw;
    int16 *buf;
    mfcc_t **cepbuf;
    size_t nsamps;
    fe_t *fe;
    long fsize;
    int32 nfr;
    
    char outfile[16];
    FILE *logfile;
    
    sprintf(outfile, "%03ld.log", (long) sbthread_arg(th));
    logfile = fopen(outfile, "w");
    pthread_setspecific(logfp_index, (void *)logfile);

    if ((fe = fe_init_auto_r(sbthread_config(th))) == NULL)
        return -1;
    if ((raw = fopen(TESTDATADIR "/chan3.raw", "rb")) == NULL)
        return -1;
    fseek(raw, 0, SEEK_END);
    fsize = ftell(raw);
    fseek(raw, 0, SEEK_SET);
    buf = ckd_malloc(fsize);
    fread(buf, 1, fsize, raw);
    nsamps = fsize / 2;

    fe_process_utt(fe, buf, nsamps, &cepbuf, &nfr);
    E_INFO("nfr = %d\n", nfr);
    fe_free_2d(cepbuf);
    ckd_free(buf);
    fclose(raw);
    fe_free(fe);
    
    fclose(logfile);

    return 0;
}

int
main(int argc, char *argv[])
{
    sbthread_t *threads[10];
    cmd_ln_t *config;
    int i;
    
    E_INFO("Processing chan3.raw in 10 threads\n");
    if ((config = cmd_ln_parse_r(NULL, fe_args, 0, NULL, FALSE)) == NULL)
        return -1;

    err_set_callback(err_threaded_cb, NULL);
    pthread_key_create(&logfp_index, NULL);
    pthread_setspecific(logfp_index, (void*)stderr);

    for (i = 0; i < 10; ++i) {
        config = cmd_ln_retain(config);
        threads[i] = sbthread_start(config, process, (void *)(long)i);
    }
    for (i = 0; i < 10; ++i) {
        int rv;
        rv = sbthread_wait(threads[i]);
        E_INFO("Thread %d exited with status %d\n", i, rv);
        sbthread_free(threads[i]);
    }
    /* Now check to make sure they all created logfiles with the
     * correct contents. */
    for (i = 0; i < 10; ++i) {
        char logfile[16], line[256];
        FILE *logfh;

        sprintf(logfile, "%03d.log", i);
        TEST_ASSERT(logfh = fopen(logfile, "r"));
        while (fgets(line, sizeof(line), logfh)) {
            string_trim(line, STRING_BOTH);
            printf("%s: |%s|\n", logfile, line);
            /* total number of frames in audio file is 1436, but there are only 1290 voiced */
            TEST_EQUAL(0, strcmp(line, "INFO: test_tls_log.c(61): nfr = 1290"));
        }
        fclose(logfh);
    }
    cmd_ln_free_r(config);
    return 0;
}

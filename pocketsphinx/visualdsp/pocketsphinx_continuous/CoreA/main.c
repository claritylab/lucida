#include <sys/exception.h>
#include <stdio.h>
#include <stdlib.h>
#include <services\services.h>				// system services
#include "../ezkitutilities.h"
#include "../system.h"
#include "uart.h"

#include <string.h>

#include "fbs.h"
#include "err.h"
#include "ad.h"
#include "cont_ad.h"

#define BAUD_RATE	(57600)		// baud rate at which the UART will run

static void utterance_loop();

static ad_rec_t *ad;

#include "../command_line.h"
    	
// User program
void main() 
{

   
    int a;
    u8 input;
    char *test;
    
 	for (a=0;a<16;a++)
		ezInitLED(a);
   
	devInit();
    UARTinit( BAUD_RATE );
    ezErrorCheck(FS_STDIO_init(5));
		
	printf("Pocket sphinx on BF561 without linux\r\nFree Heap Memory: %d\r\n",space_unused());
	
	printf("fbs_init()\r\n");

 
    fbs_init(sizeof(fake_argv)/sizeof(char*) -1, fake_argv);

   	printf("ad_open_sps()\r\n");

    if ((ad = ad_open_sps(cmd_ln_float32("-samprate"))) == NULL)
        E_FATAL("ad_open_sps failed\n");
        
	printf("utterance_loop()\r\n");
	utterance_loop();

	printf("fbs_end()\r\n");
    fbs_end();
	printf("ad_close()\r\n");
    ad_close(ad);
}

/*
 * Main utterance processing loop:
 *     for (;;) {
 * 	   wait for start of next utterance;
 * 	   decode utterance until silence of at least 1 sec observed;
 * 	   print utterance result;
 *     }
 */
static void
utterance_loop()
{
    static int16 adbuf[4096];
    int32 k, fr, ts, rem;
    char *hyp=0;
    cont_ad_t *cont=0;
    static char word[256];

    /* Initialize continuous listening module */
    if ((cont = cont_ad_init(ad, ad_read)) == NULL)
        E_FATAL("cont_ad_init failed\n");
    if (ad_start_rec(ad) < 0)
        E_FATAL("ad_start_rec failed\n");
    if (cont_ad_calib(cont) < 0)
        E_FATAL("cont_ad_calib failed\n");

    for (;;) {
        /* Indicate listening for next utterance */
        printf("READY....\n");
        fflush(stdout);
        fflush(stderr);

        /* Await data for next utterance */
        while ((k = cont_ad_read(cont, adbuf, 4096)) == 0)
            ezDelay(200);

        if (k < 0)
            E_FATAL("cont_ad_read failed\n");

        /*
         * Non-zero amount of data received; start recognition of new utterance.
         * NULL argument to uttproc_begin_utt => automatic generation of utterance-id.
         */
        if (uttproc_begin_utt(NULL) < 0)
            E_FATAL("uttproc_begin_utt() failed\n");
        uttproc_rawdata(adbuf, k, 0);
        printf("Listening...\n");
        fflush(stdout);

        /* Note timestamp for this first block of data */
        ts = cont->read_ts;

        /* Decode utterance until end (marked by a "long" silence, >1sec) */
        for (;;) {
            /* Read non-silence audio data, if any, from continuous listening module */
            if ((k = cont_ad_read(cont, adbuf, 4096)) < 0)
                E_FATAL("cont_ad_read failed\n");
            if (k == 0) {
                /*
                 * No speech data available; check current timestamp with most recent
                 * speech to see if more than 1 sec elapsed.  If so, end of utterance.
                 */
                if ((cont->read_ts - ts) > DEFAULT_SAMPLES_PER_SEC)
                    break;
            }
            else {
                /* New speech data received; note current timestamp */
                ts = cont->read_ts;
            }

            /*
             * Decode whatever data was read above.  NOTE: Non-blocking mode!!
             * rem = #frames remaining to be decoded upon return from the function.
             */
            rem = uttproc_rawdata(adbuf, k, 0);
            
            

            /* If no work to be done, sleep a bit */
            if ((rem == 0) && (k == 0))
                ezDelay(20);
        }

        /*
         * Utterance ended; flush any accumulated, unprocessed A/D data and stop
         * listening until current utterance completely decoded
         */
        ad_stop_rec(ad);
        while (ad_read(ad, adbuf, 4096) >= 0);
        cont_ad_reset(cont);

        printf("Stopped listening, please wait...\n");
        fflush(stdout);
#if 0
        /* Power histogram dump (FYI) */
        cont_ad_powhist_dump(stdout, cont);
#endif
        /* Finish decoding, obtain and print result */
        uttproc_end_utt();
        if (uttproc_result(&fr, &hyp, 1) < 0)
            E_FATAL("uttproc_result failed\n");
        printf("%d: %s\n", fr, hyp);
        fflush(stdout);

        /* Exit if the first word spoken was GOODBYE */
        sscanf(hyp, "%s", word);
        if (strcmp(word, "goodbye") == 0)
            break;

        /* Resume A/D recording for next utterance */
        if (ad_start_rec(ad) < 0)
            E_FATAL("ad_start_rec failed\n");
    }

    cont_ad_close(cont);
}


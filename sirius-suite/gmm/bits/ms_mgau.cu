/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
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
/*
 * ms_mgau.c -- Essentially a wrapper that wrap up gauden and
 * senone. It supports multi-stream. 
 *
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1997 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * HISTORY
 * $Log$
 * Revision 1.2  2006/02/22  16:56:01  arthchan2003
 * Merged from SPHINX3_5_2_RCI_IRII_BRANCH: Added ms_mgau.[ch] into the trunk. It is a wrapper of ms_gauden and ms_senone
 * 
 * Revision 1.1.2.4  2005/09/25 18:55:19  arthchan2003
 * Added a flag to turn on and off precomputation.
 *
 * Revision 1.1.2.3  2005/08/03 18:53:44  dhdfu
 * Add memory deallocation functions.  Also move all the initialization
 * of ms_mgau_model_t into ms_mgau_init (duh!), which entails removing it
 * from decode_anytopo and friends.
 *
 * Revision 1.1.2.2  2005/08/02 21:05:38  arthchan2003
 * 1, Added dist and mgau_active as intermediate variable for computation. 2, Added ms_cont_mgau_frame_eval, which is a multi stream version of GMM computation mainly s3.0 family of tools. 3, Fixed dox-doc.
 *
 * Revision 1.1.2.1  2005/07/20 19:37:09  arthchan2003
 * Added a multi-stream cont_mgau (ms_mgau) which is a wrapper of both gauden and senone.  Add ms_mgau_init and model_set_mllr.  This allow eliminating 600 lines of code in decode_anytopo/align/allphone.
 *
 *
 *
 */

/* Local headers. */
#include "ms_mgau.h"

//#include <omp.h>

#include <pthread.h>

#define NTHREADS      8

pthread_spinlock_t spinlock;

struct timeval t1,t2;
float cuda_elapsedTime;
float cpu_elapsedTime;
float par_elapsedTime;

int i, start, tids[NTHREADS];
pthread_t threads[NTHREADS];
pthread_attr_t attr;

/*struct thread_param {
    gauden_t * g;
    int tid;
    int32 n_top;    
    mfcc_t** obs;
    gauden_dist_t ** out_dist;
};
struct thread_param *tp;*/

static gauden_t *global_g;
static int32 global_topn; 
static mfcc_t **global_obs;
static gauden_dist_t **global_dist;
static senone_t *global_sen;
static int16 *global_senscr;

static int32 global_best;

static int CPU = 1;

float calculateMiliseconds(struct timeval t1, struct timeval t2) {
	float elapsedTime;
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;
	return elapsedTime;
}
          
static ps_mgaufuncs_t ms_mgau_funcs = {
    "ms",
    ms_cont_mgau_frame_eval, /* frame_eval */
    ms_mgau_mllr_transform,  /* transform */
    ms_mgau_free             /* free */
};

ps_mgau_t *
ms_mgau_init(acmod_t *acmod, logmath_t *lmath, bin_mdef_t *mdef)
{
    /* Codebooks */
    ms_mgau_model_t *msg;
    ps_mgau_t *mg;
    gauden_t *g;
    senone_t *s;
    cmd_ln_t *config;

    config = acmod->config;   
    
    msg = (ms_mgau_model_t *) ckd_calloc(1, sizeof(ms_mgau_model_t));
    msg->config = config;
    msg->g = NULL;
    msg->s = NULL;
    
    g = msg->g = gauden_init(cmd_ln_str_r(config, "-mean"),
                             cmd_ln_str_r(config, "-var"),
                             cmd_ln_float32_r(config, "-varfloor"),
                             lmath);

    /* Verify n_feat and veclen, against acmod. */
    if (g->n_feat != feat_dimension1(acmod->fcb)) {
        E_ERROR("Number of streams does not match: %d != %d\n",
                g->n_feat, feat_dimension1(acmod->fcb));
        goto error_out;
    }
    for (i = 0; i < g->n_feat; ++i) {
        if (g->featlen[i] != feat_dimension2(acmod->fcb, i)) {
            E_ERROR("Dimension of stream %d does not match: %d != %d\n", i,
                    g->featlen[i], feat_dimension2(acmod->fcb, i));
            goto error_out;
        }
    }

    s = msg->s = senone_init(msg->g,
                             cmd_ln_str_r(config, "-mixw"),
                             cmd_ln_str_r(config, "-senmgau"),
                             cmd_ln_float32_r(config, "-mixwfloor"),
                             lmath, mdef);

    s->aw = cmd_ln_int32_r(config, "-aw");

    /* Verify senone parameters against gauden parameters */
    if (s->n_feat != g->n_feat)
        E_FATAL("#Feature mismatch: gauden= %d, senone= %d\n", g->n_feat,
                s->n_feat);
    if (s->n_cw != g->n_density)
        E_FATAL("#Densities mismatch: gauden= %d, senone= %d\n",
                g->n_density, s->n_cw);
    if (s->n_gauden > g->n_mgau)
        E_FATAL("Senones need more codebooks (%d) than present (%d)\n",
                s->n_gauden, g->n_mgau);
    if (s->n_gauden < g->n_mgau)
        E_ERROR("Senones use fewer codebooks (%d) than present (%d)\n",
                s->n_gauden, g->n_mgau);

    msg->topn = cmd_ln_int32_r(config, "-topn");
    E_INFO("The value of topn: %d\n", msg->topn);
    if (msg->topn == 0 || msg->topn > msg->g->n_density) {
        E_WARN
            ("-topn argument (%d) invalid or > #density codewords (%d); set to latter\n",
             msg->topn, msg->g->n_density);
        msg->topn = msg->g->n_density;
    }

    msg->dist = (gauden_dist_t ***)
        ckd_calloc_3d(g->n_mgau, g->n_feat, msg->topn,
                      sizeof(gauden_dist_t));
    msg->mgau_active = (uint8 *) ckd_calloc(g->n_mgau, sizeof(int8));

    mg = (ps_mgau_t *)msg;
    mg->vt = &ms_mgau_funcs;
    
    pthread_attr_init(&attr);
    pthread_spin_init(&spinlock, 0);

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);    
        
    return mg;
error_out:
    ms_mgau_free(ps_mgau_base(msg));
    return NULL;    
}

void
ms_mgau_free(ps_mgau_t * mg)
{
    ms_mgau_model_t *msg = (ms_mgau_model_t *)mg;
    if (msg == NULL)
        return;

    if (msg->g)
	gauden_free(msg->g);
    if (msg->s)
        senone_free(msg->s);
    if (msg->dist)
        ckd_free_3d((void *) msg->dist);
    if (msg->mgau_active)
        ckd_free(msg->mgau_active);
    
    ckd_free(msg);
    
    pthread_spin_destroy(&spinlock);
}

int
ms_mgau_mllr_transform(ps_mgau_t *s,
		       ps_mllr_t *mllr)
{
    ms_mgau_model_t *msg = (ms_mgau_model_t *)s;
    return gauden_mllr_transform(msg->g, mllr, msg->config);
}

//gauden_dist(gauden_t * g, int mgau, int32 n_top, mfcc_t** obs, gauden_dist_t ** out_dist)
// gauden_dist(g, gid, topn, feat, msg->dist[gid]);

__global__ void cuda_score(int32 n_mgau, int32 n_density, int32 featlen, mfcc_t * obs, mfcc_t ** mean, mfcc_t ** var, mfcc_t * det, gauden_dist_t * out_dist)
{
    int gid = blockDim.x * blockIdx.x + threadIdx.x;  
    
    ms_mgau_model_t *msg = (ms_mgau_model_t *)mg;
        
    int32 temp = g->n_mgau;

            int32 temp = g->n_mgau;
            //#pragma parallel 
            //#pragma loop count min(1024)
            best = (int32) 0x7fffffff;
            //#pragma omp parallel for
           // #pragma parallel
            //#pragma acc kernels copyin(g,msg->dist) copy(senscr)
            //#pragma omp parallel for
            for (gid = 0; gid < temp; gid++) {
                                                             
                int mgau = gid;
                int32 i, j, d;
        
                //mfcc_t ** mean = g->mean[mgau][0];
                //mfcc_t ** var = g->var[mgau][0];
                //mfcc_t * det = g->det[mgau][0];
                //int32 n_density = g->n_density;               
                                     
                gauden_dist_t *worst;
                //gauden_dist_t * out_dist = msg->dist[gid][0];

                for (i = 0; i < topn; i++)
                    out_dist[i].dist = WORST_DIST;
                worst = &(out_dist[topn - 1]);

                for (d = 0; d < n_density; d++) {
                    mfcc_t *m;
                    mfcc_t *v;
                    mfcc_t dval;

                    m = mean[d];
                    v = var[d];
                    dval = det[d];

                    //for (i = 0; (i < featlen) && (dval >= worst->dist); i++) {
                    for (i = 0; (i < featlen); i++) {                        
                        mfcc_t diff;
                        diff = obs[i] - m[i];
                        /* The compiler really likes this to be a single
                         * expression, for whatever reason. */
                        dval -= diff * diff * v[i];
                    }

                    if ((i < featlen) || (dval < worst->dist))     /* Codeword d worse than worst */
                        continue;

                    /* Codeword d at least as good as worst so far; insert in the ordered list */
                    for (i = 0; (i < topn) && (dval < out_dist[i].dist); i++);
                    assert(i < topn);
                    for (j = topn - 1; j > i; --j)
                        out_dist[j] = out_dist[j - 1];
                    out_dist[i].dist = dval;
                    out_dist[i].id = d;
                }

                
                senscr[gid] = local_senone_eval(sen, gid, msg->dist[sen->mgau[gid]], topn);

           }       
}
            
void *
gauden_dist_thread(void *tid)
{
    // printf("%d\n", ((struct thread_param*)arg)->x);
    //    struct thread_param *tp = ((struct thread_param*)arg);
    
    	int k, start, *mytid, end;

        int iterations = global_g->n_mgau / NTHREADS;

	mytid = (int *) tid;
	start = (*mytid * iterations);
	end = start + iterations;
	//printf ("Thread %d doing iterations %d to %d\n",*mytid,start,end-1);       
                
        for (k=start; k < end ; k++) {           
	    //gauden_dist(global_g, k, global_topn, global_obs, global_dist[k]);
        }
}

void *
senone_eval_thread(void *tid)
{
       int32 local_best = (int32) 0x7fffffff;        
    
    // printf("%d\n", ((struct thread_param*)arg)->x);
    //    struct thread_param *tp = ((struct thread_param*)arg);
    
    	int k, start, *mytid, end;

        int iterations = global_sen->n_sen / NTHREADS;

	mytid = (int *) tid;
	start = (*mytid * iterations);
	end = start + iterations;
	//printf ("Thread %d doing iterations %d to %d\n",*mytid,start,end-1);      
        
	for (k = start; k < end; k++) {
	    //global_senscr[k] = senone_eval(global_sen, k, global_dist[global_sen->mgau[k]], global_topn);
	    if (local_best > global_senscr[k]) {
		local_best = global_senscr[k];
	    }
	}
        
        // update best local to global
        pthread_spin_lock(&spinlock);
        if (global_best > local_best) {
            global_best = local_best;
        }
        pthread_spin_unlock(&spinlock);
}

void *
dist_eval_thread(void *tid)
{
       int32 local_best = (int32) 0x7fffffff;        
    
    // printf("%d\n", ((struct thread_param*)arg)->x);
    //    struct thread_param *tp = ((struct thread_param*)arg);
    
    	int k, start, *mytid, end;

        int iterations = global_sen->n_sen / NTHREADS;

	mytid = (int *) tid;
	start = (*mytid * iterations);
	end = start + iterations;
	//printf ("Thread %d doing iterations %d to %d\n",*mytid,start,end-1);      
        
	for (k = start; k < end; k++) {
         //   gauden_dist(global_g, k, global_topn, global_obs, global_dist[k]);
	  //  global_senscr[k] = senone_eval(global_sen, k, global_dist[global_sen->mgau[k]], global_topn);
	    if (local_best > global_senscr[k]) {
		local_best = global_senscr[k];
	    }
	}
        
        // update best local to global
        pthread_spin_lock(&spinlock);
        if (global_best > local_best) {
            global_best = local_best;
        }
        pthread_spin_unlock(&spinlock);
}

/*
 * Compute senone score for one senone.
 * NOTE:  Remember that senone PDF tables contain SCALED, NEGATED logs3 values.
 * NOTE:  Remember also that PDF data may be transposed or not depending on s->n_gauden.
 */
__host__ __device__ int32
local_senone_eval(senone_t * s, int id, gauden_dist_t ** dist, int32 n_top)
{
    int32 scr;                  /* total senone score */
    int32 fden;                 /* Gaussian density */
    int32 fscr;                 /* senone score for one feature */
    int32 fwscr;                /* senone score for one feature, one codeword */
    int32 f, t;
    gauden_dist_t *fdist;

    assert((id >= 0) && (id < s->n_sen));
    assert((n_top > 0) && (n_top <= s->n_cw));

    scr = 0;

    for (f = 0; f < s->n_feat; f++) {
        int top;
        fdist = dist[f];

        /* Top codeword for feature f */
	top = fden = ((int32)fdist[0].dist + ((1<<SENSCR_SHIFT) - 1)) >> SENSCR_SHIFT;
        fscr = (s->n_gauden > 1)
	    ? (fden + -s->pdf[id][f][fdist[0].id])  /* untransposed */
	    : (fden + -s->pdf[f][fdist[0].id][id]); /* transposed */
        E_DEBUG(1, ("fden[%d][%d] l+= %d + %d = %d\n",
                    id, f, -(fscr - fden), -(fden-top), -(fscr-top)));
        /* Remaining of n_top codewords for feature f */
        for (t = 1; t < n_top; t++) {
	    fden = ((int32)fdist[t].dist + ((1<<SENSCR_SHIFT) - 1)) >> SENSCR_SHIFT;
            fwscr = (s->n_gauden > 1) ?
                (fden + -s->pdf[id][f][fdist[t].id]) :
                (fden + -s->pdf[f][fdist[t].id][id]);
            fscr = logmath_add(s->lmath, fscr, fwscr);
            E_DEBUG(1, ("fden[%d][%d] l+= %d + %d = %d\n",
                        id, f, -(fwscr - fden), -(fden-top), -(fscr-top)));
        }
	/* Senone scores are also scaled, negated logs3 values.  Hence
	 * we have to negate the stuff we calculated above. */
        scr -= fscr;
    }
    /* Downscale scores. */
    scr /= s->aw;

    /* Avoid overflowing int16 */
    if (scr > 32767)
      scr = 32767;
    if (scr < -32768)
      scr = -32768;
    return scr;
}

#define WORST_DIST	(int32)(0x80000000)

int32
ms_cont_mgau_frame_eval(ps_mgau_t * mg,
			int16 *senscr,
			uint8 *senone_active,
			int32 n_senone_active,
                        mfcc_t ** feat,
			int32 frame,
			int32 compallsen)
{
    ms_mgau_model_t *msg = (ms_mgau_model_t *)mg;
    int32 gid;
    int32 topn;
    int32 best;
    gauden_t *g;
    senone_t *sen;
    //int i;
    
    topn = ms_mgau_topn(msg);
    g = ms_mgau_gauden(msg);
    sen = ms_mgau_senone(msg);
    
    global_g = g;
    global_topn = topn;
    global_sen = sen;
   // global_dist = msg->dist;
    global_obs = feat;
    global_senscr = senscr;
    
    //printf("%d\n%d\n%d\n", g->n_mgau, msg->topn);
    
    // n_mgau = 5120
    //topn = 16             
                        
    if (compallsen) {
	int32 s;

        if (CPU) {

          //  gettimeofday(&t1, NULL);
            //#pragma omp parallel for 
            int32 temp = g->n_mgau;
            //#pragma parallel 
            //#pragma loop count min(1024)
            best = (int32) 0x7fffffff;
            //#pragma omp parallel for
           // #pragma parallel
            //#pragma acc kernels copyin(g,msg->dist) copy(senscr)
            //#pragma omp parallel for
            for (gid = 0; gid < temp; gid++) {
                                                  
                mfcc_t * obs = feat[0];                
                int32 featlen =  g->featlen[0];
                int mgau = gid;
                int32 i, j, d;
        
                mfcc_t ** mean = g->mean[mgau][0];
                mfcc_t ** var = g->var[mgau][0];
                mfcc_t * det = g->det[mgau][0];
                int32 n_density = g->n_density;               
                                     
                gauden_dist_t *worst;
                gauden_dist_t * out_dist = msg->dist[gid][0];

                for (i = 0; i < topn; i++)
                    out_dist[i].dist = WORST_DIST;
                worst = &(out_dist[topn - 1]);

                for (d = 0; d < n_density; d++) {
                    mfcc_t *m;
                    mfcc_t *v;
                    mfcc_t dval;

                    m = mean[d];
                    v = var[d];
                    dval = det[d];

                    //for (i = 0; (i < featlen) && (dval >= worst->dist); i++) {
                    for (i = 0; (i < featlen); i++) {                        
                        mfcc_t diff;
                        diff = obs[i] - m[i];
                        /* The compiler really likes this to be a single
                         * expression, for whatever reason. */
                        dval -= diff * diff * v[i];
                    }

                    if ((i < featlen) || (dval < worst->dist))     /* Codeword d worse than worst */
                        continue;

                    /* Codeword d at least as good as worst so far; insert in the ordered list */
                    for (i = 0; (i < topn) && (dval < out_dist[i].dist); i++);
                    assert(i < topn);
                    for (j = topn - 1; j > i; --j)
                        out_dist[j] = out_dist[j - 1];
                    out_dist[i].dist = dval;
                    out_dist[i].id = d;
                }

                
                senscr[gid] = local_senone_eval(sen, gid, msg->dist[sen->mgau[gid]], topn);

///                   local_senone_eval(senone_t * s, int id, gauden_dist_t ** dist, int32 n_top)


           }       
     
            //#pragma loop count min(1024)
            for (s = 0; s < sen->n_sen; s++) {
                //senscr[s] = senone_eval(sen, s, msg->dist[sen->mgau[s]], topn);
                if (best > senscr[s]) {
                    best = senscr[s];
                }
            }
           // gettimeofday(&t2, NULL);
          //  cpu_elapsedTime = calculateMiliseconds(t1, t2);
          //  printf("CPU SEQ Time=%4.3f ms\n",  cpu_elapsedTime);      

        } else {

            // GPU 
            global_best = (int32) 0x7fffffff;

            // PTHREAD        
          //  gettimeofday(&t1, NULL);
            /* for (i = 0; i < NTHREADS; i++) {
                tids[i] = i;
                pthread_create(&threads[i], &attr, dist_eval_thread, (void *) &tids[i]);
            }
            for (i = 0; i < NTHREADS; i++) {
                pthread_join(threads[i], NULL);
            }
            // compute gauden distance
           // for (i = 0; i < NTHREADS; i++) {
                tids[i] = i;
                pthread_create(&threads[i], &attr, gauden_dist_thread, (void *) &tids[i]);
            }        
            for (i = 0; i < NTHREADS; i++) {
                pthread_join(threads[i], NULL);
            }*/     
            // senone eval
            /*for (i = 0; i < NTHREADS; i++) {
                tids[i] = i;
                pthread_create(&threads[i], &attr, senone_eval_thread, (void *) &tids[i]);
            }        
            for (i = 0; i < NTHREADS; i++) {
                pthread_join(threads[i], NULL);
            }         */
            /*best = (int32) 0x7fffffff;
            for (s = 0; s < sen->n_sen; s++) {
                senscr[s] = senone_eval(sen, s, msg->dist[sen->mgau[s]], topn);
                if (best > senscr[s]) {
                    best = senscr[s];
                }
            }*/
          //  gettimeofday(&t2, NULL);

          //  par_elapsedTime = calculateMiliseconds(t1, t2);
            //printf("CPU PTHREAD Time=%4.3f ms\n",  par_elapsedTime);                       

           // if (global_best != best) {
             //   printf("ERROR computing best score!!  global_best: %d != best: %d", global_best, best);
          //  } else {
           //     best = global_best;
            //}

        }

        /// END OF PTHREAD
        
       // #pragma omp parallel for
	/* Normalize senone scores */
	for (s = 0; s < sen->n_sen; s++) {
	    int32 bs = senscr[s] - best;
	    if (bs > 32767)
		bs = 32767;
	    if (bs < -32768)
		bs = -32768;
	    senscr[s] = bs;
	}
    }
    else {
	int32 i, n;
	/* Flag all active mixture-gaussian codebooks */
	for (gid = 0; gid < g->n_mgau; gid++)
	    msg->mgau_active[gid] = 0;

	n = 0;
	for (i = 0; i < n_senone_active; i++) {
	    /* senone_active consists of deltas. */
	    int32 s = senone_active[i] + n;
	    msg->mgau_active[sen->mgau[s]] = 1;
	    n = s;
	}

	/* Compute topn gaussian density values (for active codebooks) */
	for (gid = 0; gid < g->n_mgau; gid++) {
	    if (msg->mgau_active[gid])
		gauden_dist(g, gid, topn, feat, msg->dist[gid]);
	}

	best = (int32) 0x7fffffff;
	n = 0;
	for (i = 0; i < n_senone_active; i++) {
	    int32 s = senone_active[i] + n;
	    senscr[s] = senone_eval(sen, s, msg->dist[sen->mgau[s]], topn);
	    if (best > senscr[s]) {
		best = senscr[s];
	    }
	    n = s;
	}

	/* Normalize senone scores */
	n = 0;
	for (i = 0; i < n_senone_active; i++) {
	    int32 s = senone_active[i] + n;
	    int32 bs = senscr[s] - best;
	    if (bs > 32767)
		bs = 32767;
	    if (bs < -32768)
		bs = -32768;
	    senscr[s] = bs;
	    n = s;
	}
    }

    return 0;
}

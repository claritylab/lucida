/*
 *      Mathematical operations for vectors.
 *
 * Copyright (c) 2007-2010, Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of the authors nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id$ */

#ifndef __VECMATH_H__
#define __VECMATH_H__

#include <math.h>
#include <memory.h>

#ifdef  USE_SSE
#include <emmintrin.h>
#endif/*USE_SSE*/

#ifdef _MSC_VER
#include <malloc.h>
#else
#include <stdlib.h>
static inline void *_aligned_malloc(size_t size, size_t alignment)
{
    void *p;
    int ret = posix_memalign(&p, alignment, size);
    return (ret == 0) ? p : 0;
}
static inline void _aligned_free(void *p)
{
    free(p);
}
#endif

#ifdef _MSC_VER
#define MIE_ALIGN(x) __declspec(align(x))
#else
#define MIE_ALIGN(x) __attribute__((aligned(x)))
#endif

#define CONST_128D(var, val) \
    MIE_ALIGN(16) static const double var[2] = {(val), (val)}


inline static void veczero(floatval_t *x, const int n)
{
    memset(x, 0, sizeof(floatval_t) * n);
}

inline static void vecset(floatval_t *x, const floatval_t a, const int n)
{
    int i;
    for (i = 0;i < n;++i) {
        x[i] = a;
    }
}

inline static void veccopy(floatval_t *y, const floatval_t *x, const int n)
{
    memcpy(y, x, sizeof(floatval_t) * n);
}

inline static void vecadd(floatval_t *y, const floatval_t *x, const int n)
{
    int i;
    for (i = 0;i < n;++i) {
        y[i] += x[i];
    }
}

inline static void vecaadd(floatval_t *y, const floatval_t a, const floatval_t *x, const int n)
{
    int i;
    for (i = 0;i < n;++i) {
        y[i] += a * x[i];
    }
}

inline static void vecsub(floatval_t *y, const floatval_t *x, const int n)
{
    int i;
    for (i = 0;i < n;++i) {
        y[i] -= x[i];
    }
}

inline static void vecasub(floatval_t *y, const floatval_t a, const floatval_t *x, const int n)
{
    int i;
    for (i = 0;i < n;++i) {
        y[i] -= a * x[i];
    }
}

inline static void vecmul(floatval_t *y, const floatval_t *x, const int n)
{
    int i;
    for (i = 0;i < n;++i) {
        y[i] *= x[i];
    }
}

inline static void vecinv(floatval_t *y, const int n)
{
    int i;
    for (i = 0;i < n;++i) {
        y[i] = 1. / y[i];
    }
}

inline static void vecscale(floatval_t *y, const floatval_t a, const int n)
{
    int i;
    for (i = 0;i < n;++i) {
        y[i] *= a;
    }
}

inline static floatval_t vecdot(const floatval_t *x, const floatval_t *y, const int n)
{
    int i;
    floatval_t s = 0;
    for (i = 0;i < n;++i) {
        s += x[i] * y[i];
    }
    return s;
}

inline static floatval_t vecsum(floatval_t* x, const int n)
{
    int i;
    floatval_t s = 0.;

    for (i = 0;i < n;++i) {
        s += x[i];
    }
    return s;
}

inline static floatval_t vecsumlog(floatval_t* x, const int n)
{
    int i;
    floatval_t s = 0.;
    for (i = 0;i < n;++i) {
        s += log(x[i]);
    }
    return s;
}

#ifdef  USE_SSE

inline static void vecexp(double *values, const int n)
{
    int i;
    CONST_128D(one, 1.);
    CONST_128D(log2e, 1.4426950408889634073599);
    CONST_128D(maxlog, 7.09782712893383996843e2);   // log(2**1024)
    CONST_128D(minlog, -7.08396418532264106224e2);  // log(2**-1022)
    CONST_128D(c1, 6.93145751953125E-1);
    CONST_128D(c2, 1.42860682030941723212E-6);
    CONST_128D(w11, 3.5524625185478232665958141148891055719216674475023e-8);
    CONST_128D(w10, 2.5535368519306500343384723775435166753084614063349e-7);
    CONST_128D(w9, 2.77750562801295315877005242757916081614772210463065e-6);
    CONST_128D(w8, 2.47868893393199945541176652007657202642495832996107e-5);
    CONST_128D(w7, 1.98419213985637881240770890090795533564573406893163e-4);
    CONST_128D(w6, 1.3888869684178659239014256260881685824525255547326e-3);
    CONST_128D(w5, 8.3333337052009872221152811550156335074160546333973e-3);
    CONST_128D(w4, 4.1666666621080810610346717440523105184720007971655e-2);
    CONST_128D(w3, 0.166666666669960803484477734308515404418108830469798);
    CONST_128D(w2, 0.499999999999877094481580370323249951329122224389189);
    CONST_128D(w1, 1.0000000000000017952745258419615282194236357388884);
    CONST_128D(w0, 0.99999999999999999566016490920259318691496540598896);
    const __m128i offset = _mm_setr_epi32(1023, 1023, 0, 0);

    for (i = 0;i < n;i += 4) {
        __m128i k1, k2;
        __m128d p1, p2;
        __m128d a1, a2;
        __m128d xmm0, xmm1;
        __m128d x1, x2;

        /* Load four double values. */
        xmm0 = _mm_load_pd(maxlog);
        xmm1 = _mm_load_pd(minlog);
        x1 = _mm_load_pd(values+i);
        x2 = _mm_load_pd(values+i+2);
        x1 = _mm_min_pd(x1, xmm0);
        x2 = _mm_min_pd(x2, xmm0);
        x1 = _mm_max_pd(x1, xmm1);
        x2 = _mm_max_pd(x2, xmm1);

        /* a = x / log2; */
        xmm0 = _mm_load_pd(log2e);
        xmm1 = _mm_setzero_pd();
        a1 = _mm_mul_pd(x1, xmm0);
        a2 = _mm_mul_pd(x2, xmm0);

        /* k = (int)floor(a); p = (float)k; */
        p1 = _mm_cmplt_pd(a1, xmm1);
        p2 = _mm_cmplt_pd(a2, xmm1);
        xmm0 = _mm_load_pd(one);
        p1 = _mm_and_pd(p1, xmm0);
        p2 = _mm_and_pd(p2, xmm0);
        a1 = _mm_sub_pd(a1, p1);
        a2 = _mm_sub_pd(a2, p2);
        k1 = _mm_cvttpd_epi32(a1);
        k2 = _mm_cvttpd_epi32(a2);
        p1 = _mm_cvtepi32_pd(k1);
        p2 = _mm_cvtepi32_pd(k2);

        /* x -= p * log2; */
        xmm0 = _mm_load_pd(c1);
        xmm1 = _mm_load_pd(c2);
        a1 = _mm_mul_pd(p1, xmm0);
        a2 = _mm_mul_pd(p2, xmm0);
        x1 = _mm_sub_pd(x1, a1);
        x2 = _mm_sub_pd(x2, a2);
        a1 = _mm_mul_pd(p1, xmm1);
        a2 = _mm_mul_pd(p2, xmm1);
        x1 = _mm_sub_pd(x1, a1);
        x2 = _mm_sub_pd(x2, a2);

        xmm0 = _mm_load_pd(w11);
        xmm1 = _mm_load_pd(w10);
        a1 = _mm_mul_pd(x1, xmm0);
        a2 = _mm_mul_pd(x2, xmm0);
        a1 = _mm_add_pd(a1, xmm1);
        a2 = _mm_add_pd(a2, xmm1);

        xmm0 = _mm_load_pd(w9);
        xmm1 = _mm_load_pd(w8);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm0);
        a2 = _mm_add_pd(a2, xmm0);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm1);
        a2 = _mm_add_pd(a2, xmm1);

        xmm0 = _mm_load_pd(w7);
        xmm1 = _mm_load_pd(w6);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm0);
        a2 = _mm_add_pd(a2, xmm0);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm1);
        a2 = _mm_add_pd(a2, xmm1);

        xmm0 = _mm_load_pd(w5);
        xmm1 = _mm_load_pd(w4);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm0);
        a2 = _mm_add_pd(a2, xmm0);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm1);
        a2 = _mm_add_pd(a2, xmm1);

        xmm0 = _mm_load_pd(w3);
        xmm1 = _mm_load_pd(w2);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm0);
        a2 = _mm_add_pd(a2, xmm0);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm1);
        a2 = _mm_add_pd(a2, xmm1);

        xmm0 = _mm_load_pd(w1);
        xmm1 = _mm_load_pd(w0);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm0);
        a2 = _mm_add_pd(a2, xmm0);
        a1 = _mm_mul_pd(a1, x1);
        a2 = _mm_mul_pd(a2, x2);
        a1 = _mm_add_pd(a1, xmm1);
        a2 = _mm_add_pd(a2, xmm1);

        /* p = 2^k; */
        k1 = _mm_add_epi32(k1, offset);
        k2 = _mm_add_epi32(k2, offset);
        k1 = _mm_slli_epi32(k1, 20);
        k2 = _mm_slli_epi32(k2, 20);
        k1 = _mm_shuffle_epi32(k1, 0x72);
        k2 = _mm_shuffle_epi32(k2, 0x72);
        p1 = _mm_castsi128_pd(k1);
        p2 = _mm_castsi128_pd(k2);

        /* a *= 2^k. */
        a1 = _mm_mul_pd(a1, p1);
        a2 = _mm_mul_pd(a2, p2);

        /* Store the results. */
        _mm_store_pd(values+i, a1);
        _mm_store_pd(values+i+2, a2);
    }
}

#else

inline static void vecexp(double *values, const int n)
{
    int i;
    for (i = 0;i < n;++i) {
        values[i] = exp(values[i]);
    }
}

#endif /*USE_SSE*/

#endif/*__VECMATH_H__*/

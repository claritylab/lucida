// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <iostream>
#include "SSE2CodeGenerator.hh"

using namespace Mm;

bool SSE2L2NormCodeGenerator::appendProlog()
{
    /*	x86:
	push   esi		 ; save registers
	push   ebx
	mov    esi, [esp+12]     ; esi = &mean[0];
	mov    ebx, [esp+16]     ; ebx = &featureVector[0]
    */
    /*	x86-64:
	; parameters are already stored in registers
	; rdi = &mean[0]
	; rsi = &featureVector[0]
    */
    static const u8 code[] = {
#if !defined(PROC_x86_64)
	0x56,
	0x53,
	0x8b, 0x74, 0x24, 0x0c,
	0x8b, 0x5c, 0x24, 0x10
#endif
    };
    return append(code, sizeof(code));
}

bool SSE2L2NormCodeGenerator::appendClearAccumulator()
{
    /*
	xor    eax,  eax         ; eax  = 0
	pxor   xmm7, xmm7        ; xmm7 = 0
    */
    static const u8 code[] = { 0x66, 0x0f, 0xef, 0xff };
    return append(code, sizeof(code));
}

bool SSE2L2NormCodeGenerator::appendFetchA(int block, u32 offset)
{
    /*  x86:
	block 0:
	movdqu xmm0, [esi+offset] ; copy 128 bits from mean vector in xmm0

	block 1:
	movdqu xmm4, [esi+offset] ; copy 128 bits from mean vector in xmm0
    */
    /*  x86-64:
	block 0:
	movdqu xmm0, [rdi+offset]

	block 1:
	movdqu xmm4, [rdi+offset]
    */
    static const u8 code[3] = { 0xf3, 0x0f, 0x6f };
    bool ok = append(code, sizeof(code));

    if (offset == 0) {
	// without offset
#if defined(PROC_x86_64)
	static const u8 modrm[2][1] = { { 0x07 }, { 0x27 } };
#else
	static const u8 modrm[2][1] = { { 0x06 }, { 0x26 } };
#endif
	ok = append(modrm[block], 1) && ok;
    } else {
	// with offset
	if (offset >= 128) {
	    // 32-bit displacement
#if defined(PROC_x86_64)
	    static const u8 modrm[2][1] = { { 0x87 }, { 0xa7 } };
#else
	    static const u8 modrm[2][1] = { { 0x86 }, { 0xa6 } };
#endif
	    ok = append(modrm[block], 1) && appendDWordOffset(offset) && ok;
	} else {
	    // 8bit displacement
#if defined(PROC_x86_64)
	    static const u8 modrm[2][1] = { { 0x47 }, { 0x67 } };
#else
	    static const u8 modrm[2][1] = { { 0x46 }, { 0x66 } };
#endif
	    u8 byteOffset = offset;
	    ok = append(modrm[block], 1) && append(&byteOffset, 1) && ok;
	}
    }
    return ok;
}

bool SSE2L2NormCodeGenerator::appendFetchB(int block, u32 offset)
{
    /*  x86:
	block 0:
	movdqu xmm1, [ebx+offset] ; copy 128 bits from feature vector in xmm0

	block 1:
	movdqu xmm5, [ebx+offset] ; copy 128 bits from feature vector in xmm0
    */
    /*  x86-64:
	block 0:
	movdqu xmm1, [rsi+offset]

	block 1:
	movdqu xmm5, [rsi+offset]
    */

    static const u8 code[3] = { 0xf3, 0x0f, 0x6f };
    bool ok = append(code, sizeof(code));

    if (offset == 0) {
	// without offset
#if defined(PROC_x86_64)
	static const u8 modrm[2][1] = { { 0x0e }, { 0x2e } };
#else
	static const u8 modrm[2][1] = { { 0x0b }, { 0x2b } };
#endif
	ok = append(modrm[block], 1) && ok;
    } else {
	// with offset
	if (offset >= 128) {
	    // 32-bit displacement
#if defined(PROC_x86_64)
	    static const u8 modrm[2][1] = { { 0x8e }, { 0xae } };
#else
	    static const u8 modrm[2][1] = { { 0x8b }, { 0xab } };
#endif
	    ok = append(modrm[block], 1) && appendDWordOffset(offset) && ok;
	} else {
	    // 8-bit displacement
#if defined(PROC_x86_64)
	    static const u8 modrm[2][1] = { { 0x4e}, { 0x6e } };
#else
	    static const u8 modrm[2][1] = { { 0x4b}, { 0x6b } };
#endif
	    u8 byteOffset = offset;
	    ok = append(modrm[block], 1) && append(&byteOffset, 1) && ok;
	}
    }
    return ok;
}


bool SSE2L2NormCodeGenerator::appendDuplicate(int block)
{
    /*
	block = 0
	movdqa xmm2,xmm0         ; xmm2 = xmm0 = u

	block = 1
	movdqa xmm6,xmm4         ; xmm6 = xmm4 = u
    */
    static const u8 code[2][4] = { { 0x66, 0x0f, 0x6f, 0xd0 }, { 0x66, 0x0f, 0x6f, 0xf4 } };
    return append(code[block], sizeof(code[block]));
}

bool SSE2L2NormCodeGenerator::appendSubtract(int block)
{
    /*  block = 0
	psubusb xmm0,xmm1        ; xmm0 = x - u (SIMD subtraction of packed unsigned integers)
	psubusb xmm1,xmm2        ; xmm1 = u - x

	block = 1
	psubusb xmm4,xmm5
	psubusb xmm5,xmm6
    */
    static const u8 code[2][8] = {
	{ 0x66, 0x0f, 0xd8, 0xc1,
	  0x66, 0x0f, 0xd8, 0xca },
	{ 0x66, 0x0f, 0xd8, 0xe5,
	  0x66, 0x0f, 0xd8, 0xee }
    };
    return append(code[block], sizeof(code[block]));
}

bool SSE2L2NormCodeGenerator::appendPrepareForUnpack(int block)
{
    /* 	; get the non negative distance
	block = 0
	por     xmm1,xmm0        ; xmm1 = xmm1 OR mm0 = | x - u |
	movdqa  xmm2,xmm1        ; xmm2 = xmm1
	pxor    xmm0,xmm0        ; xmm0 = 0

	block = 1
	por     xmm5,xmm4
	movdqa  xmm6,xmm5
	pxor    xmm4,xmm4
    */

    static const u8 code[2][12] = {
	{ 0x66, 0x0f, 0xeb, 0xc8,
	  0x66, 0x0f, 0x6f, 0xd1,
	  0x66, 0x0f, 0xef, 0xc0 },
	{ 0x66, 0x0f, 0xeb, 0xec,
	  0x66, 0x0f, 0x6f, 0xf5,
	  0x66, 0x0f, 0xef, 0xe4 }
    };
    return append(code[block], sizeof(code[block]));
}

bool SSE2L2NormCodeGenerator::appendUnpackAndAdd(int block)
{
    /*  block = 0
	; convert bytes to words by interleaving with zero
	punpckhbw xmm1,xmm0      ; unpack and interleave high order bytes from xmm1 with 0
	punpcklbw xmm2,xmm0      ; unpack and interleave low  order bytes from xmm2 with 0
			      ;
	; calculate squared distances for each component
	; adjacent pairs of squared distances are added
	; results are stored in double words
	pmaddwd   xmm1,xmm1      ; multiply and add packed xmm1
	pmaddwd   xmm2,xmm2

	block = 1
	punpckhbw xmm5,xmm4
	punpcklbw xmm6,xmm4
	pmaddwd   xmm5,xmm5
	pmaddwd   xmm6,xmm6


    */
    static const u8 code[2][16] = {
	{ 0x66, 0x0f, 0x68, 0xc8,
	  0x66, 0x0f, 0x60, 0xd0,
	  0x66, 0x0f, 0xf5, 0xc9,
	  0x66, 0x0f, 0xf5, 0xd2 },
	{ 0x66, 0x0f, 0x68, 0xec,
	  0x66, 0x0f, 0x60, 0xf4,
	  0x66, 0x0f, 0xf5, 0xed,
	  0x66, 0x0f, 0xf5, 0xf6 }
    };
    return append(code[block], sizeof(code[block]));
}

bool SSE2L2NormCodeGenerator::appendAccumulate(int block)
{
    /*	; accumulate distances in 4 double words
	block = 0
	paddd  xmm7,xmm1         ; xmm7 += xmm1
	paddd  xmm7,xmm2         ; xmm7 += xmm2

	block = 1
	paddd  xmm7,xmm5
	paddd  xmm7,xmm6

    */
    static const u8 code[2][8] = {
	{ 0x66, 0x0f, 0xfe, 0xf9,
	  0x66, 0x0f, 0xfe, 0xfa },
	{ 0x66, 0x0f, 0xfe, 0xfd,
	  0x66, 0x0f, 0xfe, 0xfe }
    };
    return append(code[block], sizeof(code[block]));
}

bool SSE2L2NormCodeGenerator::appendEpilog()
{
    /*	; calculate results as sum overt the 4 double words in xmm7 (s3 s2 s1 s0)
	pshufd  xmm1,xmm7,238    ; shuffle xmm7 in xmm1 : s2 s3 s2 s3
	pshufd  xmm2,xmm7,68     ; shuffle xmm7 in xmm2 : s0 s1 s0 s1
	paddd   xmm1,xmm2        ; add double words: xmm1 (s2 + s0), (s3 + s1), (s2 + s0), (s3 + s1)
	pshufd  xmm2,xmm1,177    ; shuffle xmm1 in xmm2 : (s3 + s1), (s2 + s0), (s3 + s1), (s2 + s0)
	paddd   xmm1,xmm2        ; add double words: xmm1 (s1 + s2 +s3 + s4) x 4
	; create return value
	movd    eax,xmm1         ; copy lowest double word of xmm1 to eax = result

    */
    /*  x86:
	pop ecx                  ; restore registers
	pop ebx
	pop esi
	ret
    */
    /*  x86-64:
	ret
    */

    static const u8 code[] = {
	0x66, 0x0f, 0x70, 0xcf, 0xee,
	0x66, 0x0f, 0x70, 0xd7, 0x44,
	0x66, 0x0f, 0xfe, 0xca,
	0x66, 0x0f, 0x70, 0xd1, 0xb1,
	0x66, 0x0f, 0xfe, 0xca,
	0x66, 0x0f, 0x7e, 0xc8,
#if !defined(PROC_x86_64)
	0x5b,
	0x5e,
#endif
	0xc3
    };
    return append(code, sizeof(code));
}


SSE2L2NormCodeGenerator::SSE2L2NormCodeGenerator(const Core::Configuration &c, size_t d) :
    CodeGenerator(c)
{
    const u8 blockSize = 16;
    log("using SSE2 instructions for SIMD-diagonal-maximum feature scorer");
    appendProlog();

    /* first block */
    u32 offset = 0;

    appendFetchA(0, offset);
    appendClearAccumulator();
    appendFetchB(0, offset);
    offset += blockSize;
    appendDuplicate(0);
    if (d > blockSize) appendFetchA(1, offset);
    appendSubtract(0);
    if (d > blockSize) {
	appendFetchB(1, offset);
	offset += blockSize;
    }
    appendPrepareForUnpack(0);
    if (d > blockSize) appendDuplicate(1);
    appendUnpackAndAdd(0);

    /* in between blocks */
    size_t block = 0;
    for (; offset < d; offset += blockSize, block = 1 - block) {
	appendSubtract(1 - block);
	appendFetchA(block, offset);
	appendAccumulate(block);
	appendFetchB(block, offset);
	appendPrepareForUnpack(1 - block);
	appendDuplicate(block);
	appendUnpackAndAdd(1 - block);
    }

    /* last block */
    if (d > blockSize) appendSubtract(1 - block);
    appendAccumulate(block);
    if (d > blockSize) {
	appendPrepareForUnpack(1 - block);
	appendUnpackAndAdd(1 - block);
	appendAccumulate(1 - block);
    }

    appendEpilog();
    finalize();
}

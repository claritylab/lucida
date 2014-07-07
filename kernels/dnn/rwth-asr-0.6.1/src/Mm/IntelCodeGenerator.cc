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
#include "IntelCodeGenerator.hh"

using namespace Mm;


bool IntelMMXL2NormCodeGenerator::appendProlog() {
    /*
      push   edi                     ; save registers
      push   esi
      push   ebx
      mov    esi,DWORD PTR [esp+16]  ; esi = &mean[0]
      mov    ebx,DWORD PTR [esp+20]  ; ebx = &featureVector[0]
    */
    static const u8 code[] = {
	0x57, 0x56, 0x53, 0x8b, 0x74, 0x24, 0x10, 0x8b, 0x5c, 0x24, 0x14
    };
    return append(code, sizeof(code));
}

bool IntelMMXL2NormCodeGenerator::appendClearAccumulator() {
    /*
      xor    eax,eax               ; eax = 0
      pxor   mm7,mm7               ; mm7 = 0
    */
    static const u8 code[] = { 0x31, 0xc0, 0x0f, 0xef, 0xff };
    return append(code, sizeof(code));
}

bool IntelMMXL2NormCodeGenerator::appendFetchA(int block, u8 offset) {
    /* block = 0
       movq   mm0,DWORD PTR [esi + offset]   ; copy 64 bits from mean vector in mm0

       block = 1
       movq   mm4,DWORD PTR [esi + offset]   ; move 64 bits from mean vector in mm4

    */
    static const u8 code[2][3] = { { 0x0f, 0x6f, 0x46 }, { 0x0f, 0x6f, 0x66 } };
    return (append(code[block], sizeof(code[block])) && append(&offset, 1));
}

bool IntelMMXL2NormCodeGenerator::appendFetchB(int block, u8 offset) {
    /* block = 0
       movq   mm1,DWORD PTR [ebx + offset]   ; copy 64 bits from feature vector in mm1

       block = 1
       movq   mm5,DWORD PTR [ebx + offset]   ; copy 64 bits from feature vector in mm5
    */
    static const u8 code[2][3] = { { 0x0f, 0x6f, 0x4b }, { 0x0f, 0x6f, 0x6b } };
    return (append(code[block], sizeof(code[block])) && append(&offset, 1));
}

bool IntelMMXL2NormCodeGenerator::appendDuplicate(int block) {
    /* block = 0
       movq mm2,mm0           ; mm2 = mm0

       block = 1
       movq mm6,mm4           ; mm6 = mm4
    */
    static const u8 code[2][3] = { { 0x0f, 0x6f, 0xd0 }, { 0x0f, 0x6f, 0xf4 } };
    return append(code[block], sizeof(code[block]));
}

bool IntelMMXL2NormCodeGenerator::appendSubtract(int block) {
    /* block = 0
       psubusb mm0,mm1        ; mm0 -= mm1 (SIMD subtraction of packed unsigned integers)
       psubusb mm1,mm2        ; mm1 -= mm2

       block = 1
       psubusb mm4,mm5
       psubusb mm5,mm6
    */
    static const u8 code[2][6] = {
	{ 0x0f, 0xd8, 0xc1, 0x0f, 0xd8, 0xca },
	{ 0x0f, 0xd8, 0xe5, 0x0f, 0xd8, 0xee }
    };
    return append(code[block], sizeof(code[block]));
}

bool IntelMMXL2NormCodeGenerator::appendPrepareForUnpack(int block) {
    /* block = 0
       por     mm1,mm0        ; mm1 = mm1 OR mm0
       pxor    mm0,mm0        ; mm0 = 0
       movq    mm2,mm1        ; mm2 = mm1
    */
    /* block = 1
       por     mm5,mm4        ; mm5 = mm5 OR mm4
       pxor    mm4,mm4        ; mm4 = 0
       movq    mm6,mm5        ; mm6 = mm5
    */

    static const u8 code[2][9] = {
	{ 0x0f, 0xeb, 0xc8, 0x0f, 0xef, 0xc0, 0x0f, 0x6f, 0xd1 },
	{ 0x0f, 0xeb, 0xec, 0x0f, 0xef, 0xe4, 0x0f, 0x6f, 0xf5 }
    };
    return append(code[block], sizeof(code[block]));
}

bool IntelMMXL2NormCodeGenerator::appendUnpackAndAdd(int block) {
    /* block = 0
       punpckhbw mm1,mm0      ; mm1 = unpack and interleave high order bytes from mm1 and mm0
			      ; mm0 is zero => convert higher 4 byte integers in mm1 to word integers
       punpcklbw mm2,mm0      ; convert lower 4 byte integers in mm2 to word integers
			      ;
       pmaddwd   mm1,mm1      ; multiply and add packed mm1 = | mm1[3] * mm1[3] + mm1[2] * mm1[2],
			      ;                               | mm1[1] * mm1[1] + mm1[0] * mm1[0]
       pmaddwd   mm2,mm2

       block = 1
       punpckhbw mm5,mm4
       punpcklbw mm6,mm4
       pmaddwd mm5,mm5
       pmaddwd mm6,mm6

    */
    static const u8 code[2][12] = {
	{ 0x0f, 0x68, 0xc8, 0x0f, 0x60, 0xd0, 0x0f, 0xf5, 0xc9, 0x0f, 0xf5, 0xd2 },
	{ 0x0f, 0x68, 0xec, 0x0f, 0x60, 0xf4, 0x0f, 0xf5, 0xed, 0x0f, 0xf5, 0xf6 }
    };
    return append(code[block], sizeof(code[block]));
}

bool IntelMMXL2NormCodeGenerator::appendAccumulate(int block) {
    /* block = 0
       paddd  mm7,mm1            ; mm7 += mm1
       paddd  mm7,mm2            ; mm7 += mm2

       block = 1
       paddd  mm7,mm5
       paddd  mm7,mm6
    */
    static const u8 code[2][6] = {
	{ 0x0f, 0xfe, 0xf9, 0x0f, 0xfe, 0xfa },
	{ 0x0f, 0xfe, 0xfd, 0x0f, 0xfe, 0xfe }
    };
    return append(code[block], sizeof(code[block]));
}

bool IntelMMXL2NormCodeGenerator::appendEpilog() {
    /*
      movq  mm0,mm7        ; mm0 = mm7
      psrlq mm7,0x20       ; right shift mm7 by 32
      paddd mm0,mm7        ; mm0 = mm7 + (mm7 << 32)
      movd eax,mm0         ; copy half of mm0 to eax
      pop ebx              ; restore registers
      pop esi
      pop edi
      ret
    */
    static const u8 code[] = {
	0x0f, 0x6f, 0xc7, 0x0f, 0x73, 0xd7, 0x20, 0x0f, 0xfe, 0xc7, 0x0f, 0x7e,
	0xc0, 0x5b, 0x5e, 0x5f, 0xc3
    };
    return append(code, sizeof(code));
}


IntelMMXL2NormCodeGenerator::IntelMMXL2NormCodeGenerator(const Core::Configuration &c, size_t d) :
    CodeGenerator(c)
{
    if (d > 128) {
	error("does not support dimensions larger than 128 due to signed"
	      "displacement. resetting to 128.");
	d = 128;
    }

    appendProlog();

    /* first block */
    u8 offset = 0;
    appendFetchA(0, offset);
    appendClearAccumulator();
    appendFetchB(0, offset);
    offset += 8;
    appendDuplicate(0);
    if (d > 8) appendFetchA(1, offset);
    appendSubtract(0);
    if (d > 8) {
	appendFetchB(1, offset);
	offset += 8;
    }
    appendPrepareForUnpack(0);
    if (d > 8) appendDuplicate(1);
    appendUnpackAndAdd(0);

    /* in between blocks */
    size_t block = 0;
    for (; offset < d; offset += 8, block = 1 - block) {
	appendSubtract(1 - block);
	appendFetchA(block, offset);
	appendAccumulate(block);
	appendFetchB(block, offset);
	appendPrepareForUnpack(1 - block);
	appendDuplicate(block);
	appendUnpackAndAdd(1 - block);
    }

    /* last block */
    if (d > 8) appendSubtract(1 - block);
    appendAccumulate(block);
    if (d > 8) {
	appendPrepareForUnpack(1 - block);
	appendUnpackAndAdd(1 - block);
	appendAccumulate(1 - block);
    }

    appendEpilog();
    finalize();
}


IntelMMXResetCodeGenerator::IntelMMXResetCodeGenerator(const Core::Configuration &c) :
    CodeGenerator(c)
{
    /*
       emms      ; empty mmx technology state, enable fpu computations
       ret       ;
    */
    static const u8 code[] = { 0x0f, 0x77, 0xc3 };
    append(code, sizeof(code));
    finalize();
}

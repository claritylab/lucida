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
#include <stdio.h>

#include "Assertions.hh"
#include "MD5.hh"
#include "Types.hh"

using Core::MD5;


/*****************************************************************************/
MD5::MD5()
    /*****************************************************************************/
{
    A_ = 0x67452301;
    B_ = 0xefcdab89;
    C_ = 0x98badcfe;
    D_ = 0x10325476;

    nblocks = 0;
    memset(buf, 0, 64);
    count = 0;
    isFinalized_ = false;
}

/* These are the four functions used in the four steps of the MD5 algorithm
   and defined in the RFC 1321.  The first function is a little bit optimized
   (as found in Colin Plumbs public domain implementation).  */
/* #define FF(b, c, d) ((b & c) | (~b & d)) */
#define FF(b, c, d) (d ^ (b & (c ^ d)))
#define FG(b, c, d) FF (d, b, c)
#define FH(b, c, d) (b ^ c ^ d)
#define FI(b, c, d) (c ^ (b | ~d))
#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/*****************************************************************************/
void MD5::transform(const void *data) const
    /*****************************************************************************/
{
    u32 correct_words[16];
    u32 A = A_;
    u32 B = B_;
    u32 C = C_;
    u32 D = D_;
    u32 *cwp = correct_words;

#if __BYTE_ORDER == __BIG_ENDIAN
    {
	int i;
	u8 *p2, *p1;
	for(i = 0, p1 = (u8*)data, p2 = (u8*)correct_words; i < 16; i++, p2 += 4) {
	    p2[3] = *p1++;
	    p2[2] = *p1++;
	    p2[1] = *p1++;
	    p2[0] = *p1++;
	}
    }
#elif __BYTE_ORDER == __LITTLE_ENDIAN
    memcpy(correct_words, data, 64);
#else
#error "Machine has unsupported byte order!"
#endif


#define OP(a, b, c, d, s, T) \
  do \
    { \
      a += FF(b, c, d) + (*cwp++) + T; \
      a = rol(a, s); \
      a += b; \
    } \
  while (0)

    // round 1
    OP(A, B, C, D,  7, 0xd76aa478);
    OP(D, A, B, C, 12, 0xe8c7b756);
    OP(C, D, A, B, 17, 0x242070db);
    OP(B, C, D, A, 22, 0xc1bdceee);
    OP(A, B, C, D,  7, 0xf57c0faf);
    OP(D, A, B, C, 12, 0x4787c62a);
    OP(C, D, A, B, 17, 0xa8304613);
    OP(B, C, D, A, 22, 0xfd469501);
    OP(A, B, C, D,  7, 0x698098d8);
    OP(D, A, B, C, 12, 0x8b44f7af);
    OP(C, D, A, B, 17, 0xffff5bb1);
    OP(B, C, D, A, 22, 0x895cd7be);
    OP(A, B, C, D,  7, 0x6b901122);
    OP(D, A, B, C, 12, 0xfd987193);
    OP(C, D, A, B, 17, 0xa679438e);
    OP(B, C, D, A, 22, 0x49b40821);

#undef OP
#define OP(f, a, b, c, d, k, s, T)  \
    do								      \
      { 							      \
	a += f (b, c, d) + correct_words[k] + T;		      \
	a = rol(a, s);						      \
	a += b; 						      \
      } 							      \
    while (0)

    // round 2
    OP(FG, A, B, C, D,  1,  5, 0xf61e2562);
    OP(FG, D, A, B, C,  6,  9, 0xc040b340);
    OP(FG, C, D, A, B, 11, 14, 0x265e5a51);
    OP(FG, B, C, D, A,  0, 20, 0xe9b6c7aa);
    OP(FG, A, B, C, D,  5,  5, 0xd62f105d);
    OP(FG, D, A, B, C, 10,  9, 0x02441453);
    OP(FG, C, D, A, B, 15, 14, 0xd8a1e681);
    OP(FG, B, C, D, A,  4, 20, 0xe7d3fbc8);
    OP(FG, A, B, C, D,  9,  5, 0x21e1cde6);
    OP(FG, D, A, B, C, 14,  9, 0xc33707d6);
    OP(FG, C, D, A, B,  3, 14, 0xf4d50d87);
    OP(FG, B, C, D, A,  8, 20, 0x455a14ed);
    OP(FG, A, B, C, D, 13,  5, 0xa9e3e905);
    OP(FG, D, A, B, C,  2,  9, 0xfcefa3f8);
    OP(FG, C, D, A, B,  7, 14, 0x676f02d9);
    OP(FG, B, C, D, A, 12, 20, 0x8d2a4c8a);

    // round 3
    OP(FH, A, B, C, D,  5,  4, 0xfffa3942);
    OP(FH, D, A, B, C,  8, 11, 0x8771f681);
    OP(FH, C, D, A, B, 11, 16, 0x6d9d6122);
    OP(FH, B, C, D, A, 14, 23, 0xfde5380c);
    OP(FH, A, B, C, D,  1,  4, 0xa4beea44);
    OP(FH, D, A, B, C,  4, 11, 0x4bdecfa9);
    OP(FH, C, D, A, B,  7, 16, 0xf6bb4b60);
    OP(FH, B, C, D, A, 10, 23, 0xbebfbc70);
    OP(FH, A, B, C, D, 13,  4, 0x289b7ec6);
    OP(FH, D, A, B, C,  0, 11, 0xeaa127fa);
    OP(FH, C, D, A, B,  3, 16, 0xd4ef3085);
    OP(FH, B, C, D, A,  6, 23, 0x04881d05);
    OP(FH, A, B, C, D,  9,  4, 0xd9d4d039);
    OP(FH, D, A, B, C, 12, 11, 0xe6db99e5);
    OP(FH, C, D, A, B, 15, 16, 0x1fa27cf8);
    OP(FH, B, C, D, A,  2, 23, 0xc4ac5665);

    // round 4
    OP(FI, A, B, C, D,  0,  6, 0xf4292244);
    OP(FI, D, A, B, C,  7, 10, 0x432aff97);
    OP(FI, C, D, A, B, 14, 15, 0xab9423a7);
    OP(FI, B, C, D, A,  5, 21, 0xfc93a039);
    OP(FI, A, B, C, D, 12,  6, 0x655b59c3);
    OP(FI, D, A, B, C,  3, 10, 0x8f0ccc92);
    OP(FI, C, D, A, B, 10, 15, 0xffeff47d);
    OP(FI, B, C, D, A,  1, 21, 0x85845dd1);
    OP(FI, A, B, C, D,  8,  6, 0x6fa87e4f);
    OP(FI, D, A, B, C, 15, 10, 0xfe2ce6e0);
    OP(FI, C, D, A, B,  6, 15, 0xa3014314);
    OP(FI, B, C, D, A, 13, 21, 0x4e0811a1);
    OP(FI, A, B, C, D,  4,  6, 0xf7537e82);
    OP(FI, D, A, B, C, 11, 10, 0xbd3af235);
    OP(FI, C, D, A, B,  2, 15, 0x2ad7d2bb);
    OP(FI, B, C, D, A,  9, 21, 0xeb86d391);

    // put checksum in context given as argument
    A_ += A;
    B_ += B;
    C_ += C;
    D_ += D;
}

/*****************************************************************************/
void MD5::update(const char *v, u32 n) const
    /*****************************************************************************/
{
    u32 pos = 0;

    require_(!isFinalized_);

    // flush the buffer
    if (count == 64) {
	transform(buf);
	count = 0;
	nblocks++;
    }
    if (!v) return;
    if (count) {
	for (; n && count < 64; n--) buf[count++] = v[pos++];
	update(NULL, 0);
	if (!n) return;
    }

    while (n >= 64) {
	transform(v);
	count = 0;
	nblocks++;
	n -= 64;
	v += 64;
    }
    for (; n && count < 64; n--) buf[count++] = v[pos++];
}

void MD5::update(const std::string &data) const {
    update(data.data(), data.size());
}

/*****************************************************************************/
bool MD5::updateFromFile(const std::string &filename)
    /*****************************************************************************/
{
    FILE *f = fopen(filename.c_str(), "r");
    if (!f) return false;
    size_t n;
    char buffer[256];
    do {
	n = fread(buffer, 1, 256, f);
	update(buffer, n);
    } while (n > 0);
    fclose(f);
    return true;
}

/*****************************************************************************/
void MD5::finalize(void) const
/*****************************************************************************/
{
    u32 t, msb, lsb;

    require(!isFinalized_);

    // flush
    update(NULL, 0);
    msb = 0;
    t = nblocks;

    // multiply by 64 to make a byte count
    if ((lsb = t << 6) < t) msb++;
    msb += t >> 26;
    t = lsb;

    // add the count
    if ((lsb = t + count) < t) msb++;
    t = lsb;

    // multiply by 8 to make a bit count
    if( (lsb = t << 3) < t ) msb++;
    msb += t >> 29;

    // enough room
    if (count < 56) {
	/* pad */
	buf[count++] = 0x80;
	while (count < 56) buf[count++] = 0;
    } else {
	// need one extra block
	// pad character
	buf[count++] = 0x80;
	while (count < 64) buf[count++] = 0;

	// flush
	update(NULL, 0);

	// fill next block with zeroes
	memset(buf, 0, 56);
    }

    // append the 64 bit count
    buf[56] = lsb;
    buf[57] = lsb >>  8;
    buf[58] = lsb >> 16;
    buf[59] = lsb >> 24;
    buf[60] = msb;
    buf[61] = msb >>  8;
    buf[62] = msb >> 16;
    buf[63] = msb >> 24;
    transform(buf);

    u32 *buf32 = (u32*) buf;
    buf32[0] = A_;
    buf32[1] = B_;
    buf32[2] = C_;
    buf32[3] = D_;
#if __BYTE_ORDER == __BIG_ENDIAN
    swapEndianess<4>(buf32, 4);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#else
#error "Machine has unsupported byte order!"
#endif

    isFinalized_ = true;
}

MD5::operator std::string() const {
    if (!isFinalized_) finalize();
    std::string result;
    for (int i = 0; i < 16; i++) result += form("%02x", buf[i]);
    return result;
}

std::ostream& Core::operator<< (std::ostream &o, const MD5 &m) {
    if (!m.isFinalized_) m.finalize();
    for (int i = 0; i < 16; i++) o << form("%02x", m.buf[i]);
    return o;
}

Core::BinaryOutputStream& Core::operator<< (BinaryOutputStream &o, const MD5 &m) {
    if (!m.isFinalized_) m.finalize();
    for (int i = 0; i < 16; i++) o << m.buf[i];
    return o;
}

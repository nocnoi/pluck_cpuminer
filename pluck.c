/*
 * Copyright 2009 Colin Percival, 2011 ArtForz, 2011-2014 pooler, 2015 Jordan Earls
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "cpuminer-config.h"
#include "miner.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <emmintrin.h>

//windows hack bleh
#ifndef htobe32
#define htobe32(x)  ((u_int32_t)htonl((u_int32_t)(x)))
#endif

//#define USE_SSE2 1

#define ROTL(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
//note, this is 64 bytes
static inline void xor_salsa8(uint32_t B[16], const uint32_t Bx[16])
{
    uint32_t x00,x01,x02,x03,x04,x05,x06,x07,x08,x09,x10,x11,x12,x13,x14,x15;
    int i;

    x00 = (B[ 0] ^= Bx[ 0]);
    x01 = (B[ 1] ^= Bx[ 1]);
    x02 = (B[ 2] ^= Bx[ 2]);
    x03 = (B[ 3] ^= Bx[ 3]);
    x04 = (B[ 4] ^= Bx[ 4]);
    x05 = (B[ 5] ^= Bx[ 5]);
    x06 = (B[ 6] ^= Bx[ 6]);
    x07 = (B[ 7] ^= Bx[ 7]);
    x08 = (B[ 8] ^= Bx[ 8]);
    x09 = (B[ 9] ^= Bx[ 9]);
    x10 = (B[10] ^= Bx[10]);
    x11 = (B[11] ^= Bx[11]);
    x12 = (B[12] ^= Bx[12]);
    x13 = (B[13] ^= Bx[13]);
    x14 = (B[14] ^= Bx[14]);
    x15 = (B[15] ^= Bx[15]);
    for (i = 0; i < 8; i += 2) {
        /* Operate on columns. */
        x04 ^= ROTL(x00 + x12,  7);  x09 ^= ROTL(x05 + x01,  7);
        x14 ^= ROTL(x10 + x06,  7);  x03 ^= ROTL(x15 + x11,  7);

        x08 ^= ROTL(x04 + x00,  9);  x13 ^= ROTL(x09 + x05,  9);
        x02 ^= ROTL(x14 + x10,  9);  x07 ^= ROTL(x03 + x15,  9);

        x12 ^= ROTL(x08 + x04, 13);  x01 ^= ROTL(x13 + x09, 13);
        x06 ^= ROTL(x02 + x14, 13);  x11 ^= ROTL(x07 + x03, 13);

        x00 ^= ROTL(x12 + x08, 18);  x05 ^= ROTL(x01 + x13, 18);
        x10 ^= ROTL(x06 + x02, 18);  x15 ^= ROTL(x11 + x07, 18);

        /* Operate on rows. */
        x01 ^= ROTL(x00 + x03,  7);  x06 ^= ROTL(x05 + x04,  7);
        x11 ^= ROTL(x10 + x09,  7);  x12 ^= ROTL(x15 + x14,  7);

        x02 ^= ROTL(x01 + x00,  9);  x07 ^= ROTL(x06 + x05,  9);
        x08 ^= ROTL(x11 + x10,  9);  x13 ^= ROTL(x12 + x15,  9);

        x03 ^= ROTL(x02 + x01, 13);  x04 ^= ROTL(x07 + x06, 13);
        x09 ^= ROTL(x08 + x11, 13);  x14 ^= ROTL(x13 + x12, 13);

        x00 ^= ROTL(x03 + x02, 18);  x05 ^= ROTL(x04 + x07, 18);
        x10 ^= ROTL(x09 + x08, 18);  x15 ^= ROTL(x14 + x13, 18);
    }
    B[ 0] += x00;
    B[ 1] += x01;
    B[ 2] += x02;
    B[ 3] += x03;
    B[ 4] += x04;
    B[ 5] += x05;
    B[ 6] += x06;
    B[ 7] += x07;
    B[ 8] += x08;
    B[ 9] += x09;
    B[10] += x10;
    B[11] += x11;
    B[12] += x12;
    B[13] += x13;
    B[14] += x14;
    B[15] += x15;
}

#ifdef USE_SSE2

static inline void xor_salsa8_sse2(__m128i B[4], const __m128i Bx[4])
{
  __m128i X0, X1, X2, X3;
  __m128i T;
  int i;

  X0 = B[0] = _mm_xor_si128(B[0], Bx[0]);
  X1 = B[1] = _mm_xor_si128(B[1], Bx[1]);
  X2 = B[2] = _mm_xor_si128(B[2], Bx[2]);
  X3 = B[3] = _mm_xor_si128(B[3], Bx[3]);

  for (i = 0; i < 8; i += 2) {
    /* Operate on "columns". */
    T = _mm_add_epi32(X0, X3);
    X1 = _mm_xor_si128(X1, _mm_slli_epi32(T, 7));
    X1 = _mm_xor_si128(X1, _mm_srli_epi32(T, 25));
    T = _mm_add_epi32(X1, X0);
    X2 = _mm_xor_si128(X2, _mm_slli_epi32(T, 9));
    X2 = _mm_xor_si128(X2, _mm_srli_epi32(T, 23));
    T = _mm_add_epi32(X2, X1);
    X3 = _mm_xor_si128(X3, _mm_slli_epi32(T, 13));
    X3 = _mm_xor_si128(X3, _mm_srli_epi32(T, 19));
    T = _mm_add_epi32(X3, X2);
    X0 = _mm_xor_si128(X0, _mm_slli_epi32(T, 18));
    X0 = _mm_xor_si128(X0, _mm_srli_epi32(T, 14));

    /* Rearrange data. */
    X1 = _mm_shuffle_epi32(X1, 0x93);
    X2 = _mm_shuffle_epi32(X2, 0x4E);
    X3 = _mm_shuffle_epi32(X3, 0x39);

    /* Operate on "rows". */
    T = _mm_add_epi32(X0, X1);
    X3 = _mm_xor_si128(X3, _mm_slli_epi32(T, 7));
    X3 = _mm_xor_si128(X3, _mm_srli_epi32(T, 25));
    T = _mm_add_epi32(X3, X0);
    X2 = _mm_xor_si128(X2, _mm_slli_epi32(T, 9));
    X2 = _mm_xor_si128(X2, _mm_srli_epi32(T, 23));
    T = _mm_add_epi32(X2, X3);
    X1 = _mm_xor_si128(X1, _mm_slli_epi32(T, 13));
    X1 = _mm_xor_si128(X1, _mm_srli_epi32(T, 19));
    T = _mm_add_epi32(X1, X2);
    X0 = _mm_xor_si128(X0, _mm_slli_epi32(T, 18));
    X0 = _mm_xor_si128(X0, _mm_srli_epi32(T, 14));

    /* Rearrange data. */
    X1 = _mm_shuffle_epi32(X1, 0x39);
    X2 = _mm_shuffle_epi32(X2, 0x4E);
    X3 = _mm_shuffle_epi32(X3, 0x93);
  }

  B[0] = _mm_add_epi32(B[0], X0);
  B[1] = _mm_add_epi32(B[1], X1);
  B[2] = _mm_add_epi32(B[2], X2);
  B[3] = _mm_add_epi32(B[3], X3);
}

#endif



static inline void assert(int cond)
{
  if(!cond)
  {
    printf("error\n");
    exit(1);
  }
}
//computes a single sha256 hash
void sha256_hash(unsigned char *hash, const unsigned char *data, int len)
{
  uint32_t S[16] __attribute__((aligned(32))), T[16] __attribute__((aligned(32)));
  int i, r;

  sha256_init(S);
  for (r = len; r > -9; r -= 64) {
    if (r < 64)
      memset(T, 0, 64);
    memcpy(T, data + len - r, r > 64 ? 64 : (r < 0 ? 0 : r));
    if (r >= 0 && r < 64)
      ((unsigned char *)T)[r] = 0x80;
    for (i = 0; i < 16; i++)
      T[i] = be32dec(T + i);
    if (r < 56)
      T[15] = 8 * len;
    sha256_transform(S, T, 0);
  }
  for (i = 0; i < 8; i++)
    be32enc((uint32_t *)hash + i, S[i]);
}

//hash exactly 64 bytes (ie, sha256 block size)
void sha256_hash512(unsigned char *hash, const unsigned char *data)
{
  uint32_t S[16] __attribute__((aligned(32))), T[16] __attribute__((aligned(32)));
  int i, r;

  sha256_init(S);

    memcpy(T, data, 64);
    for (i = 0; i < 16; i++)
      T[i] = be32dec(T + i);
    sha256_transform(S, T, 0);

    memset(T, 0, 64);
    //memcpy(T, data + 64, 0);
    ((unsigned char *)T)[0] = 0x80;
    for (i = 0; i < 16; i++)
      T[i] = be32dec(T + i);
      T[15] = 8 * 64;
    sha256_transform(S, T, 0);

  for (i = 0; i < 8; i++)
    be32enc((uint32_t *)hash + i, S[i]);
}

//static const int HASH_MEMORY=128*1024;
static const int BLOCK_HEADER_SIZE=80;

int scanhash_pluck(int thr_id, uint32_t *pdata,
  unsigned char *scratchbuf, const uint32_t *ptarget,
  uint32_t max_nonce, unsigned long *hashes_done, int N)
{
  uint32_t data[20], hash[8];
  uint32_t S[16];
  uint32_t n = pdata[19] - 1;
  const int HASH_MEMORY=N*1024;
  const uint32_t first_nonce = pdata[19];
  const uint32_t Htarg = ptarget[7];
  int throughput = 1;
  int counti;
  
  
    memcpy(data, pdata, 80);

     for (int a = 0; a < 20; a++)
      be32enc((uint32_t *)data + a, (((uint32_t*)pdata))[a]); 

  do {
      data[19] = ++n; //incrementing nonce

    uint8_t *hashbuffer = scratchbuf; //don't allocate this on stack, since it's huge.. 
    //allocating on heap adds hardly any overhead on Linux
    int size=HASH_MEMORY;
    memset(hashbuffer, 0, 64); 
    sha256_hash(&hashbuffer[0], data, BLOCK_HEADER_SIZE);
    

    for (int i = 64; i < size-32; i+=32)
    {
        //i-4 because we use integers for all references against this, and we don't want to go 3 bytes over the defined area
        int randmax = i-4; //we could use size here, but then it's probable to use 0 as the value in most cases
        uint32_t joint[16];
        uint32_t randbuffer[16];
//        assert(i-32>0);
//        assert(i<size);
        uint32_t randseed[16];
        assert(sizeof(int)*16 == 64);

        //setup randbuffer to be an array of random indexes
        memcpy(randseed, &hashbuffer[i-64], 64);
        if(i>128)
        {
            memcpy(randbuffer, &hashbuffer[i-128], 64);
        }else
        {
            memset(&randbuffer, 0, 64);
        }
        xor_salsa8(randbuffer, randseed);
        memcpy(joint, &hashbuffer[i-32], 32);
        //use the last hash value as the seed
        for (int j = 32; j < 64; j+=4)
        {
            assert((j - 32) / 2 < 16);
            //every other time, change to next random index
            uint32_t rand = randbuffer[(j - 32)/4] % (randmax-32); //randmax - 32 as otherwise we go beyond memory that's already been written to
//            assert(j>0 && j<64);
//            assert(rand<size);
//            assert(j/2 < 64);
            joint[j/4] = *((uint32_t*)&hashbuffer[rand]);
        }
//        assert(i>=0 && i+32<size);

        sha256_hash512(&hashbuffer[i], joint);

        //setup randbuffer to be an array of random indexes
        memcpy(randseed, &hashbuffer[i-32], 64); //use last hash value and previous hash value(post-mixing)
        if(i>128)
        {
            memcpy(randbuffer, &hashbuffer[i-128], 64);
        }else
        {
            memset(randbuffer, 0, 64);
        }
        //#ifdef USE_SSE2
        //xor_salsa8_sse2((__m128i*)randbuffer, (__m128i*)randseed);
        //#else
        xor_salsa8(randbuffer, randseed);
       // #endif

        //use the last hash value as the seed
        for (int j = 0; j < 32; j+=2)
        {
//            assert(j/4 < 16);
            uint32_t rand = randbuffer[j/2] % randmax;
//            assert(rand < size);
//            assert((j/4)+i >= 0 && (j/4)+i < size);
//            assert(j + i -4 < i + 32); //assure we dont' access beyond the hash
            *((uint32_t*)&hashbuffer[rand]) = *((uint32_t*)&hashbuffer[j + i - 4]);
        }
    }
    //note: off-by-one error is likely here...     
    for (int i = size-64-1; i >= 64; i -= 64)       
    {      
//      assert(i-64 >= 0);       
//      assert(i+64<size);    
      sha256_hash512(&hashbuffer[i-64], &hashbuffer[i]);
     }


    memcpy((unsigned char*)hash, hashbuffer, 32);
      if (hash[7] <= Htarg && fulltest(hash, ptarget)) {
        *hashes_done = n - pdata[19] + 1;
        pdata[19] = htobe32(data[19]);
        return 1;
      }
  } while (n < max_nonce && !work_restart[thr_id].restart);
  
  *hashes_done = n - first_nonce + 1;
  pdata[19] = n;
  return 0;
}

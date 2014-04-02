/*
 * Copyright (c) 2012-2014, Magnus Skjegstad / Forsvarets Forskningsinstitutt
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Halvdan Grelland, Jostein Aardal
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include "upack.h"

int main (void){

    printf("Running upack tests...\n");
    
    // Test double encode/decode
        
    double d1 = 159.123;
    double d2 = -9478421.42425;

    uint64_t d1_p = enc754(d1);
    uint64_t d2_p = enc754(d2);

    double d1_u = dec754(d1_p);
    double d2_u = dec754(d2_p);

    assert(d1 == d1_u);
    assert(d2 == d2_u);

    printf("<float OK>\n");

    // Test uint8_t
    
    uint8_t s = 123;
    unsigned char* s_p = malloc(1);
    pack8(s_p, s);
    
    uint8_t s_u = unpacku8(s_p);

    assert(s == s_u);

    printf("<unsigned 8 OK>\n");
    free(s_p);

    // Test uint16_t

    uint16_t u = 0xC0;
    uint16_t u0 = SHRT_MAX;

    unsigned char u_p[2];
    unsigned char u0_p[2];

    pack16(u_p, u);
    pack16(u0_p, u0);

    uint16_t u_u = unpacku16(u_p);
    uint16_t u0_u = unpacku16(u0_p);

    assert(u == u_u);
    assert(u0 == u0_u);

    printf("<unsigned 16 OK>\n");

    // Test uint32_t

    uint32_t u1 = 123456;
    uint32_t u2 = UINT_MAX;

    unsigned char u1_p[4];
    unsigned char u2_p[4];

    pack32(u1_p, u1);
    pack32(u2_p, u2);

    uint32_t u1_u = unpacku32(u1_p);
    uint32_t u2_u = unpacku32(u2_p);

    assert(u1 == u1_u);
    assert(u2 == u2_u);

    printf("<unsigned 32 OK>\n");

    // Test uint64_t

    uint64_t u3 = 123345678;
    uint64_t u4 = ULONG_MAX;

    unsigned char u3_p[8];
    unsigned char u4_p[8];

    pack64(u3_p, u3);
    pack64(u4_p, u4);

    uint64_t u3_u = unpacku64(u3_p);
    uint64_t u4_u = unpacku64(u4_p);

    assert(u3 == u3_u);
    assert(u4 == u4_u);

    printf("<unsigned 64 OK>\n");

    // Test double->encode->pack->unpack->decode
    
    d1_u = 0.0f;
    d2_u = 0.0f;

    unsigned char dbl1[8];
    unsigned char dbl2[8];

    packdouble(dbl1, d1);
    packdouble(dbl2, d2);

    d1_u = unpackdouble(dbl1);
    d2_u = unpackdouble(dbl2);

    assert(d1_u == d1);
    assert(d2_u == d2);

    printf("<double encode/decode/pack successful>\n");

    return 0;
}

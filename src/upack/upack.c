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

/* microPack - Portable data serialization micro-library for p2p-dprd 
 *  -- Heavily based on Beej's Network Guide example serialization routines)
 *      See --> http://beej.us/guide/bgnet/examples/pack2.c 
 */

#include "upack.h"

#define uchar unsigned char

/* Pack and unpack routines */

/*
** enc754() -- pack a floating point number into IEEE-754 format
*/ 
uint64_t enc754(double f){
	double fnorm;
	int32_t shift;
	int64_t sign, exp, significand;
	uint32_t significandbits = 52; //64 - 11 - 1 && -1 for sign bit

	if (f == 0.0) return 0; // get this special case out of the way

	// check sign and begin normalization
	if (f < 0) { sign = 1; fnorm = -f; }
	else { sign = 0; fnorm = f; }

	// get the normalized form of f and track the exponent
	shift = 0;
	while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
	while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
	fnorm = fnorm - 1.0;

	// calculate the binary form (non-float) of the significand data
	significand = fnorm * ((1LL<<significandbits) + 0.5f);

	// get the biased exponent
	exp = shift + ((1<<(10)) - 1); // shift + bias

	// return the final answer
	return (sign<<(63)) | (exp<<(52)) | significand;
}

/*
** dec754() -- decode a floating point number from IEEE-754 format
*/ 
double dec754(uint64_t i){
	double result;
	int64_t shift;
	uint32_t bias;
	uint32_t significandbits = 52; //64 - 11 - 1 && -1 for sign bit

	if (i == 0) return 0.0;

	// pull the significand
	result = (i&((1LL<<significandbits)-1)); // mask
	result /= (1LL<<significandbits); // convert back to float
	result += 1.0f; // add the one back on

	// deal with the exponent
	bias = (1<<(11-1)) - 1;
	shift = ((i>>significandbits)&((1LL<<11)-1)) - bias;
	while(shift > 0) { result *= 2.0; shift--; }
	while(shift < 0) { result /= 2.0; shift++; }

	// sign it
	result *= (i>>(63))&1? -1.0: 1.0;

	return result;
}


/*
** pack8() -- store an 8-bit int into a char buffer (a simple wrapper, really)
*/
void pack8(uchar *buf, uint8_t i){
    *buf++ = (unsigned char)i;
}

uint8_t unpacku8(uchar *buf){
    return (uint8_t)*buf++;
}

/*
** pack16() -- store a 16-bit int into a char buffer (like htons())
*/ 
void pack16(uchar *buf, uint16_t i)
{
    *buf++ = i>>8; *buf++ = i;
}

/*
** unpacku16() -- unpack a 16-bit unsigned from a char buffer (like ntohs())
*/ 
uint16_t unpacku16(unsigned char *buf)
{
        return ((uint16_t)buf[0]<<8) | buf[1];
}

/*
** pack32() -- store a 32-bit int into a char buffer (like htonl())
*/
void pack32(uchar *buf, uint32_t i) {
    *buf++ = i>>24; *buf++ = i>>16;
    *buf++ = i>>8;  *buf++ = i;
}

/*
** unpacku32() -- unpack a 32-bit unsigned from a char buffer (like ntohl())
*/ 
uint32_t unpacku32(uchar *buf){
    return ((uint32_t)buf[0]<<24) |
           ((uint32_t)buf[1]<<16) |   
           ((uint32_t)buf[2]<<8)  |
           buf[3];
}

/*
 * ** pack64() -- store a 64-bit int into a char buffer (like htonl())
 * */ 
void pack64(uchar *buf, uint64_t i){
    *buf++ = i>>56; *buf++ = i>>48;
    *buf++ = i>>40; *buf++ = i>>32;
    *buf++ = i>>24; *buf++ = i>>16; 
    *buf++ = i>>8;  *buf++ = i;
}

/*
 * ** unpacku64() -- unpack a 64-bit unsigned from a char buffer (like ntohl())
 * */ 
uint64_t unpacku64(uchar *buf){
    return ((uint64_t)buf[0]<<56) |
           ((uint64_t)buf[1]<<48) |
           ((uint64_t)buf[2]<<40) |
           ((uint64_t)buf[3]<<32) |
           ((uint64_t)buf[4]<<24) |
           ((uint64_t)buf[5]<<16) |
           ((uint64_t)buf[6]<<8)  |
           buf[7];
}

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
 */
/*
 * serialize.h
 *
 * Declaration of functions handling conversion between internal and binary (network) data formats.
 *
 *      Author: Halvdan Hoem Grelland and Jostein Aardal
 */

#ifndef INCLUDE_SERIALIZE_H_
#define INCLUDE_SERIALIZE_H_

/* Forward-declare datatypes to prevent circular reference */
#ifndef INCLUDE_NODE_H

//typedef struct NodeCollection NodeCollection;
//typedef struct Node Node;
//typedef enum payloadType payloadType;
//typedef struct LocalRequest LocalRequest;

#endif

#include <stdlib.h>

#include "protocol.h"
#include "node.h"
#include "io.h"

/* Constants used while packing/unpacking structured data */
#define NODECOLL_VAR_CNT 3
#define NODE_VAR_CNT 9

/* Pack a NodeCollection to a byte buffer
 * 	Arguments:
 * 		nc 		- NodeCollection* to pack in byte buffer
 * 		size 	- Used to return size of resulting buffer
 *
 * 	Return:
 * 		char*	- Pointer to resulting byte buffer
 */
unsigned char* NodeCollection_pack(NodeCollection* nc, int* size);

/* Unpack a NodeCollection from a byte buffer
 *  Arguments:
 *  	buff	- Buffer to unpack to
 *  	size	- Size of buffer
 *  	num		- Amount of Nodes are written to num
 *
 * 	Return:
 * 		NodeCollection* - Pointer to unpacked NodeCollection
 */
NodeCollection* NodeCollection_unpack(unsigned char* buff, int size, int *num);

/* Unpack a locally received request from byte buffer
 *	Arguments:
 *		buff	- Buffer to unpack to
 *		size	- Size of buffer
 *
 *	Return:
 *		LocalRequest* - Pointer to unpacked LocalRequest-object
 */
LocalRequest* LocalRequest_unpack(unsigned char *buffer, int buffer_size);


#endif /* INCLUDE_SERIALIZE_H_ */

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
 * Author: Halvdan Hoem Grelland - halvdanhg@gmail.com - FFI
 */


/*
 * serialize.c
 *
 * Simple implementation of serialize.h using uPack
 *
 */

#include "upack/upack.h"
#include "serialize.h"

/* size(versionId, payloadType, nodeCount) = 5 bytes */
#define NC_HEADER_OFFSET 5
/* size(Node) in bytes */
#define NODE_OFFSET ( (4 * 4) + \
                      (3 * 2) + \
                      (2 * 8))

/* Serialize a NodeColletion to a byte-buffer */
unsigned char* NodeCollection_pack(NodeCollection* nc, int* size){
    unsigned char* buff = NULL; /* Return buffer  */
    *size = 0;
    /* (Superficially) check validity of input data */
    if(NodeCollection_isValid(nc)){
        
        /* Calculate needed buffer size */
        buff = malloc(NC_HEADER_OFFSET + (nc->nodeCount * NODE_OFFSET));    

        /* Pack fields using upack */
        int sz = 0;
        pack16(buff, nc->versionID);        sz += 2;
        pack8(buff + sz, nc->payloadType);  sz++;
        pack16(buff + sz, nc->nodeCount);   sz += 2;

        /* Pack each node in buffer successively */
        int i;
        for(i = 0 ; i < nc->nodeCount ; i++){
            pack32(buff + sz, nc->nodes[i].nodeID);     sz += 4;
            packdouble(buff + sz, nc->nodes[i].lat);    sz += 8;
            packdouble(buff + sz, nc->nodes[i].lon);    sz += 8;
            pack16(buff + sz, nc->nodes[i].coordRange); sz += 2;
            pack32(buff + sz, nc->nodes[i].ipAddr);     sz += 4;
            pack16(buff + sz, nc->nodes[i].port);       sz += 2;
            pack32(buff + sz, nc->nodes[i].radac_ip);   sz += 4;
            pack16(buff + sz, nc->nodes[i].radac_port); sz += 2;
            pack32(buff + sz, nc->nodes[i].timeStamp);  sz += 4;
       }
       *size = sz;

       /* TEST */
       

   }
   return buff;
}

NodeCollection* NodeCollection_unpack(unsigned char* buff, int size, int* num){
    NodeCollection* nc = NULL;
    
    if(buff != NULL && size > 0){
        int o = 0; // Track buffer offset

        /* Unpack header fields */
        uint16_t versionID = unpacku16(buff);       o += 2;
        uint8_t payloadType = unpacku8(buff + o);   o += 1;
        uint16_t nodeCount = unpacku16(buff + o);   o += 2;
       
        nc = NodeCollection_new(versionID, payloadType, nodeCount);
        *num = nodeCount;
        nc->nodeCount = nodeCount;

        int i; /* Track node number */
        for(i = 0 ; i < nc->nodeCount ; i++){
            nc->nodes[i].nodeID = unpacku32(buff + o);      o += 4;
            nc->nodes[i].lat = unpackdouble(buff + o);      o += 8;
            nc->nodes[i].lon = unpackdouble(buff + o);      o += 8;
            nc->nodes[i].coordRange = unpacku16(buff + o);  o += 2;
            nc->nodes[i].ipAddr = unpacku32(buff + o);      o += 4;
            nc->nodes[i].port = unpacku16(buff + o);        o += 2;
            nc->nodes[i].radac_ip = unpacku32(buff + o);    o += 4;
            nc->nodes[i].radac_port = unpacku16(buff + o);  o += 2;
            nc->nodes[i].timeStamp = unpacku32(buff + o);   o += 4;
        }
    }
    return nc;
}

LocalRequest* LocalRequest_unpack(unsigned char* buff, int buff_size){
    LocalRequest* lr = NULL;
    
    if(buff != NULL && buff_size > 0){
        lr = LocalRequest_new(-1, 0, 0, 0, " ");
        
        int o = 0, bytes = 0; /* Buffer offset, string buffer size */
        lr->type = unpacku8(buff);  o += 1;

        switch(lr->type){
            case SET_POSITION:
                lr->values->lat = unpackdouble(buff + o);   o += 8;
                lr->values->lon = unpackdouble(buff + o);   o += 8;
                break;
            case SET_COORDINATION_RANGE:
                lr->values->coord_range = unpacku16(buff + o);  o += 2;
                break;
            case SET_POS_AND_RANGE:
                printf("\nWARNING: Deprecated SET_POS_AND_RANGE. Local request ignored.\n\n");
                LocalRequest_destroy(lr);
                return NULL;
            case SUB_CANDNODES:
                bytes = snprintf(lr->values->sock_addr, LOCAL_ADDR_MAX_LENGTH, "%s", buff + o);
                o += bytes;
                break;
            case UNSUB_CANDNODES:
                bytes = snprintf(lr->values->sock_addr, LOCAL_ADDR_MAX_LENGTH, "%s", buff + o);
                o += bytes;
                break;
            default:
                log_event(LOG_DEBUG, "Invalid local request received");
                break;
        }
        
        if(o != buff_size) printf("WARN: buff_size: %d and offset: %d doesn't match!\n", buff_size, o);

        } else {
            log_event(LOG_DEBUG, "Tried to unpack a NULL-buffer received on local socket.");
        }
    return lr;
}

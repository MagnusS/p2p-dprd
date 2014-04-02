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
 * Author: Halvdan Hoem Grelland, Jostein Aardal, Magnus Skjegstad
 */

/*
 * io.h
 *
 * Function prototypes and data types related to Local- and Network-IO.
 *
 */

#ifndef INCLUDE_IO_H_
#define INCLUDE_IO_H_

/* Includes */
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

/*
 * We are using UDP, and therefore always need to allocate memory for the receive-buffer
 * before knowing the actual packet-size. This number could be tuned for lowering memory
 * usage. The default value is fairly arbitrary, but set so that we can receive a single
 * packet of approximately 1000 Nodes.
 * Note that UDP doesn't support packets of size > 2^16 (~65k).
 */
#define MAX_PAYLOAD_BYTESIZE 32768 /* Assuming max 1000 nodes * ~32 bytes/node */

/* Max string-length of local socket address (path) */
#define LOCAL_ADDR_MAX_LENGTH 512

/* Identifies local requests to the program */
typedef enum LOCAL_REQ_TYPE {
	SET_POSITION,				/* 0x0 */
	SET_COORDINATION_RANGE,		/* 0x1 */
	SET_POS_AND_RANGE,			/* 0x2 */
	SUB_CANDNODES,				/* 0x3 */
	UNSUB_CANDNODES				/* 0x4 */
} LOCAL_REQ_TYPE;

/* Structure wrapping the set of values we can receive */
typedef struct request_values {
	double 		lat;
	double 		lon;
	uint16_t 	coord_range;
	char		sock_addr[LOCAL_ADDR_MAX_LENGTH];
} request_values;

/*
 * Structure used to internally represent a local request.
 * Contains field to identify type of request, as well
 * as a set of values. */
typedef struct LocalRequest {
	LOCAL_REQ_TYPE type;
	request_values* values;
} LocalRequest;

#include "configuration.h"
#include "protocol.h"
#include "serialize.h"
#include "subscribe.h"

/*
 * Initialise values used in the main-loop call to select()
 * 	Arguments:
 * 		initSet 	- Pointer to an empty set of file descriptors
 *		varTime		- Pointer to an empty timeval
 *		initTime	- Pointer to an empty timeval (used to init timer)
 *		networkSock - FD to the network (listening UDP) socket
 *		localSock	- FG to the local (listening DGRAM) socket
 *		TIMEOUT		- Base timeout value - stored in initTime
 *
 *	Returns:
 *		int socket	- The current highest socket FD
 */
int IO_selectVars_init(fd_set* initSet, struct timeval* varTime, struct timeval* initTime, int networkSock, int localSock, time_t TIMEOUT);

/*
 *  Construct a UDP socket to send data
 * 	Arguments:
 *		s		- Pointer to empty sockaddr_in
 *		ipAddr	- Network-encoded IPv4 address of receiver
 *		port	- Port of receiver
 *
 *	Returns:
 *		int sock - FD to a new socket, primed to send UDP packages to receiver
 */
int IO_sendSocket_init(struct sockaddr_in* s, uint32_t ipAddr, uint16_t port);

/*
 *  Construct and bind a UDP socket to receive data
 * 	Arguments:
 * 		port	- port to bind to
 * 	Returns:
 *		int socket - FD to primed listening socket
 */
int IO_recvSocket_init(uint16_t port);

/*
 * Send byte-buffer
 * 	Arguments:
 * 		buffer		- pointer to buffer to send
 * 		buff_size 	- byte-size of buffer
 * 		ip			- Network encoded IP to send to
 * 		port		- Receiver port
 *
 * 	Returns:
 * 		int sendBytes - Number of bytes sent to receiver
 */
int IO_sendBytes(unsigned char* buffer, uint16_t buff_size, uint32_t ip, uint16_t port);

/*
 * Construct and fill a new LocalRequest object
 * 	Arguments:
 * 		req_type	- Request identifier
 * 		lat / lon	- latitude / longitude
 * 		coord_range	- coordination range
 * 		sock_addr	- Pointer to bytearray containing a path (AF_UNIX address)
 *
 * 	Returns:
 * 		Pointer to allocated and initialised LocalRequest object
 */
LocalRequest* LocalRequest_new(LOCAL_REQ_TYPE req_type, double lat, double lon, uint16_t coord_range, char sock_addr[LOCAL_ADDR_MAX_LENGTH]);

/*
 *  Free a malloc'ed LocalRequest object from memory.
 * 	Arguments:
 * 		lr	- Pointer to object to destroy
 *
 * 	Returns:
 * 		void
 */
void LocalRequest_destroy(LocalRequest* lr);

/*
 * Pushes list of nodes to all subscribers. The first node in the sent result is always our own node description.
 *	
 *	Arguments:
 *		cn		- NodeCollection of candidate nodes
 *		subs	- List of subscribers to send to (local sockets)
 *      ownNode - Pointer to Node struct with own node description
 * 
 *	Returns:
 *		int bytes - Total amount of bytes sent
 */
int LocalIO_sendCandidateNodes(NodeCollection* cn, SubscriberList* subs, Node* ownNode);

/*
 * Set up and bind an AF_UNIX datagram socket on local path
 * 	Arguments:
 *		unix_sock_name	- Pointer to char-array path
 *
 *	Returns:
 *		int - FD of new socket
 */
int LocalIO_localSocket_init(char* unix_sock_name);

/*
 * Handle a locally received request
 * 	Arguments:
 * 		lr		- Pointer to LocalRequest object to handle
 * 		config 	- Pointer to Config to update
 * 		subs	- Pointer to list of subsribers
 *
 * 	Returns:
 * 		int - 1 on success, 0 on undefined request type / failure
 */
int LocalIO_handleRequest(LocalRequest* lr, Config* config, SubscriberList* subs);

#endif /* INCLUDE_IO_H_ */

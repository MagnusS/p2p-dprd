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
 * protocol.h
 *
 * Declaration of functions implementing protocol-defined routines
 *
 *      Author: Halvdan Hoem Grelland and Jostein Aardal
 */

#ifndef INCLUDE_PROTOCOL_H_
#define INCLUDE_PROTOCOL_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <float.h>

#include "node.h"
#include "io.h"
#include "utilities.h"
#include "configuration.h"

/*
 * Update NodeCollection rn using received NodeCollection nc
 *
 * 	Arguments:
 * 		nc	- Pointer to the received NodeCollection
 * 		rn	- Pointer to the random NodeCollection
 * 	Returns:
 * 		void
 *
 * 1. Append nc to rn
 * 2. Null out duplicate Nodes from rn (using timestamp as priority)
 * 3. Sort rn by timestamp
 * 4. Delete Nodes in rn with index > N (size of rn = 2*N)
 */
void Protocol_updateRandomNodes(NodeCollection* nc, NodeCollection* rn);

/* Update NodeCollection importantNodes (in) using received NodeCollection nc
 * 	Arguments:
 * 		nc	- Pointer to NodeCollection used to update
 * 		rn	- Pointer to randomNodes NodeCollection to update
 *
 * 	Returns:
 * 		void
 *
 * 1. Append nc to rn
 * 2. Null out duplicate Nodes from in (using timestamp as priority)
 * 3. Calculate utility of nodes in NodeCollection in
 * 4. Sort in by utility (high to low)
 * 5. Grow if necessary
 * 6. Delete Nodes in in with index > M-K (size of in = M+K)
 */
void Protocol_updateImportantNodes(NodeCollection* nc, NodeCollection* in);

/*
 * Protocol subroutine - receive, unpack and handle data from peer
 * 	Arguments:
 * 		sock			- FD of socket to receive from
 * 		importantNodes 	- Pointer to NodeCollection of important nodes
 * 		randomNodes		- Pointer to NodeCollection of random nodes
 *
 * 	Returns:
 * 		void
 */
void Protocol_receiveFromPeer(int sock, NodeCollection* importantNodes, NodeCollection* randomNodes);

/*
 * Protocol subroutine - local timeout triggered
 * Send randomNodes to a random Node, send importantNodes to an important Node
 * 	Arguments:
 * 		rn	- Pointer to NodeCollection of random Nodes
 * 		in	- Pointer to NodeCollection of important Nodes
 */
void Protocol_timeout(NodeCollection* rn, NodeCollection* in);

/*
 * Bootstrap the protocol on startup - contact the origin (first) peer
 * 	Arguments:
 * 		originPeerIP	- IP of peer to contant
 * 		originPeerPort	- Port of peer to contact
 * 	Returns:
 * 		void
 *
 * 	Sends a NodeCollection containing only ourself to peer
 */
void Protocol_bootstrap(uint32_t originPeerIP, uint16_t originPeerPort);

/*
 * Send a NodeCollection of random nodes to peer
 *	Arguments:
 *		rn			- Pointer to NodeCollection randomNodes
 *		type		- Type of correspondance (enum payloadType)
 *		peerNode	- Pointer to Node to send to
 *
 * Main steps taken:
 * 	1. Allocates new NodeCollection nc of size rn.size + 1
 * 	2. Creates Node ownNode
 * 	3. Prepends ownNode to top of nc
 * 	4. Copies contents of rn into nc
 *	5. Sends nc to peerNode
 */
void Protocol_sendRandomNodes(NodeCollection* rn, payloadType type, Node* peerNode);

/*
 * Send a NodeCollection of important nodes to peer
 * 	Arguments:
 * 		in			- Pointer to NodeCollection of important nodes
 * 		type		- Type of correspondance (enum payloadType)
 * 		peerNode	- Pointer to Node to send to
 */
void Protocol_sendImportantNodes(NodeCollection* in, payloadType type, Node* peerNode);

#endif /* INCLUDE_PROTOCOL_H_ */

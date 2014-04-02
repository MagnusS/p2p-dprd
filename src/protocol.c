
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
 * Authors: Halvdan Hoem Grelland, Jostein Aardal, Magnus Skjegstad
 */

/*
 * protocol.c
 *
 *	Implementation of functions defined in protocol.h
 *	Refer to header file for documentation.
 *
 */

#include "protocol.h"

void Protocol_timeout(NodeCollection* rn, NodeCollection* in){
	/* 1. remove old nodes from randomNodes
	 * 2. remove old nodes from importantNodes
	 * 3. sort ImportantNodes depending on utility
	 * 4. get random peer Node from randomNodes
	 * 5. send randomNodes to randomPeerNode -> Type = RND_REQ
	 * 6. get random peer Node from importantNodes
	 * 7. send importantNodes to importantPeerNode -> TYPE = IMP_REQ
	 */

	/* Remove old nodes from randomNodes and importantNodes.
	 * sort importantNodes to return it to its origanl state */
	int removed_nodes = 0;
	removed_nodes = NodeCollection_removeExpiredNodes(rn, CONFIG->PROTO_nodeMaxAge);
	if(removed_nodes > 0){
		log_event(LOG_DEBUG, "%d nodes in randomNodes met the age limit and were discarded", removed_nodes);
	}
	removed_nodes = NodeCollection_removeExpiredNodes(in, CONFIG->PROTO_nodeMaxAge);

	if(removed_nodes > 0){
		log_event(LOG_DEBUG, "%d nodes in importantNodes met the age limit and were discarded", removed_nodes);
	}

	NodeCollection_sortByUtility(in);

	/* Get a random peerNode and send randomNodes to this peer */
	Node* peerNode = Node_getRandomPeerNode(rn);
	if(peerNode){
		Protocol_sendRandomNodes(rn, RND_REQ, peerNode);
		log_event(LOG_DEBUG, "Sent randomNodes to peer %d\n", peerNode->nodeID);
	} else {
		/* There are zero nodes in randomNodesList -> run kickstart-subroutine */
		Protocol_bootstrap(CONFIG->NETWORK_originPeerIP, CONFIG->NETWORK_originPeerPort);
		log_event(LOG_DEBUG, "Sent ownNode to originPeer on port %d\n", CONFIG->NETWORK_originPeerPort);
	}

	/* Get a random peerNode and send importantNodes to this peer */
	peerNode = Node_getRandomImportantNode(in);
	if(peerNode){
		Protocol_sendImportantNodes(in, IMP_REQ, peerNode);
		log_event(LOG_DEBUG, "Sent importantNodes to peer %d\n", peerNode->nodeID);
	}
}
/* Run-once function to send own Node object to the origin peer.
 * Only used on startup of program.
 */
void Protocol_bootstrap(uint32_t originPeerIP, uint16_t originPeerPort){
	/* Create own node */
	Node* ownNode = Node_createOwnNode();

	/* Create peer node */
	Node* peerNode = Node_new(0, 0, 0, 0, originPeerIP, originPeerPort, 0, 0, time(NULL));

	/*
	 * 1. Create empty randomNodes with ownNode on top.
	 * 2. Send to peerNode
	 */
	NodeCollection* nc = NodeCollection_new(P2PDPRD_VERSION_ID, RND_REQ, 1);
	memcpy(&nc->nodes[0], ownNode, sizeof(Node));
	nc->nodeCount = 1;

	NodeCollection_sendToPeer(nc, peerNode);

	Node_destroy(peerNode);
	Node_destroy(ownNode);
	NodeCollection_destroy(nc);
}

void Protocol_receiveFromPeer(int sock, NodeCollection* importantNodes, NodeCollection* randomNodes){
	/* First, we need to receive the data on the socket
	 * To handle this event, we take the following sequence of actions:
	 * 1. Run subroutine to determine size of UDP payload
	 * 2. Receive payload, store to byte buffer
	 * 3. Deserialize payload, convert to NodeCollection
	 * 4. Examine received NodeCollection, handle appropriately
	 */

	/* Control variables */
	int numNodes;													/* Number of nodes in received NodeCollection */
	int payloadSize;												/* Bytesize of received payload */
	struct sockaddr_in from_addr;									/* Packet source address */
	unsigned int from_addr_len = sizeof(struct sockaddr_in);		/* Variable to store length of packet source address */

	/* Set up the byte buffer */
	unsigned char* buffer = malloc(MAX_PAYLOAD_BYTESIZE);

	/* Read data from socket. Returns size of payload in bytes to payloadSize */
	payloadSize = recvfrom(	sock,							/* Socket to read from */
							buffer,							/* Buffer to read to */
							MAX_PAYLOAD_BYTESIZE ,			/* Max size of buffer. If exceeded, data is cut short */
							0,								/* No flags are set */
							(struct sockaddr *)&from_addr,	/* Sender sockaddr */
							&from_addr_len					/* Size of sender addr field */
	);

	/* Unpack the NodeCollection object from the byte buffer. Number of nodes is returned to numNodes */
	NodeCollection* nc = NodeCollection_unpack(buffer, payloadSize, &numNodes);

	/* We have received a NodeCollection from a peer */
	if(NodeCollection_isValid(nc)){
		/* Print nodeCollection for debugging: */
		NodeCollection_print(nc);
		/* Check type of NodeCollection. Take appropriate action */
		if	(nc->payloadType == RND_NOREQ){
			log_event(LOG_DEBUG, "Received NodeCollection of type RND_NOREQ from %d", nc->nodes[0].nodeID);

			Protocol_updateRandomNodes(nc, randomNodes);
			log_event(LOG_DEBUG, "Updated randomNodes using NodeCollection from peer %d", nc->nodes[0].nodeID);

			Protocol_updateImportantNodes(randomNodes, importantNodes);
			log_event(LOG_DEBUG, "Updated importantNodes\n");

		} else if (nc->payloadType == RND_REQ){
			log_event(LOG_DEBUG, "Received NodeCollection of type RND_REQ from %d", nc->nodes[0].nodeID);
			Protocol_sendRandomNodes(randomNodes, RND_NOREQ, &nc->nodes[0]);

			log_event(LOG_DEBUG, "Sent randomNodes to peer %d", nc->nodes[0].nodeID);

			Protocol_updateRandomNodes(nc, randomNodes);
			log_event(LOG_DEBUG, "Updated randomNodes using NodeCollection from peer %d", nc->nodes[0].nodeID);

			Protocol_updateImportantNodes(randomNodes, importantNodes);
			log_event(LOG_DEBUG, "Updated importantNodes\n");

		} else if (nc->payloadType == IMP_NOREQ){
			log_event(LOG_DEBUG, "Received NodeCollection of type IMP_NOREQ from %d - port %d\n", nc->nodes[0].nodeID, nc->nodes[0].port);
			Protocol_updateImportantNodes(nc, importantNodes);

			log_event(LOG_DEBUG, "Updated importantNodes using NodeCollection from peer %d - port: %d\n", nc->nodes[0].nodeID, nc->nodes[0].port);

		} else if (nc->payloadType == IMP_REQ){
			log_event(LOG_DEBUG, "Received NodeCollection of type IMP_REQ from %d", nc->nodes[0].nodeID);

			Protocol_sendImportantNodes(importantNodes, IMP_NOREQ, &nc->nodes[0]);
			log_event(LOG_DEBUG, "Sent importantNodes to peer %d", nc->nodes[0].nodeID);

			Protocol_updateImportantNodes(nc, importantNodes);
			log_event(LOG_DEBUG, "Updated importantNodes using NodeCollection from peer %d - port: %d\n", nc->nodes[0].nodeID, nc->nodes[0].port);
		} else {
			log_error(NOTICE, errno, "NodeCollection contained corrupted payload");
		}
	} else {
		/* Received a NodeCollection of non-valid type. Something is wrong, but it is not critical. Discard and log. */
		log_event(LOG_DEBUG, "Received a non-valid NodeCollection from peer");
		//log_event(LOG_DEBUG, "Received a non-valid NodeCollection from peer %d", nc->nodes[0].nodeID);
	}
	/* Cleaning */
	free(buffer);
	NodeCollection_destroy(nc);

}
void Protocol_updateRandomNodes(NodeCollection* nc, NodeCollection* rn){

	/* Update NodeCollection randomNodes (rn) using received NodeCollection nc
	 * 1. Append nc to rn
	 * 2. Null out duplicate Nodes from rn (using timestamp as priority)
	 * 3. Sort rn by timestamp
	 * 4. Delete Nodes in rn with index > N (size of rn = 2*N)
	 */

	NodeCollection_append(rn, nc, CONFIG->CLIENT_id); // append, but ignore own ID
	NodeCollection_removeDuplicateNodes(rn);
	NodeCollection_sortByTimeStamp(rn);
	NodeCollection_removeExcessNodes(rn, rn->maxNodeCount / 2);
}
void Protocol_updateImportantNodes(NodeCollection* nc, NodeCollection* in){
	/* Update NodeCollection importantNodes (in) using received NodeCollection nc
	 * 1. Append nc to rn
	 * 2. Null out duplicate Nodes from in (using timestamp as priority)
	 * 3. Calculate utility of nodes in NodeCollection in
	 * 4. Sort in by utility (high to low)
	 * 5. Grow if necessary
	 * 6. Delete Nodes in in with index > M-K (size of in = M+K)
	 * 7. Deliver updated list to system?
	 */

	/* Create updated Node-object of ourself*/
	Node* ownNode = Node_new(CONFIG->CLIENT_id,			/* Only used for internal calculation */
							 CONFIG->CLIENT_lat,		/* -> No need for networking vars */
							 CONFIG->CLIENT_lon,
							 CONFIG->CLIENT_coordRange,
							 0,
							 0,
							 0,
							 0,
							 time(NULL)
	);

	NodeCollection_calculateUtility(nc, ownNode);
	NodeCollection_append(in, nc, CONFIG->CLIENT_id); // append, but ignore own ID
	NodeCollection_removeDuplicateNodes(in);
	NodeCollection_sortByUtility(in);

	/* Check to see if growing is necessary */
	int candidate_amount = NodeCollection_countCandidateNodes(in);
	if(candidate_amount > (in->maxNodeCount - CONFIG->PROTO_K))
		NodeCollection_grow(in, CONFIG->PROTO_K);

	if(in->nodeCount > (in->maxNodeCount - CONFIG->PROTO_K)){
		NodeCollection_removeExcessNodes(in, (in->maxNodeCount - CONFIG->PROTO_K));
	}

	/* Print a message */
	log_event(LOG_DEBUG, "Counted %d candidate nodes from %d important nodes", candidate_amount, in->nodeCount);

	/* Clean */
	Node_destroy(ownNode);
}

/* Sends a NodeCollection of random nodes rn to Node peerNode
 *
 * Main steps taken:
 * 	1. Allocates new NodeCollection nc of size rn.size + 1
 * 	2. Creates Node ownNode
 * 	3. Prepends ownNode to top of nc
 * 	4. Copies contents of rn into nc
 *	5. Sends nc to peerNode
 */
void Protocol_sendRandomNodes(NodeCollection* rn, payloadType type, Node* peerNode){
   /* Create new NodeCollection */
   NodeCollection* nc = NodeCollection_new(P2PDPRD_VERSION_ID, type, rn->nodeCount + 1);

   /* Create own Node object and stuff on top of nc*/
   Node* ownNode = Node_createOwnNode();
   memcpy(&nc->nodes[0], ownNode, sizeof(Node));

   /* Copy Nodes from originalNodeCollection. Nodes[n-1] goes to Nodes[n], leaving the node on the top empty */
   int j;
   if(rn->nodeCount > 1){
	   /* There are more Nodes to carry over than just our own Node object*/
	   for(j = 1; j < rn->nodeCount ; j++){
		   memcpy(&nc->nodes[j], &rn->nodes[j-1], sizeof(Node));
	   }
	   nc->nodeCount = rn->nodeCount;
   } else {
	   /* There is only one Node - ourself */
	   nc->nodeCount = 1;
   }

   NodeCollection_sendToPeer(nc, peerNode);

   /* Cleaning */
   NodeCollection_destroy(nc);
   Node_destroy(ownNode);
}

/* Sends a NodeCollection of important nodes in to Node peerNode */
void Protocol_sendImportantNodes(NodeCollection* in, payloadType type, Node* peerNode){

	NodeCollection* tmp_nc = NodeCollection_new(P2PDPRD_VERSION_ID, type, in->nodeCount);

	int l;
	for(l = 0 ; l < in->nodeCount; l++){
		memcpy(&tmp_nc->nodes[l], &in->nodes[l], sizeof(Node));
	}
	/* Update tmp_nc->nodeCount */
	tmp_nc->nodeCount = in->nodeCount;

	/* If the list is too large - sort and remove excess nodes based on utility*/
	if(tmp_nc->nodeCount > CONFIG->PROTO_K){
		NodeCollection_calculateUtility(tmp_nc, peerNode);			/* Calculate utility of nodes */
		NodeCollection_sortByUtility(tmp_nc);						/* Sort by newly calculated utility */
		NodeCollection_removeExcessNodes(tmp_nc, CONFIG->PROTO_K);	/* Remove all excess nodes */
	}

	/* Create new nc to send to peer */
	NodeCollection* nc = NodeCollection_new(P2PDPRD_VERSION_ID, type, tmp_nc->nodeCount + 1);

	/* Copy Nodes from originalNodeCollection. Nodes[n-1] goes to Nodes[n], leaving the node on the top empty */
	int j;
	for(j = 1; j < tmp_nc->nodeCount + 1 ; j++){
		memcpy(&nc->nodes[j], &tmp_nc->nodes[j - 1], sizeof(Node));
	}
	/* Create own Node */
	Node* ownNode = Node_createOwnNode();

	/* Copy and add own Node object to top */
	memcpy(&nc->nodes[0], ownNode, sizeof(Node));

	/* Update nodeCount */
	nc->nodeCount = tmp_nc->nodeCount + 1;

	/* Dispatch to peer */
	NodeCollection_sendToPeer(nc, peerNode);

	/* Cleaning */
	NodeCollection_destroy(nc);
	NodeCollection_destroy(tmp_nc);
	Node_destroy(ownNode);
}

/*
 * Copyright (c) 2012-2014, Magnus Skjegstad / Forsvarets Forskningsinstitutt (FFI)
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
 * node.h
 *
 * Definition of datatypes for internal representation of peer data.
 * Declaration of functions relating to Node and NodeCollection datatypes.
 *
 *      Author: Halvdan Hoem Grelland, Jostein Aardal, Magnus Skjegstad
 */

#ifndef INCLUDE_NODE_H_
#define INCLUDE_NODE_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>


/*
 * Enumeration of NodeCollection type-identifiers.
 * RND_NOREQ 	- Random nodes, no request
 * RND_REQ		- Random nodes, requesting reply
 * IMP_NOREQ	- Important nodes, no request
 * IMP_REQ		- Important nodes, requesting reply
 * INTERNAL		- Internally created / used NodeCollection
 */
typedef enum payloadType {RND_NOREQ, RND_REQ, IMP_NOREQ, IMP_REQ, INTERNAL} payloadType;

/* Base Node data structure - holds all data associated with a single peer */
typedef struct Node {
	uint32_t 		nodeID;		/* Node ID - unsigned 32-bit integer */
	double 			lat;		/* Latitudal coordinate of node position */
	double 			lon;		/* Longitudal coordinate of node position */
	uint16_t 		coordRange;	/* Node coordination range in metres */
	uint32_t 		ipAddr;		/* Node IP-address, network encoded */
	uint16_t		port;		/* Port node listens on */
	uint32_t 		timeStamp;	/* Time since creation of Node object */
	uint32_t		radac_ip;	/* IP of associated RADAC instance */
	uint16_t		radac_port;	/* Port of associated RADAC instance */
	double			utility;	/* Utility of Node */
} Node;

/* Structure holding a collection of 0 or more Node objects. */
typedef struct NodeCollection {
	uint16_t 		versionID;			/* Identifies program/protocol version which generated the NodeCollection. */
	payloadType		payloadType;		/* Identifies type of NodeCollection (contents/context) */
	uint16_t		nodeCount;			/* Actual amount of Nodes in collection*/
	uint16_t		maxNodeCount;		/* Max amount of Nodes allocated in memory for collection */
	Node*			nodes;
} NodeCollection;

#include "configuration.h"
#include "serialize.h"

/* Function prototypes */

/*
 * Construct a new instance of Node
 * 	Arguments:
 * 		nodeId		- ID of new Node
 * 		lat/lon		- Geo-position
 * 		coordRange	- Coordination range
 * 		ipAddr		- IP
 * 		port		- Port
 * 		timeStamp	- time of object creation
 * 	Retuns:
 * 		Pointer to new Node
 *
 * 	Memory is allocated on heap, use Node_destroy() to free memory
 */
Node* Node_new
(uint32_t nodeID, double lat, double lon, uint16_t coordRange, uint32_t ipAddr, uint16_t port, uint32_t radac_ip, uint16_t radac_port, uint32_t timeStamp);

/*
 * Destroy an instance of Node
 * 	Arguments:
 * 		n	- Pointer to Node
 * 	Returns:
 * 		void
 *
 * 	Frees memory allocated to Node
 */
void Node_destroy(Node* n);

/* 
 * Create new Node with content from CONFIG 
 * Arguments:
 *      none
 * Returns:
 *      Pointer to new Node struct
 */
Node* Node_createOwnNode();

/*
 * Invalidate a Node object
 * 	Arguments:
 * 		n	- Pointer to Node
 * 	Returns:
 * 		void
 *
 * 	Implementation only nulls the NodeID. Internally, a Node with ID == 0
 * 	is considered to be invalid and will eventually be discarded.
 *
 */
void Node_nullOutNode(Node* n);

/*
 * Swaps pointers of two Nodes
 * 	Arguments:
 * 		n0 / n1	- Pointers to instances of Node
 * 	Returns;
 * 		void
 *
 * 	Swapping pointers of Nodes is useful in certain sorting scanarios
 * 	operating on lists/arrays of Node instances.
 */
void Node_swapPointers(Node** n0, Node** n1);

/*
 * Construct and allocate a new NodeCollection object
 * 	Arguments:
 * 		versionID	- Protocol/program versionID
 * 		type		- Type of payload (enum payloadType)
 * 		nodeCount	- Amount of Nodes to allocate in memory for NodeCollection
 *
 * 	Returns:
 * 		Pointer to new NodeCollection
 */
NodeCollection* NodeCollection_new(uint16_t versionID, payloadType type, uint16_t nodeCount);

/*
 * Destroy (free) a NodeCollection
 * 	Arguments:
 * 		nc	- Pointer to a NodeCollection
 * 	Returns:
 * 		void
 */
void NodeCollection_destroy(NodeCollection* nc);

/*
 * Grow the allocated memory space of a NodeCollection
 * 	Arguments:
 * 		nc				- Pointer to NodeCollection to grow
 * 		num_new_nodes	- Amount of Nodes to grow allocated memory with
 * 	Returns:
 * 		void
 */
void NodeCollection_grow(NodeCollection* nc, unsigned int num_new_nodes);

/*
 * Check validity of a NodeCollection
 * 	Arguments:
 * 		nc	- Pointer to a NodeCollection
 * 	Returns:
 * 		1 if deemed valid, 0 else
 */
int NodeCollection_isValid(const NodeCollection* nc);

/*
 * Check validity of a payload-identifier
 * 	Arguments:
 * 		type - payload-identifier to check
 * 	Returns:
 * 		1 if valid, 0 if not valid
 */
int NodeCollection_typeIsValid(payloadType type);

/*
 * Print a formatted representation of NodeCollection
 * 	Arguments:
 * 		nc	- Pointer to NodeCollection to print
 * 	Returns:
 * 		void
 *
 * 	Outputs to STDOUT
 */
void NodeCollection_print(NodeCollection* nc);

/*
 *	Print a formatted representation of a NodeCollection to a file
 *		Arguments:
 *			file_path	- Pointer to char-array with filepath
 *			title		- Pointer to char-array with title of NodeCollection (e.g. "ImportantNodes")
 *			nc			- Pointer to NodeCollection to print
 *		Returns:
 *			1 on success, 0 else
 */
int NodeCollection_printToFile(char* file_path, char* title, NodeCollection* nc);

/*
 * Sort a NodeCollection by Node time stamp - high to low
 *	Arguments:
 *		nc	- Pointer to NodeCollection to sort
 *	returns:
 *		void
 */
void NodeCollection_sortByTimeStamp(NodeCollection* nc);

/*
 * Sort a NodeCollection by utility - high to low
 * 	Arguments:
 * 		nc	- Pointer to NodeCollection to sort
 * 	Returns:
 * 		void
 */
void NodeCollection_sortByUtility(NodeCollection* nc);

/*
 * Sort a NodeCollection by NodeID
 * 	Arguments:
 * 		nc	- Pointer to NodeCollection to sort
 * 	Returns:
 * 		void
 */
void NodeCollection_sortByNodeID(NodeCollection* nc);

/*
 * Append a NodeCollection to another
 * 	Arguments:
 * 		a	- Pointer to NodeCollection to append to
 * 		b	- Pointer to NodeCollection to append to a
 * 	    ignoreNodeId - NodeID to ignore from b. Only used if > 0.
 * 	Returns:
 * 		void
 */
void NodeCollection_append(NodeCollection* a, NodeCollection* b, uint32_t ignoreNodeId);

/*
 * Remove duplicate Nodes from a NodeCollection
 * 	Arguments:
 * 		nc	- Pointer to NodeCollection to remove duplicates from
 * 	Returns:
 * 		void
 *
 * 	Removes all duplcaite Nodes (matching IDs) - keeps most recent Node
 */
void NodeCollection_removeDuplicateNodes(NodeCollection* nc);

/*
 * Removes excess Nodes from a NodeCollection
 * 	Arguments:
 * 		nc	  -	Pointer to a NodeCollection
 * 		floor -	Floor / max size of NodeCollection
 * 	Returns:
 * 		int - amount of removed nodes
 *
 * 	All Nodes with index > floor will be nulled out (ID = 0)
 */
int NodeCollection_removeExcessNodes(NodeCollection* nc, unsigned int floor);

/*
 * Removes any expired Nodes from a NodeCollection
 *	Arguments:
 *		nc			- Pointer to NodeCollection to remove from
 *		expire_time	- Age-limit of expired Nodes (EPOCH time)
 *	Returns:
 *		int - amount of removed expired Nodes
 */
int NodeCollection_removeExpiredNodes(NodeCollection* nc, unsigned int expire_time);

/*
 * Get a NodeCollection of candidate nodes
 * 	Arguments:
 * 		nc	- Pointer to source NodeCollection
 * 	Returns:
 * 		Pointer to NodeCollection of candidate nodes
 * 	Note: The NodeCollection is malloced, use NodeCollection_destroy() to free memory properly
 */
NodeCollection* NodeCollection_getCandidateNodes(NodeCollection* nc);

/*
 * Get a random important node from a NodeCollection
 * 	Arguments:
 * 		nc	- Pointer to NodeCollection
 * 	Returns:
 * 		Node* - Pointer to a Node
 */
Node* Node_getRandomImportantNode(NodeCollection* nc);

/*
 * Choose and get a random Node from a NodeCollection
 * 	Arguments:
 * 		nc	- Pointer to NodeCollection
 * 	Returns:
 * 		Node* - Pointer to randomly chosen Node
 */
Node* Node_getRandomPeerNode(NodeCollection* nc);

/*
 * Calculate the utility-value of a node
 * 	Arguments:
 * 		a	- Pointer to node a
 * 		b	- Pointer to node b
 * 	Returns:
 * 		double	- Utility of node b with respect to node a
 */
double Node_utility(Node* a, Node* b);

/*
 * Counts amount of candidate nodes in a NodeCollection
 * 	Arguments:
 * 		in	- Pointer to a NodeCollection
 * 	Returns:
 * 		Amount of candidate nodes found in 'in'
 */
int NodeCollection_countCandidateNodes(NodeCollection* in);

/*
 * Calculates utility of all Nodes in a NodeCollection
 * 	Arguments:
 * 		nc	- Pointer to NodeCollection
 * 		n	- Pointer to Node to calculate with respect to
 * 	Returns:
 * 		void
 */
void NodeCollection_calculateUtility(NodeCollection* nc, Node* n);

/*
 * Send a NodeCollection to a peer Node
 * 	Arguments:
 * 		nc			- Pointer to NodeCollection to send
 *		peerNode	- Pointer to Node object of peer to send to
 *	Returns:
 *		int - Amount of sent bytes
 */
int NodeCollection_sendToPeer(NodeCollection* nc, Node* peerNode);

#endif /* INCLUDE_NODE_H_ */

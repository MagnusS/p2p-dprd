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
 * node.c
 *
 *	Implementation of functions defined in node.h
 *	Refer to header file for documentation.
 *
 */
#include "node.h"

/* Utility quicksort subroutine */
int comp_sort_utility_h2l(const Node* a, const Node* b);
/* timestamp quicksort subroutine */
int comp_sort_timestamp_h2l(const Node* a, const Node* b);
/* NodeID quicksort subroutine */
int comp_sort_id(const Node* a, const Node* b);

/* Create an empty NodeCollection of allocated size maxNodeCount
 * Returns pointer to allocated NodeCollection.
 * NOTE: Does not check validity of data.
 */
NodeCollection* NodeCollection_new(uint16_t versionID, payloadType type, uint16_t maxNodeCount){
	NodeCollection* nc = malloc(sizeof(NodeCollection));

	nc->versionID = versionID;
	nc->payloadType = type;
	nc->maxNodeCount = maxNodeCount;
	nc->nodes = malloc(sizeof(Node) * nc->maxNodeCount);
	nc->nodeCount = 0;

	return nc;
}

void NodeCollection_destroy(NodeCollection* nc){
	if(nc){
		free(nc->nodes);
		free(nc);
	}
}

/* Grow amount of Nodes in a NodeCollection with grow_amount nodes */
void NodeCollection_grow(NodeCollection* nc, unsigned int grow_amount){
	if(nc->maxNodeCount + grow_amount <= P2PDPRD_NODES_MAX_SIZE){
		nc->maxNodeCount = nc->maxNodeCount + grow_amount;
		nc->nodes = realloc(nc->nodes, (nc->maxNodeCount) * sizeof(Node));

		log_event(LOG_DEBUG, "A NodeCollection has been grown by %d nodes", grow_amount);

	} else {
		log_event(LOG_ERROR, "List of candidate nodes reached the max limit of %d nodes", P2PDPRD_NODES_MAX_SIZE);
	}
}

/* Heap-allocates a new Node object. Note that memory is malloced,
 * and therefore must be freed using Node_destroy().
 *
 * Returns: Pointer to a Node
 */
Node* Node_new
(uint32_t nodeID, double lat, double lon, uint16_t coordRange, uint32_t ipAddr, uint16_t port, uint32_t radac_ip, uint16_t radac_port, uint32_t timeStamp){
	Node* n = malloc(sizeof(Node));

	n->nodeID = nodeID;
	n->lat = lat;
	n->lon = lon;
	n->coordRange = coordRange;
	n->ipAddr = ipAddr;
	n->port = port;
	n->radac_ip = radac_ip;
	n->radac_port = radac_port;
	n->timeStamp = timeStamp;
	/* Also, zero-initialize utility field*/
	n->utility = 0;

	return n;
}

/* Create new Node with content from CONFIG */
Node* Node_createOwnNode() {
    return Node_new(CONFIG->CLIENT_id,
                             CONFIG->CLIENT_lat,
                             CONFIG->CLIENT_lon,
                             CONFIG->CLIENT_coordRange,
                             CONFIG->NETWORK_ownIP,
                             CONFIG->NETWORK_port,
                             CONFIG->RADAC_ip,
                             CONFIG->RADAC_port,
                             time(NULL));
}

/* Frees memory of a Node object */
void Node_destroy(Node* n){
	free(n);
}

/* Nulls out a Node */
void Node_nullOutNode(Node* n){
	/* We only check NodeID, thus we only need to set the ID to zero */
	n->nodeID = 0;
}

/* Swap n0 and n1, effectively swapping the placement
   of the Nodes pointed to by n0 and n1*/
void Node_swapPointers(Node** n0, Node** n1){
	Node* tmp = *n0;
	*n0 = *n1;
	*n1 = tmp;
}

int NodeCollection_isValid(const NodeCollection* nc){
	/* NodeCollection is 'valid' if the nodeCount is non-negative and the nodes points to non-NULL.
	 * Also, the pointer nc itself needs to be non-NULL
	 */
	if(nc){		/* nc is not a NULL-pointer. Check values */
		if(nc->nodeCount > -1){
			return 1;	/* Values are OK. *nc is valid */
		} else {
			return 0;	/* Check failed, *nc is not valid */
		}
	} else {
		return 0;	/* nc is a NULL-pointer. Return false */
	}
}

int NodeCollection_typeIsValid(payloadType type){
	if (type == RND_NOREQ || type == RND_REQ || type == IMP_NOREQ || type == IMP_REQ || type == INTERNAL)
		return 1;
	else
		return 0;
}

void NodeCollection_print(NodeCollection* nc){
	if(NodeCollection_isValid(nc)){
		printf("NodeCollection:"
				"versionID = %d \t type = %d \t nodeCount = %d \t maxNodeCount = %d\n"
				,nc->versionID, nc->payloadType, nc->nodeCount, nc->maxNodeCount );

		int k;
		for (k = 0 ; k < nc->nodeCount ; k++){
			printf("%d \t - %d \t %f \t %f \t %d \t %d \t %d \t %d \t %d \t %d \n", k,
				nc->nodes[k].nodeID,
				nc->nodes[k].lat,
				nc->nodes[k].lon,
				nc->nodes[k].coordRange,
				nc->nodes[k].ipAddr,
				nc->nodes[k].port,
				nc->nodes[k].radac_ip,
				nc->nodes[k].radac_port,
				nc->nodes[k].timeStamp
				);
		}
	} else {
		printf("Tried to print invalid NodeCollection\n");
	}
}

int NodeCollection_printToFile(char* file_path, char* title, NodeCollection* nc){
	int success = 0;
	if(NodeCollection_isValid(nc)){
		FILE* file = fopen(file_path,"a+");

		if(file){
			char ts[64];
			writeTimestamp(ts);
			fprintf(file, "%s: %s\n",title, ts);
			int k;
			for (k = 0 ; k < nc->nodeCount ; k++){
				fprintf(file ,"%d \t - %d \t %f \t %f \t %d \t %d \t %d \t %d \t %d \t %d \n", k,
					nc->nodes[k].nodeID,
					nc->nodes[k].lat,
					nc->nodes[k].lon,
					nc->nodes[k].coordRange,
					nc->nodes[k].ipAddr,
					nc->nodes[k].port,
					nc->nodes[k].radac_ip,
					nc->nodes[k].radac_port,
					nc->nodes[k].timeStamp
				);
			}
			fprintf(file, "\n");
			success = 1;
		fclose(file);
		}
	} else {
		success = 0;
	}
	return success;
};

void NodeCollection_sortByUtility(NodeCollection* nc){
	qsort(nc->nodes, nc->nodeCount, sizeof(nc->nodes[0]), (void *)comp_sort_utility_h2l);
}
int comp_sort_utility_h2l(const Node* a, const Node* b){
	if(a->utility > b->utility)
		return -1;
	else if (a->utility < b->utility)
		return  1;
	else
		return 0;
}
void NodeCollection_sortByTimeStamp(NodeCollection* nc){
	qsort(nc->nodes, nc->nodeCount, sizeof(nc->nodes[0]), (void *)comp_sort_timestamp_h2l);
}

int comp_sort_timestamp_h2l(const Node* a, const Node* b){
	if(a->timeStamp > b->timeStamp)
		return -1;
	else if (a->timeStamp < b->timeStamp)
		return  1;
	else
		return 0;
}

void NodeCollection_sortByNodeID(NodeCollection* nc){
	qsort(nc->nodes, nc->nodeCount, sizeof(nc->nodes[0]), (void *)comp_sort_id);
}

int comp_sort_id(const Node* a, const Node* b){
	if(a->nodeID > b->nodeID)
		return -1;
	else if (a->nodeID < b->nodeID)
		return 1;
	else
		return 0;
}
/* Append NodeCollection b to NodeCollection a. Ignores ignoreNodeId if > 0 */
void NodeCollection_append(NodeCollection* a, NodeCollection* b, uint32_t ignoreNodeId){
	int b_pos = 0;

    while (a->nodeCount < a->maxNodeCount && b_pos < b->nodeCount) {

        if (ignoreNodeId == 0 || ignoreNodeId != b->nodes[b_pos].nodeID) {
            memcpy(&a->nodes[a->nodeCount], &b->nodes[b_pos], sizeof(b->nodes[b_pos]));
            a->nodeCount++; // move a ahead after insert
        }

        b_pos++; // always move b ahead
    }
}

/* Remove duplicate nodes from NodeCollection nc */
void NodeCollection_removeDuplicateNodes(NodeCollection* nc){
	/* 1. Sort by nodeID to ensure identical IDs are grouped together */
	NodeCollection_sortByNodeID(nc);
	int i, newNodeCount = nc->nodeCount;
	for (i = 0 ; i < nc->nodeCount - 1 ; i++){

		/* Compare with index + 1 element */
		// new and imporoved version
		if(nc->nodes[i].nodeID == nc->nodes[i + 1].nodeID){
			/* Found two neighbouring duplicates: (indexed from top to bottom) n0 and n1 */
			if(nc->nodes[i].timeStamp < nc->nodes[i + 1].timeStamp){
				/* n0 duplicate is the oldest. Null out n0 */
				Node_nullOutNode(&nc->nodes[i]);
				newNodeCount--;
			} else {
				/* n1 is the oldest (or they are of identical age).
					   Null out n1, then swap places of n1 and n0 */
				Node_nullOutNode(&nc->nodes[i + 1]);
				Node_swapPointers((Node**)&nc->nodes[i], (Node**)&nc->nodes[i+1]);
				newNodeCount--;
			}

		}

	}
	/* We need to ensure that null-Nodes are placed on the bottom of our
	 * NodeCollection. Therefore, we sort by ID again
	 */
	NodeCollection_sortByNodeID(nc);
	nc->nodeCount = newNodeCount;
}

/*
 * Remove any Node instances in NodeCollection which have expired time stamps
 *
 * Returns number of nodes removed.
 */
int NodeCollection_removeExpiredNodes(NodeCollection* nc, unsigned int expire_time){
	int num = 0;
	int newNodeCount = nc->nodeCount;
	time_t cmprTime = time(NULL) - CONFIG->PROTO_nodeMaxAge;

	int i = 0;
	/* Null out node if timestamp too old and update the newNodeCount*/
	for(i = 0; i < nc->nodeCount; i++){
		if(nc->nodes[i].timeStamp <= cmprTime){
			Node_nullOutNode(&nc->nodes[i]);
			newNodeCount--;
			num++;
		}
	}
	/* Make sure the nulled out nodes are at the bottom before updating nc->nodeCount */
	NodeCollection_sortByNodeID(nc);
	nc->nodeCount = newNodeCount;

	return num;
}

/* Remove all nodes with index > floor */
int NodeCollection_removeExcessNodes(NodeCollection* nc, unsigned int floor_value){
	int nodes_removed = 0;
	/* Executes only if needed */
	if(nc->nodeCount > floor_value){
		int i;
		for(i = floor_value ; i < nc->maxNodeCount ; i++){
			Node_nullOutNode(&nc->nodes[i]);
			nodes_removed++;
		}
		/* Log event */
		log_event(LOG_DEBUG, "%d excess nodes were removed",(nodes_removed));
		/* Update nodeCount*/
		nc->nodeCount = floor_value;
	}
	return nodes_removed;
}

/* Updates the list of candidate nodes cn based on Nodes in nc */
NodeCollection* NodeCollection_getCandidateNodes(NodeCollection* nc){
	/* Create new NodeCollection candidate nodes (nc) */
	NodeCollection* cn = NodeCollection_new(P2PDPRD_VERSION_ID, INTERNAL, nc->nodeCount);

	int i, j = 0;
	for(i = 0 ; i < nc->nodeCount ; i++ ){
		if(nc->nodes[i].utility >= 1.0f){
			memcpy(&cn->nodes[j], &nc->nodes[i], sizeof(Node));
			j++;
		}
	}
	cn->nodeCount = j;

	return cn;
}

Node* Node_getRandomImportantNode(NodeCollection* nc){
	Node* n = NULL;
	if(nc->nodeCount <= 0){
		log_event(LOG_DEBUG, "Tried to choose a random Node from zero candidates.");
	} else if (nc->nodeCount == 1){
		if(nc->nodes[0].nodeID != CONFIG->CLIENT_id){
			n = &(nc->nodes[0]);
		}
	} else {
		int r = -1;	/* random number to choose */
		int candidateAmount = NodeCollection_countCandidateNodes(nc);
		int nodeRange = 0;
		/* Determine nodeRange */
		if(nc->nodeCount < 10){
			nodeRange = nc->nodeCount;/* One of the existing nodes will be chosen */
		}else if(candidateAmount < 10){
			nodeRange = 9; /* One of the ten nodes at the top of the list will be chosen */
		}else{
			nodeRange = candidateAmount;/* one of the candidate nodes will be chosen */
		}

		int i; /* We know an endless loop condition should be impossible, */
		for(i = 0 ; i < 100 ; i ++){	/* however, always good to be safe and specify a max amount of iterations */
			/* Choose a random number within the range given in nodeRange */
			r = rand() % nodeRange;
			if(nc->nodes[r].nodeID != CONFIG->CLIENT_id){
				n = &(nc->nodes[r]);
				log_event(LOG_DEBUG, "Chose a random Node with ID: %d", nc->nodes[r].nodeID);
				break;
			}
		}
	}
	return n;
}

/* Returns amount of candidate nodes in NodeCollection pointed to by in */
int NodeCollection_countCandidateNodes(NodeCollection* in){
	int counter = 0;
	int i = 0;

	/* If its utility >= 1, a node is considered a 'candidate' node */
	for(i = 0; i < in->nodeCount; i++){
		if(in->nodes[i].utility >= 1)
			counter++;
	}
	return counter;
}

/* Calculate 'utility' of all nodes in nc with respect to Node n.
 * We are using geo-coordinates (lat, long) for the nodes. These need to be transformed
 * into x/y-pairs using carthesian meter-coordinates. This calculation is done using the
 * haversine() formula. See geo.h for details.
 *
 * The utility formula is as follows:
 *
 * utility(xi,yi,zi ; xj,yj,zj ; cri,crj) =
 * (cri + cr j )^2 / (xj − xi )^2 + (yj − yi )^2 + (zj − zi)^2
 */
void NodeCollection_calculateUtility(NodeCollection* nc, Node* n){

    int i;
    /* Initialize variables for node a and b*/
    for(i = 0 ; i < nc->nodeCount ; i++){
    	nc->nodes[i].utility = Node_utility(n, &nc->nodes[i]);
    }
}

/* Packs and sends NodeCollection pointed to by nc to address:port-pair in peerNode. */
int NodeCollection_sendToPeer(NodeCollection* nc, Node* peerNode){
	int buff_size = 0, bytes = 0;
	unsigned char* buff = NodeCollection_pack(nc, &buff_size);
	bytes = IO_sendBytes(buff, buff_size, peerNode->ipAddr, peerNode->port);

	free(buff);
	return bytes;
}

/* Calculates utility of Node b with respects to Node a */
double Node_utility(Node* a, Node* b){
	double ab_dist_sqrd = pow((geo_distance_meters(a->lat, a->lon, b->lat, b->lon)), 2);
	double ab_cr_sqrd = pow(a->coordRange + b->coordRange, 2);

	if (ab_dist_sqrd != 0)
		return ab_cr_sqrd / ab_dist_sqrd;
	else
		return DBL_MAX; // If distance is 0, we return the maximal utility
}
Node* Node_getRandomPeerNode(NodeCollection* nc){
	Node* n = NULL;
	if(nc->nodeCount <= 0){
		log_event(LOG_DEBUG, "Tried to choose a random Node from zero candidates.");
	} else if (nc->nodeCount == 1){
		if(nc->nodes[0].nodeID != CONFIG->CLIENT_id){
			n = &(nc->nodes[0]);
		}
	} else {
		int r = -1;			/* random number to choose */
		int i; 								/* We know an endless loop condition should be impossible, */
		for(i = 0 ; i < 100 ; i ++){		/* however, always good to be safe and specify a max amount of iterations */
			/* Choose a random number within range of the nodes */
			r = rand() % nc->nodeCount;
			if(nc->nodes[r].nodeID != CONFIG->CLIENT_id){
				n = &(nc->nodes[r]);
				log_event(LOG_DEBUG, "Chose a random Node with ID: %d", nc->nodes[r].nodeID);
				break;
			}
		}
	}
	return n;
}

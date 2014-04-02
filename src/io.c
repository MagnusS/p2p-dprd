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
 * Authors: Halvdan Hoem Grelland, Jostein Aardal, Magnus Skjegstad
 */

/*
 * io.c
 *
 *	Implementation of functions defined in io.h
 *	Refer to header file for documentation.
 *
 */
#include <errno.h>
#include "io.h"

/* Initialize variables for use with select() */
int IO_selectVars_init(fd_set* initSet , struct timeval* varTime, struct timeval* initTime, int networkSock, int localSock, time_t TIMEOUT){
	FD_ZERO(initSet);
	FD_SET(networkSock, initSet);
	FD_SET(localSock, initSet);

	initTime->tv_sec = TIMEOUT;
	initTime->tv_usec = 0;
	varTime->tv_sec = 0; /* Set to zero to ensure select() returns immidiately at first run */
	varTime->tv_usec = 0;

	/* Return the highest FD */
	if(networkSock > localSock)
		return networkSock;
	else
		return localSock;
}
/* Returns a FD to a new network socket. The socket is bound to the defined port. */
int IO_recvSocket_init(uint16_t port){
	/* Instantiate socket variables */
	struct sockaddr_in s;				/* Address of host */
	memset(&s, 0, sizeof(s));			/* Zero-out buffer */
	s.sin_family = AF_INET;				/* Domain is: IPv4 */
	s.sin_port = htons(port);			/* Convert port to network byte order and store in s */

	/* Create UDP-socket instance */
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	/* Check if successful */
	if(sock < 0){
		log_error(CRITICAL, errno, "Failed to create datagram socket");
		exit(EXIT_FAILURE);
	}

	/* Bind host address to socket */
	/* Check if we successfully bound socket to host address*/
	if(bind(sock, (struct sockaddr*)&s, sizeof(s)) < 0){
		/* Failed to bind socket to address. Critical error. */
		log_error(CRITICAL, errno, "Binding socket to port %d failed", port);
		exit(EXIT_FAILURE);
	} else {
		/* Successfully bound socket */
		log_event(LOG_DEBUG, "Listening on port %d\n", port);
	}
	return sock;
}


int IO_sendSocket_init(struct sockaddr_in* s, uint32_t ipAddr, uint16_t port){
	/* Instantiate socket variables */
	memset(s, 0, sizeof(*s));				/* Zero-out buffer */
	s->sin_family = AF_INET;				/* Use IPv4 */
	s->sin_port = htons(port);				/* Convert port to network byte order and store in s */
	uint32_t ip_network_order = htonl(ipAddr);
	/* Copy 32bit-encoded ip host-address to s. Address is converted to network byte order. */
	memcpy(&(s->sin_addr), &ip_network_order, sizeof(ip_network_order));

	/* Create UDP-socket instance */
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	/* Check if successful */
	if(sock < 0){
		/* Something went wrong creating the socket */
		return -1;
	} else {
		/* Socket successfully created */
		return sock;
	}
}

/* Send byte-buffer buffer to ip */
int IO_sendBytes(unsigned char* buffer, uint16_t buff_size, uint32_t ip, uint16_t port){
	/* Create send-socket */
	struct sockaddr_in s;
	int sock = IO_sendSocket_init(&s, ip, port);
	uint32_t ip_n = htonl(ip);	/* converted to network byte-order for presentation (ntop) */
	int sentBytes = -1;	

	/* Sanity checks */
	if(buff_size <= 0){
		log_error(CRITICAL, errno, "Tried to send a buffer of size 0");
		return -1;
	}

	if(buffer == NULL){
		log_error(CRITICAL, errno, "Tried to send a NULL-buffer");
	}

	if (sock){
		/* Send data on socket */
		sentBytes  = sendto(sock,						/* Socket to send on */
							buffer, 					/* Data to send */
							buff_size, 					/* Size of data in bytes */
							0,							/* No flags */
							(struct sockaddr *)&s,		/* Address of recipient */
							sizeof(struct sockaddr_in)	/* Size of address field */
		);

		/* Check is sendto() was successful */
		if (sentBytes < 0){
			/* Terminal error during sendto() */
			char addr_str[17];
			log_error(CRITICAL, errno, "There was an error sending data to %s : %d", inet_ntop(AF_INET, &ip_n, addr_str, INET_ADDRSTRLEN), port);
			exit(0);
		} else if (sentBytes != buff_size) {
			/* An error occured, causing some, but not all, data to be sent. */
			log_error(NOTICE, errno, "Buffer/send mismatch: %d of %d bytes was sent", sentBytes, buff_size);
		} else {
			/* Successful */
			char addr_str[17];		
			if(inet_ntop(AF_INET, &ip_n, addr_str, INET_ADDRSTRLEN) == NULL){
				printf("ntop_failed: %s\n", strerror(errno));
			}

			log_event(LOG_DEBUG, "%d bytes was successfully sent to %s : %d", sentBytes, inet_ntop(AF_INET, &ip_n, addr_str, INET_ADDRSTRLEN), port);
		}
	} else {
		log_error(CRITICAL, errno, "Failed to initialize socket");
		exit(0);
	}
	/* Close socket */
	close(sock);
	return sentBytes;
}

int LocalIO_localSocket_init(char* unix_sock_name){
	/* Instantiate socket variables */
	struct sockaddr_un s;								/* Address of host */
	memset(&s, 0, sizeof(s));							/* Zero out s */
	s.sun_family = AF_UNIX;								/* Local domain */
	strcpy(s.sun_path, unix_sock_name);					/* Assigning path */

	/* Create stream-socket instance */
	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	/* Check if successful */
	if(sock < 0){
		log_error(CRITICAL, errno, "Creating local listening socket on %s failed", unix_sock_name);
		exit(EXIT_FAILURE);
	} else {
		log_event(LOG_DEBUG,"Local listening socket created on %s", unix_sock_name);
	}

	/* Unlink to ensure that socket path is not bound already */
	unlink(s.sun_path);

	int len = (strlen(s.sun_path) + sizeof(s.sun_family));	/* Size of address space */

	/* Bind unix address to socket */
	if(bind(sock, (struct sockaddr*)&s, len) < 0){
		/* Failed to bind socket to address. Critical error. */
		log_error(CRITICAL, errno, "Binding socket to %s failed", unix_sock_name);
		exit(EXIT_FAILURE);
	} else {
		/* Successfully bound socket */
		log_event(LOG_DEBUG, "Listening on local socket %s\n", unix_sock_name);
	}

	return sock;
}

int LocalIO_sendCandidateNodes(NodeCollection* cn, SubscriberList* subs, Node* ownNode){

    /* Create empty collection with room for own node */
    NodeCollection* nodes_to_send = NodeCollection_new(
            P2PDPRD_VERSION_ID,
            INTERNAL,
            cn->nodeCount + 1
            );

    /* Put own node first */
    memcpy(&nodes_to_send->nodes[0], ownNode, sizeof(Node));
    nodes_to_send->nodeCount = 1;

    /* Append node collection to nc_mod */
    NodeCollection_append(nodes_to_send, cn, ownNode->nodeID);

	/* Send nodes_to_send on sock_path */
	int data_size = 0;
	
    unsigned char* data = NodeCollection_pack(nodes_to_send, &data_size);
	
    int sentBytes = -1, bytes_total = 0;
    NodeCollection_destroy(nodes_to_send);

	int i;
	for(i = 0 ; i < subs->num_subs ; i++){
		struct sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, subs->subscribers[i]->socket_address);
		int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
		if(sock < 0){
			log_error(NOTICE, errno ,"Problem creating local socket");
			break;
		}

		int addr_len = strlen(addr.sun_path) + sizeof(addr.sun_family);

		sentBytes = sendto(	sock,
							data,
							data_size,
							0,
							(struct sockaddr *)&addr,
							addr_len
		);

		if(sentBytes < 0){
			log_error(NOTICE, errno, "Sending candidate Nodes on local socket");
		} else {
			bytes_total += sentBytes;
			log_event(LOG_DEBUG, "Delivered %d bytes on socket: %s", sentBytes, subs->subscribers[i]->socket_address);
		}
	}

	free(data);

	return bytes_total;
}

int LocalIO_handleRequest(LocalRequest* lr, Config* config, SubscriberList* subs){
	int success = 0;

	switch (lr->type){
		case SET_POSITION:
		{
			Config_setNodePosition(lr->values->lat, lr->values->lon, config);
			success = 1;
			break;
		}
		case SET_COORDINATION_RANGE:
		{
			Config_setNodeCoordinationRange(lr->values->coord_range, config);
			success = 1;
			break;
		}
		case SET_POS_AND_RANGE:
		{
			Config_setNodePosition(lr->values->lat, lr->values->lon, config);
			Config_setNodeCoordinationRange(lr->values->coord_range, config);
			success = 1;
			break;
		}
		case SUB_CANDNODES:
		{
			/* TODO: Strlen() causes uninitiliazed value-failure in valgrind. Possible strlen-bug in uClibc */
			Subscriber* new_sub = Subscriber_new(lr->values->sock_addr, strlen(lr->values->sock_addr) + 1);
			
		
			int res = SubscriberList_addSub(subs, new_sub);

			if (res == 1){
				log_event(LOG_DEBUG, "Socket address %s has been subscribed to the candidate nodes service.", lr->values->sock_addr);
			} else if (res == 0){
				log_event(LOG_DEBUG, "A subscription to the canidate nodes list by %s was denied. List is full.", lr->values->sock_addr);
			} else {
				/* res == -1 */
				log_event(LOG_DEBUG, "Failed to add %s to subscriber list.", lr->values->sock_addr);
			}
			success = 1;
			break;
		}
		case UNSUB_CANDNODES:
		{
			int res = SubscriberList_removeSub(subs, Subscriber_new(lr->values->sock_addr, strlen(lr->values->sock_addr)));

			if(res == 1){
				log_event(LOG_DEBUG, "Subscriber on %s was removed from subscription list", lr->values->sock_addr);
			} else {
				log_event(LOG_DEBUG, "Failed to remove subscriber %s - no such subscriber found", lr->values->sock_addr);
			}
			success = 1;
			break;
		}
		default:
		{
			log_event(LOG_DEBUG, "Tried to process LocalRequest of undefined type.");
			success = 0;
			break;
		}
	}
	return success;
}

LocalRequest* LocalRequest_new(LOCAL_REQ_TYPE req_type, double lat, double lon, uint16_t coord_range, char sock_addr[LOCAL_ADDR_MAX_LENGTH]){
	LocalRequest* lr = malloc(sizeof(LocalRequest));
	lr->values = malloc(sizeof(request_values));

	lr->type = req_type;
	lr->values->lat = lat;
	lr->values->lon = lon;
	lr->values->coord_range = coord_range;
	strcpy(lr->values->sock_addr, sock_addr);

	return lr;
}

void LocalRequest_destroy(LocalRequest* lr){
	free(lr->values);
	free(lr);
}

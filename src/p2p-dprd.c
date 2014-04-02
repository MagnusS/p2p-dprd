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
 * Authors: Halvdan Grelland, Jostein Aardal, Magnus Skjegstad
 */

/*
 *	Implementation of the Peer-to-Peer Discovery Protocol for Radio Devices (P2P-DPRD)
 *
 *	See README.MD for further documentation.
 */

/* Defining DEBUG will enable log-output to stdout */
#define DEBUG

/* Include headers */
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>

#include "debug.h"
#include "node.h"
#include "protocol.h"
#include "subscribe.h"

int RUNNING = 1;		/* Flag to determine run-status, 0 or 1 */

/* Function to initiate graceful shutdown on SIGTERM */
void terminate(){
	log_event(LOG_DEBUG, "TEST: Received signal to shut down, exiting program...\n");
	RUNNING = 0;
}

/* Define global CONFIG */
Config* CONFIG;

int main(int argc, char* argv[]){
	/* Catch SIGTERM and SIGINT for graceful termination */
	signal(SIGTERM, terminate);
	signal(SIGINT, terminate);

    /* Seed random generator (ONCE) - we should by all means use a better seed, by the way */
	srand(time(NULL));

	/* ---------- Set up and handle configuration ---------- */
	CONFIG = Config_new();					/* Allocate global Config CONFIG */
	Config_set(argc, argv[1], CONFIG);		/* Set config. Get config from config-file (arg[1]) */
	
	log_event(LOG_DEBUG, "P2P identifier is %d", CONFIG->CLIENT_id);


	/* ---------- Initialise data structures in memory ---------- */
	NodeCollection* importantNodes 	= NodeCollection_new(P2PDPRD_VERSION_ID, INTERNAL, (CONFIG->PROTO_M + CONFIG->PROTO_K));
	NodeCollection* randomNodes		= NodeCollection_new(P2PDPRD_VERSION_ID, INTERNAL, (CONFIG->PROTO_N * 2));

	/* Allocate subscriber list */
	SubscriberList* subs = SubscriberList_new(MAX_NUM_SUBSCRIBERS);
	/* Allocate local socket recieve buffer */
	unsigned char* local_sock_buf = (unsigned char*) malloc(LOCAL_SOCK_BUF_SIZE);


	/* ---------- Initialise I/O and message handling ---------- */

	/* Declare variables and structures for select() call */
	int selectState = 0;		/* Select writes its state to this variable upon return */
	fd_set varSet;				/* Set of fd's which select() writes to */
	fd_set initSet;				/* Set of fd's which are used to reset select()'s state */
	struct timeval varTime;		/* Timeout which is set in select() */
	struct timeval initTime;	/* Timeout which is used to reset select()'s state */

	/* Set up network socket. A FD to the socket is returned if successful. */
	int networkSock = IO_recvSocket_init(CONFIG->NETWORK_port);
	/* Set up local listening socket. A FD to the socket is returned if successful. */
	int localSock = LocalIO_localSocket_init(CONFIG->LOCAL_socketPath);

	/* Initilize the above variables for use with select()-call */
	int largestSock = IO_selectVars_init(
										&initSet,
										&varTime,
										&initTime,
										networkSock,
										localSock,
										(time_t)CONFIG->PROTO_timeout
	);

	/* Our main select()-loop used to catch and handle events.
	 *
	 * There are four main types of events, causing select() to return:
	 * 		1. Something arrived on the listening network socket
	 * 		2. The internal client timeout was triggered
	 * 		3. Something has arrived on the listening local socket
	 * 		4. There was an error
	 *
	 * 	The select() call is used to monitor and respond to these events
	 */

    long last_cleanup_timestamp = time(NULL);

	/* ---------- Start main loop ---------- */
	while(RUNNING){
		varSet = initSet;	/* The varSet fd-set is changed by the system upon return of select().
		 * Therefore it needs to be reset to the value of initSet.
		 */

		/* Call select() and save the state in selectState. Program will wait until select() returns. */
		selectState = select(largestSock+1,		/* Check range [0 -> current (sock) + 1]  of FD's*/
				&varSet,		/* Save returned info in varSet */
				NULL,			/* No write*/
				NULL,			/* No exception */
				&varTime		/* Update time to timeout in varTime */
		);
		/* Select has returned. Check what event caused it. */

		if(selectState < 0 && RUNNING){
			/* There was an error during call to select() */
			log_event(LOG_ERROR, "Select() returned error %d: %s", largestSock, errno, strerror(errno));
			varTime = initTime;
			varTime.tv_usec = (time_t) rand() % CONFIG->PROTO_timeout_variation; /* Add random interval to timeout */

		} else if (selectState == 0){
			/* Reset timer */
			varTime = initTime;
			varTime.tv_usec = (time_t) rand() % CONFIG->PROTO_timeout_variation; /* Add random interval to timeout */
	
		} else if(selectState > 0){
			/* select() returned on socket activity */

			/* Check wich socket triggered select and take appropriate action*/
			if(FD_ISSET(networkSock, &varSet)){
				/* The network-socket FD is set >> Something has arrived on the network-socket we are listening on.
				 * Run routine to receive and handle data from peer.
				 */
				Protocol_receiveFromPeer(networkSock, importantNodes, randomNodes);

			} else if(FD_ISSET(localSock, &varSet)){
				/* Something has arrived on the local socket we are listening on.
				 * Run routine to receive and handle data from system.
				 */

				/* Zero out receive buffer before writing to it */
				memset(local_sock_buf, 0, LOCAL_SOCK_BUF_SIZE);

				/* Read data from local socket */
				int bytes = recv(localSock, local_sock_buf, LOCAL_SOCK_BUF_SIZE , 0);
				if (bytes < 0){
					log_event(LOG_ERROR, "Corrupt data was received on local socket");
				} else if (bytes > 0) {
					/* Handle buffer contents */
					LocalRequest* lr = LocalRequest_unpack(local_sock_buf, bytes);
					
					if (lr){	/* Check for null before trying to handle */
						LocalIO_handleRequest(lr, CONFIG, subs);
						LocalRequest_destroy(lr);
					}

				}
			}
		}

        if (time(NULL) - last_cleanup_timestamp > CONFIG->PROTO_timeout) {
            log_event(LOG_DEBUG,"Performing periodic cleanup.");

            /* This used to be called when select() returned on a timeout, but is now called periodically */
            Protocol_timeout(randomNodes, importantNodes);

            /* TEMPORARY TEST */
            NodeCollection* cn = NodeCollection_getCandidateNodes(importantNodes);
            printf("Found %d candidate nodes...\n", cn->nodeCount);
            if (subs->num_subs > 0){
                /* Create own Node */
                Node* ownNode = Node_createOwnNode();
                int cand_bytes_total = LocalIO_sendCandidateNodes(cn, subs, ownNode);
                log_event(LOG_DEBUG, "Sent %d bytes to %d subscribers\n", cand_bytes_total, subs->num_subs);
                Node_destroy(ownNode);
            }

            NodeCollection_destroy(cn);
            /* TEST END */

            last_cleanup_timestamp = time(NULL);
        }
	}

	/* ---------- Clean up ---------- */

	/* Close open sockets */
	close(networkSock);
	close(localSock);

	/* Free allocated memory */
	NodeCollection_destroy(importantNodes);
	NodeCollection_destroy(randomNodes);
	SubscriberList_destroy(subs);
	free(local_sock_buf);

	/* Unlink local listening socket from local socket path */
	unlink(CONFIG->LOCAL_socketPath);

	Config_destroy(CONFIG);

	/* End main */
	return EXIT_SUCCESS;
}

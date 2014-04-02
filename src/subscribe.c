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
 * subscribe.c
 *
 *	Implementation of functions defined in subscribe.h
 *	Refer to header file for documentation.
 *
 *      Authors: Halvdan Hoem Grelland & Jostein Aardal
 */

#include "subscribe.h"
#include <string.h>

Subscriber* Subscriber_new(char* address, unsigned int address_length){
	Subscriber* cs = malloc(sizeof(Subscriber));
	cs->socket_address = malloc(address_length);
	cs->address_length = address_length;
	strcpy(cs->socket_address, address);
	
	return cs;
}
void Subscriber_destroy(Subscriber* sub){
	free(sub->socket_address);
	free(sub);
}

SubscriberList* SubscriberList_new(int max_num_subs){
	SubscriberList* subs = malloc(sizeof(SubscriberList));
	subs->max_num_subs = max_num_subs;
	subs->num_subs = 0;

	return subs;
}

void SubscriberList_destroy(SubscriberList* sl){
	int i = 0;
	for(i = 0 ; i < sl->num_subs ; i++){
		free(&sl->subscribers[i]);
	}
	free(sl);
}

int SubscriberList_addSub(SubscriberList* subs, Subscriber* new_sub){

	if(subs->num_subs < subs->max_num_subs){
		/* There are room for more subs */

		/* Check if already subscribed */
		int k = 0, match = 0;
		for (k = 0 ; k < subs->num_subs ; k++){
			if(new_sub->address_length == subs->subscribers[k]->address_length){
				/* Same length addresses, check if identical */
				if(strcmp(new_sub->socket_address, subs->subscribers[k]->socket_address) == 0){
					/* Identical addresses */
					match++;
					break;
				}
			}
		}

		if(match == 0){
			subs->subscribers[subs->num_subs] = new_sub;		/* Append new sub */
			subs->num_subs++;

			/* Success */
			return 	1;
		} else {

			/* Already subscribed */
			return -1;
		}
	} else if (!new_sub){
		return -1;
	} else {
		/* The sub-list is full */
		return 0;
	}
}

int SubscriberList_removeSub(SubscriberList* subs, Subscriber* rmv_sub){
	/* Check if in subList, find index */
	int i = 0, index = -1;
	for(i = 0 ; i < subs->num_subs ; i++){
		if(strcmp(subs->subscribers[i]->socket_address, rmv_sub->socket_address) == 0){
			index = i;
		}
	}

	if(index > -1){
		/* Remove sub */
		Subscriber_destroy(subs->subscribers[index]);

		/* Propagate in list */
		int k = 0;
		for (k = index ; k < subs->num_subs - 1 ; k++){
			subs->subscribers[k] = subs->subscribers[k + 1];
		}
		/* Update num_subs */
		subs->num_subs--;

		return 1;	/* Return success */
	} else {
		/* Sub was not in list */
		return 0;	/* Return failure */
	}

}

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
 * subscribe.h
 *
 * Declaration of datatypes and functions which form the local candidate node subscription-mechanism
 *
 *      Author: Halvdan Hoem Grelland and Jostein Aardal
 */

#ifndef INCLUDE_SUBSCRIBE_H_
#define INCLUDE_SUBSCRIBE_H_

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

/* Hard-coded maximum (allocated) size of subscriber-list. */
#define MAX_NUM_SUBSCRIBERS 25

/* Wrapper object for a subscriber socket address */
typedef struct Subscriber {
	char* 		socket_address;	/* Path to the subscriber's unix domain listening socket */
	unsigned int	address_length;	/* Byte-size of socket_address */
} Subscriber;

/* Wrapper object for list of Subsribers */
typedef struct SubscriberList {
	Subscriber* 		subscribers[MAX_NUM_SUBSCRIBERS];	/* Array of subscribers */
	unsigned int		num_subs;							/* Actual number of subscribers */
	unsigned int		max_num_subs;						/* Maximum (allocated) size of subscriber array */
} SubscriberList;

/*
 * Construct a new Subscriber
 * 	Arguments:
 * 		address			- Pointer to path/address of new subscriber
 * 		address_length	- Byte-size of address (minus null-terminator)
 * 	Returns:
 * 		Subscriber*		- Pointer to new Subscriber
 *
 * 	Subscriber is allocated on heap, use Subscriber_destroy to free memory properly
 */
Subscriber* Subscriber_new(char* address, unsigned int address_length);

/*
 * Destroy/free a Subscriber
 * 	Arguments:
 * 		sub	- Pointer to a Subscriber instance
 * 	Returns:
 * 		void
 * 	Frees memory. Subscriber should be constructed using Subscriber_new
 */
void Subscriber_destroy(Subscriber* sub);

/*
 * Construct a new SubscriberList
 * 	Arguments:
 * 		max_num_subs	- Number of Subscriber-instances to allocate on heap
 * 	Returns:
 * 		SubscriberList*	- Pointer to new SubscriberList
 *
 * 	Use SubscriberList_destroy() to properly free memory
 */
SubscriberList* SubscriberList_new(int max_num_subs);

/*
 * Destroy/free a SubscriberList
 * 	Arguments:
 * 		sl	- SubscriberList to destroy/deconstruct
 * 	Returns:
 * 		void
 *
 * 	SubscriberList should have been instantiated with Subscriber_new()
 */
void SubscriberList_destroy(SubscriberList* sl);

/*
 * Add a new Subscriber to a Subscriber-list
 * 	Arguments:
 * 		subs	- Pointer to SubscriberList to add to
 * 		new_sub	- Pointer to Subscriber to add
 * 	Returns:
 * 		int - 	1 if subscriber is added, 0 if list if full,
 * 				-1 if subscriber is already subscribed
 */
int SubscriberList_addSub(SubscriberList* subs, Subscriber* new_sub);

/*
 * Remove a subscriber from a SubscriberList
 * 	Arguments:
 * 		subs	- Pointer to SubscriberList
 * 		rmv_sub	- Pointer to Subscriber to remove from subs
 * 	Returns:
 * 		int		- 1 if success (removed), 0 if failure (sub not found in list)
 */
int SubscriberList_removeSub(SubscriberList* subs, Subscriber* rmv_sub);

#endif /* INCLUDE_SUBSCRIBE_H_ */

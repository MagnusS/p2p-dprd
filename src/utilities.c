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
 * utilities.c
 *
 *	Implementation of functions defined in utilities.h
 *	Refer to header file for documentation.
 *
 *      Authors: Halvdan Hoem Grelland & Jostein Aardal
 */

#include "utilities.h"

/* Logs logMsg to file. Supports printf-style
   	   	   variable arguments and format specifier string */
void log_event(logType type, char* logMsg, ...){
	va_list args;			/* Supplied variable arguments */
	va_start(args, logMsg);	/* Init variable arguments */

	FILE* logFile;
	char ts[64];
	writeTimestamp(ts);

	/* Open file and do appropriate error-checking */
	//if((logFile = fopen(CONFIG->LOG_path, "a+")) == NULL){
	if((logFile = fopen(CONFIG->LOG_path, "a+")) == NULL){
		D(printf("Error opening file %s - Program will exit - ERRNO: %s\n", CONFIG->LOG_path, strerror(errno)));
		exit(EXIT_FAILURE);
	}
	/* Check type and log appropriate message */
	char msg[256];		/* The inner message-string*/
	char str[384];		/* Final product to print to stdout and file*/
	
	int str_len = 0;

	vsprintf(msg, logMsg, args);			/* Build inner message-string from variable args*/
	if(type == LOG_ERROR){
		str_len = sprintf(str, "%s ERR: %s\n", ts, msg);	/* Format final string for file */
		D(printf("P2PDPRD ERR: %s\n", msg));			/* Print message to stout */
	} else if (type == LOG_DEBUG){
		str_len = sprintf(str, "%s DBG: %s\n", ts, msg);	/* Format final string for file */
		D(printf("DBG: %s\n", msg));			/* Print message to stout */
	}

	fwrite(str, 1 , str_len, logFile);		/* Print to file */

	fclose(logFile);	/* Close filehandle */
	va_end(args);		/* Cleanup variable arguments */
}

void log_error(errType priority, int err, char* errMsg, ...){
	char logMsg[P2PDPRD_LOG_MAX_MSG_SIZE];

	/* Init variable argument-stuff */
	va_list args;
	va_start(args, errMsg);

	vsprintf(logMsg, errMsg, args);	/* Build string from format and var-args */

	/* Check priority and log appropriate message */
	if(priority == CRITICAL)
		snprintf(logMsg, P2PDPRD_LOG_MAX_MSG_SIZE, "%s - This is a critical error, program will exit - ERRNO: %s", errMsg, strerror(err));
	else if(priority == NOTICE)
		snprintf(logMsg, P2PDPRD_LOG_MAX_MSG_SIZE, "%s - ERRNO: %s", errMsg, strerror(err));
	log_event(LOG_ERROR, logMsg);

	va_end(args);		/* Cleanup variable argument stuff */
}

/* A pretty simple and unrealiable way to get the ip of the host.
 * Will get the last supplied address, which may or may not be the
 * actual Internet-address of the host. Only used as fallback. */
int getHostIPAddress(){
	struct ifaddrs* ifAddrStruct = NULL;
	struct ifaddrs* ifa = NULL;
	uint32_t* networkByteIP_ptr = malloc(INET_ADDRSTRLEN);
	uint32_t ip;

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next){
		/* Check if it is ipv4 */
		if (ifa->ifa_addr->sa_family == AF_INET){
			memcpy(networkByteIP_ptr, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, INET_ADDRSTRLEN);
		}
	}
	/* Store contents of memory pointed to by networkByteip_ptr in IP. */
	ip = *networkByteIP_ptr;
	/* Log address */
	char address[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, networkByteIP_ptr, address, INET_ADDRSTRLEN);
	char msg[56];
	sprintf(msg,"Read own ip: %s",address);
	log_event(LOG_DEBUG, msg);
    
	/* Cleaning */
	if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
	free(networkByteIP_ptr);
	return ip;
}

/* Prints timestamp to (empty) string pointed to by str */
char* writeTimestamp(char* str){
	/* Create timestamp string */
    time_t ltime; /* calendar time */
    struct tm *tm;

    ltime=time(NULL); /* get current epoch time */
    tm = localtime(&ltime);	/* Transform into calendar time */
    /* Print formatted timestamp string to str */
    sprintf(str, "%d:%d:%d/%d:%d:%d -",
    		tm->tm_mday,
    		tm->tm_mon + 1,
    		tm->tm_year + 1900,
    		tm->tm_hour,
    		tm->tm_min,
    		tm->tm_sec
    );
    return str;
}

/* Calculate distance between two coordinate points. */
double geo_distance_meters(double th1, double ph1, double th2, double ph2){
	double dx, dy, dz;
	ph1 -= ph2;
	ph1 *= TO_RAD, th1 *= TO_RAD, th2 *= TO_RAD;

	dz = sin(th1) - sin(th2);
	dx = cos(ph1) * cos(th1) - cos(th2);
	dy = sin(ph1) * cos(th1);
	return asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R;
}

uint32_t generateUniqueID(){
	
	return (uint32_t)rand();
}

/* Helper function to convert uint64_t to network byte order. 
 * Taken from http://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c */ 
int64_t htonll(uint64_t value)
{
    // The answer is 42
    static const int num = 42;

    // Check the endianness
    if (*(char *)&num == num) // must flip
    {
        const uint32_t high_part = htonl((uint32_t)(value >> 32));
        const uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));

        return (((uint64_t)low_part) << 32) | high_part;
    } else
    {
        return value;
    }
}

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
 * utilities.h
 *
 * Not-so-closely related functions and macros handling program support-tasks
 *
 *      Author: Halvdan Hoem Grelland and Jostein Aardal
 */

#ifndef INCLUDE_UTILITIES_H_
#define INCLUDE_UTILITIES_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

#include "configuration.h"

/* Geo-position */

/* WGS84 arithmetic mean radius of the Earth = (2a+b)/3 */
#define R 6371008.7714 /* meters */
#define TO_RAD (3.1415926536 / 180)	/* Degree to rad conversion */

/*
 * Calculate distance between two geo-positional points
 *	Arguments:
 *		th1	- latitude of P1
 *		ph1	- longitude of P1
 *		th2	- latitude of P2
 *		ph2	- longitude of P2
 *	Returns:
 *		double	- distance between points in meters
 *
 *	NOTE:
 *		The function implements the haversine formula to calculate spherical
 *		surface distance between the two points. As the earth is not perfectly
 *		spherical, the calculation is an approximation.
 *		The defined constant R is used as a measure of the earth radius.
 *
 */
double geo_distance_meters(double th1, double ph1, double th2, double ph2);

/* Logging */

#define P2PDPRD_LOG_MAX_MSG_SIZE 512   	 /* The maximum number of characters that can be used in a log-message */

/* Enumeration of error-types */
typedef enum errType {CRITICAL, NOTICE} errType;
/* Enumeration of logging types */
typedef enum logType {LOG_ERROR, LOG_DEBUG} logType;

/* Log an event
 *	Arguments:
 *		type	- LOG_ERROR || LOG_DEBUG - identifies criticality of event
 *		logMsg	- Pointer to char-array containing the format string
 *		...		- Arguments to format-string
 *	Returns:
 *		void
 *
 *	Note:
 *		The function implements a printf-style format-string/arguments argument structure.
 *		Prints to stdout if 'D(x)' is defined to 'x', always writes to the log file
 */
void log_event(logType type, char* logMsg, ...);

/*
 * Log an error
 * 	Arguments:
 * 		priority	- CRITICAL || NOTICE - severity of error
 * 		err			- Error-flag, typically ERRNO
 * 		errMsg		- Pointer to error format-string
 * 		...			- Arguments to format-string
 * 	Returns:
 * 		void
 *
 * 	Note:
 * 		Logs an error-message to file and stdout (if 'D(x)' defined to 'x').
 * 		Does not handle the error itself.
 * 		The error-flag is used for lookup of an appropriate
 * 		error-string, which is appended to the message.
 * 		Uses printf-style format string/arguments.
 *
 */
void log_error(errType priority, int err, char* errMsg, ...);

/*
 * Writes a formatted, current timestamp
 * 	Arguments:
 * 		str	- Pointer to string to write to
 * 	Returns:
 * 		char*	- Pointer to string (str)
 *
 * 	Format of timestamp - d:m:y/h:m:s
 */
char* writeTimestamp(char* str);

/*
 * Get the IP-address of the host
 * 	Arguments:
 * 		void
 * 	Returns:
 * 		int	- Network-formatted IPv4-address
 *
 * 	Note that this function is highly unreliable as it performs unqualified picking of the IP.
 * 	Will work if the host has one and only one IP (rarely the case). Might work if the relevant
 * 	IP is the last in the system-call lookup (e.g. ifconfig) results. Only to be used as fallback.
 */
int getHostIPAddress();

/*
 * Generate a unique, random identifier
 * 	Arguments:
 * 		void
 * 	Returns:
 * 		uin32_t	- Generated ID
 *
 */
uint32_t generateUniqueID();

int64_t htonll(uint64_t value);

/* Use to get colored output in formatted print statements */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#endif /* INCLUDE_UTILITIES_H_ */

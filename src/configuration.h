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
 * configuration.h
 *
 * Data types, constants and function prototypes relating to configuration
 * of protocol, client and system parameters. Also contains defaults/fallbacks.
 *
 * Most parameters are modifiable via configuration file.
 *
 *      Authors: Halvdan Hoem Grelland & Jostein Aardal
 */

#ifndef INCLUDE_CONFIGURATION_H_
#define INCLUDE_CONFIGURATION_H_

/* Includes */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libconfig.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#include "debug.h"
#include "utilities.h"

/* Configuration default values. These are in most cases used as fallback if a paramter
   is not defined in config-file, or the config-file cannot be found/read */

/* The program version ID. Hard-coded and not configurable at run-time. */
#define P2PDPRD_VERSION_ID 1
#define P2PDPRD_NODES_MAX_SIZE 10000	/* Absolute (hard limit) maximum size of a NodeCollection. */

/*
 * P2PDPRD constants default values.
 * These values are configurable in the configuration file. The default values
 * reflect the recommended values (as defined by the current draft of the protocol).
 *
 * Refer to the protocol whitepaper for further explanation.
 */
#define CFG_DEFAULT_P2PDPRD_CONSTANT_N	10
#define CFG_DEFAULT_P2PDPRD_CONSTANT_M	30
#define CFG_DEFAULT_P2PDPRD_CONSTANT_K	15

/* Default important-list max node count */
#define CFG_DEFAULT_P2PDPRD_IMPORTANT_NODES_MAX_SIZE (CFG_DEFAULT_P2PDPRD_CONSTANT_M + CFG_DEFAULT_P2PDPRD_CONSTANT_K)
/* Default random-list max node count */
#define CFG_DEFAULT_P2PDPRD_RANDOM_NODES_MAX_SIZE (2 * CFG_DEFAULT_P2PDPRD_CONSTANT_N)

/* Default path of log file. Only used as fallback if no path is specified in cfg-file
 * The program does NOT create directories, meaning is is up to the user to make sure
 * the given path exists. The program will, however, create the file if it doesn't exist */
#define CFG_DEFAULT_LOG_PATH "p2p-dprd.log"

/* Default fallback local listening socket used to handle local service calls.
 * This path is configurable in the configuration file.
 *
 * The path is used for other system processes to subscribe to candidate nodes, as well
 * as changig paramters of the service (e.g. geo-position, coord-range). */
#define CFG_DEFAULT_LOCAL_SOCK "/tmp/p2p-dprd.sock"

/* Network defaults */
#define CFG_DEFAULT_PORT 45454				/* IP-domain listening port */
#define CFG_DEFAULT_PEER_IP "127.0.0.1"		/* Default fallback IP of the origin peer (for bootstrapping) */
#define CFG_DEFAULT_PEER_PORT 45544			/* Port of origin peer */
#define CFG_DEFAULT_RADAC_IP "127.0.0.1"	/* Default IP of associated radac-instance */
#define CFG_DEFAULT_RADAC_PORT 45542		/* Default port of associated radac-instance */

/* Discovery protocol defaults. Needs to be properly defined in config-file for
   correct and useful operation of the program and discovery service. */
#define CFG_DEFAULT_CLIENT_TIMEOUT 10					/* Timeout base-value in seconds*/
#define CFG_DEFAULT_CLIENT_TIMEOUT_VARIATION 2000000	/* Timeout variation on base value, given in microseconds */
#define CFG_DEFAULT_CLIENT_COORD_RANGE 10				/* Coordination range in meters */
#define CFG_DEFAULT_CLIENT_LAT 59.921161				/* Geo-position - latitude */
#define CFG_DEFAULT_CLIENT_LON 10.733608				/* Geo-position - longitude */
#define CFG_DEFAULT_NODE_AGE_LIMIT 10800				/* Default max age of Node object - in seconds */

/* Buffer/string size limits.
 *
 * Note that most systems allow up to 4096 chars for a file path.
 * This program WILL cut strings at the defined values, thus giving the possibility of long file-paths
 * being misrepresented. Please make sure paths are withing set limits. */
#define LOCAL_SOCK_BUF_SIZE 1024	/* Buffer size of local socket */
#define MAX_SOCK_PATH_LENGTH 512	/* Allowed max-length of a socket path */
#define MAX_LOG_PATH_LENGTH 512		/* Allowed max-length of the log path */

/* Structure to hold the program-wide configuration.
   Holds a collection of all variable and static parameters used throughout the system. */
typedef struct Config {
	/* Network config */
	uint32_t	NETWORK_originPeerIP;
	uint16_t	NETWORK_originPeerPort;
	uint32_t	NETWORK_ownIP;
	uint16_t	NETWORK_port;
	char		LOCAL_socketPath[MAX_SOCK_PATH_LENGTH];
	/* P2PDPRD client config */
	uint32_t	CLIENT_id;
	double		CLIENT_lat;
	double		CLIENT_lon;
	uint16_t	CLIENT_coordRange;
	/* P2PDPRD protocol config */
	uint32_t	PROTO_nodeMaxAge;
	uint16_t	PROTO_timeout;
	uint32_t	PROTO_timeout_variation;
	uint16_t	PROTO_N;
	uint16_t	PROTO_M;
	uint16_t	PROTO_K;
	/* Radac-config */
	uint32_t	RADAC_ip;
	uint16_t	RADAC_port;
	/* Dev/debug config */
	char 		LOG_path[MAX_LOG_PATH_LENGTH];
} Config;

/* The program instantiates and uses a GLOBAL config structure. */
extern Config* CONFIG;

/*
 * Set config - wrapper function to set the configuration.
 * Gets cfg-file path from commandline-supplied path
 * 	Arguments:
 * 		argc	- Number of arguments in argv
 * 		argv	- Pointer to array of chars (cmdline-supplied string)
 *
 * 	Returns:
 * 		void
 */
void Config_set(int argc, char* argv, Config* cfg);

/*
 * Reads config parameters from the config-file - store in cfg
 * 	Arguments:
 * 		path 	- path to config file
 * 		cfg		- pointer to Config-object
 *
 * 	Returns:
 * 		int		- 1 on success, 0 on failure
 */
int Config_readFromFile(char* path, Config* cfg);

/*
 * Sets config cfg to hard-coded default values
 * 	Arguments:
 * 		cfg		- Pointer to Config-object
 */
void Config_setToDefault(Config* cfg);

/*
 * Updates the node positional data (lat/lon)
 * 	Arguments:
 * 		lat		- New latitude
 * 		lon		- New longitude
 * 		cfg		- Pointer to Config-object
 *
 * 	Returns:
 * 		void
 */
void Config_setNodePosition(double lat, double lon, Config* cfg);

/*
 * Updates the node coordination range
 * 	Arguments:
 * 		coordRange	- New coordination range
 * 		cfg			- Pointer to Config-object
 *
 * 	Returns:
 * 		void
 */
void Config_setNodeCoordinationRange(uint16_t coordRange, Config* cfg);

/*
 * Construct an instance of Config
 * 	Arguments:
 * 		void
 * 	Returns:
 * 		Config*	- Pointer to Config
 */
Config* Config_new();

/*
 * Free Config from memory
 * 	Arguments:
 * 		cfg	- Pointer to Config to deconstruct
 * 	Returns:
 * 		void
 */
void Config_destroy(Config* cfg);

#endif /* INCLUDE_CONFIGURATION_H_ */

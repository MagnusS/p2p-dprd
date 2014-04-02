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
 * configuration.c
 *
 *	Implementation of functions defined in configuration.h
 *	Refer to header file for documentation.
 *
 *      Authors: Halvdan Hoem Grelland & Jostein Aardal
 */

#include "configuration.h"
#include "utilities.h"

/* Reads global config from file given by filepath */
int Config_readFromFile(char* path, Config* c){
	
	/* The libconfig API handling of small ints changed from version 8 to 9. A dirty workaround is to use a temp-variable and typecast afterwards */
	int tmp_int = 0;
	
	config_t cfg;
	config_setting_t* setting;

	/* Initialize configuration */
	config_init(&cfg);

	/* Read the configuration file. Exit and report error on failure. */
	if(!config_read_file(&cfg, path)){
		D(printf("Config error: %d - %s\n", config_error_line(&cfg), config_error_text(&cfg)));
		config_destroy(&cfg);
		return 0;
	}

	/* Read network configuration */
	setting = config_lookup(&cfg, "network_cfg");

	printf("\nReading config from file:");
	if(setting){	/* non-NULL result */
		/* Read host_ip */
		const char* tmp;
		if(config_setting_lookup_string(setting, "host_ip", (void *)&tmp)){
			uint32_t ip;
			inet_pton(AF_INET, tmp, &ip);	/* Convert to network ordered uint  */
			ip = ntohl(ip);		/* Convert to host byte order  */
			c->NETWORK_ownIP = ip;

			D(printf("\n\tHost IP: %s", tmp));
		} else {
			D(printf("\n\tNo 'host_ip' set in configuration file.\n\tTrying to fetch IP from system (unreliable)."));
			c->NETWORK_originPeerIP = getHostIPAddress();
		}
		/* Read host_port*/
		if(config_setting_lookup_int(setting, "host_port", (int *)&tmp_int)){
			c->NETWORK_port = (uint16_t)tmp_int;		
			D(printf("\n\tNetwork port: %d", c->NETWORK_port));
		} else {
			D(printf("\n\tNo 'host_port' setting found in configuration file.\n"));
			return 0;
		}

		/* Read origin_peer_ip */
		const char* tmp0;
		if(config_setting_lookup_string(setting, "origin_peer_ip", (void *)&tmp0)){
			uint32_t ip;
			struct in_addr sin_addr;
			inet_pton(AF_INET, tmp0, &sin_addr); 
			ip = ntohl(sin_addr.s_addr);
			c->NETWORK_originPeerIP = ip;
			D(printf("\n\tOrigin peer IP: %s", tmp0));
		} else {
			D(printf("\n\tNo 'origin_peer_ip' set in configuration file.\n"));
			return 0;
		}
		/* Read origin_peer_port */
		if(config_setting_lookup_int(setting, "origin_peer_port", (int *)&tmp_int)){
			c->NETWORK_originPeerPort = (uint16_t)tmp_int;
			D(printf("\n\tOrigin peer port: %d", c->NETWORK_originPeerPort));
		} else {
			D(printf("\n\tNo 'origin_peer_port' found in configuration file.\n"));
			return 0;
		}

	}

	/* Read P2PDPRD protocol config */
	setting = config_lookup(&cfg, "proto_cfg");

	if(setting){ /* non-NULL result */

		/* Read client_id */
		if(config_setting_lookup_int(setting, "client_id", (int *)&tmp_int)){
			c->CLIENT_id = (uint32_t)tmp_int;			
			D(printf("\n\tClient ID: %d", c->CLIENT_id));
		} else {
            c->CLIENT_id = generateUniqueID();
			D(printf("\n\tClient ID: %d (random)", c->CLIENT_id));
		}

		/* Read client_timeout */
		if(config_setting_lookup_int(setting, "client_timeout", (int *)&tmp_int)){
			c->PROTO_timeout = (uint16_t)tmp_int;			
			D(printf("\n\tClient timeout: %d", c->PROTO_timeout));
		} else {
			D(printf("\n\tNo 'client_timeout' set in configuration file.\n\t"
									"Using default value of %d seconds.", CFG_DEFAULT_CLIENT_TIMEOUT));
			c->PROTO_timeout = CFG_DEFAULT_CLIENT_TIMEOUT;
		}

		/* Read client timeout variation */
		if(config_setting_lookup_int(setting, "client_timeout_variation", (int*)&c->PROTO_timeout_variation)){
			D(printf("\n\tClient timeout variation: %d", c->PROTO_timeout_variation));
		} else {
			D(printf("\n\tNo 'client_timeout_variation' set in configuration file.\n\t"
									"Using default value of %d seconds.", CFG_DEFAULT_CLIENT_TIMEOUT_VARIATION));
			c->PROTO_timeout_variation = CFG_DEFAULT_CLIENT_TIMEOUT_VARIATION;
		}

		/* Read latitude */
		if(config_setting_lookup_float(setting, "lat", &c->CLIENT_lat)){
			D(printf("\n\tClient latitude: %f", c->CLIENT_lat));
		} else {
			D(printf("\n\tNo 'lat' set in configuration file.\n\tUsing default value %f", CFG_DEFAULT_CLIENT_LAT));
			c->CLIENT_lat = CFG_DEFAULT_CLIENT_LAT;
		}

		/* Read longitude */
		if(config_setting_lookup_float(setting, "lon", &c->CLIENT_lon)){
			D(printf("\n\tClient longitude: %f", c->CLIENT_lon));
		} else {
			D(printf("\n\tNo 'lon' set in configuration file.\n\tUsing default value %f", CFG_DEFAULT_CLIENT_LON));
			c->CLIENT_lon = CFG_DEFAULT_CLIENT_LON;
		}

		/* Read coordination range */
		if(config_setting_lookup_int(setting, "coord_range", (int *)&tmp_int)){
			c->CLIENT_coordRange = (uint16_t)tmp_int;
			D(printf("\n\tClient coordination range: %d", c->CLIENT_coordRange));
		} else {
			D(printf("\n\tNo 'coord_range' set in configuration file.\n\t"
									"Using default value %d", CFG_DEFAULT_CLIENT_COORD_RANGE));
			c->CLIENT_coordRange = CFG_DEFAULT_CLIENT_COORD_RANGE;
		}

		/* Read node age limit */
		if(config_setting_lookup_int(setting, "node_max_age", (int *)&c->PROTO_nodeMaxAge)){
			D(printf("\n\tNode max age: %d", c->PROTO_nodeMaxAge));
		} else {
			D(printf("\n\tNo 'node_max_age' set in configuration file.\n\t"
									"Using default value %d", CFG_DEFAULT_NODE_AGE_LIMIT));
			c->PROTO_nodeMaxAge = CFG_DEFAULT_NODE_AGE_LIMIT;
		}

		/* Read protocol constant N */
		if(config_setting_lookup_int(setting, "proto_N", (int *)&tmp_int)){
			c->PROTO_N = (uint16_t)tmp_int;
			D(printf("\n\tProtocol constant N: %d", c->PROTO_N));
		} else {
			D(printf("\n\tNo 'proto_N' set in configuration file.\n\t"
									"Using default value %d", CFG_DEFAULT_P2PDPRD_CONSTANT_N));
			c->PROTO_N = CFG_DEFAULT_P2PDPRD_CONSTANT_N;
		}
		/* Read protocol constant M */
		if(config_setting_lookup_int(setting, "proto_M", (int *)&tmp_int)){
			c->PROTO_M = (uint16_t)tmp_int;
			D(printf("\n\tProtocol constant M: %d", c->PROTO_M));
		} else {
			D(printf("\n\tNo 'proto_M' set in configuration file.\n\t"
									"Using default value %d.", CFG_DEFAULT_P2PDPRD_CONSTANT_M));
			c->PROTO_M = CFG_DEFAULT_P2PDPRD_CONSTANT_M;
		}
		/* Read protocol constant K */
		if(config_setting_lookup_int(setting, "proto_K", (int *)&tmp_int)){
			c->PROTO_K = (uint16_t)tmp_int;
			D(printf("\n\tProtocol constant K: %d", c->PROTO_K));
		} else {
			D(printf("\n\tNo 'proto_K' set in configuration file.\n\t"
									"Using default value %d", CFG_DEFAULT_P2PDPRD_CONSTANT_K));
			c->PROTO_K = CFG_DEFAULT_P2PDPRD_CONSTANT_K;
		}

	}
	/* Read debug config */
	setting = config_lookup(&cfg, "deb_cfg");

	if(setting){ /* non-NULL result */
		const char* tmp;
		if(config_setting_lookup_string(setting, "logfile_path", (void *)&tmp)){
			strncpy(c->LOG_path, tmp, MAX_LOG_PATH_LENGTH);
			D(printf("\n\tLog file at: %s", c->LOG_path));
		} else {
			D(printf("\n\tNo 'logfile_path' set in configuration file.\n\t"
									"Writing log at %s", CFG_DEFAULT_LOG_PATH));
			strcpy(c->LOG_path, CFG_DEFAULT_LOG_PATH);
		}
	}

	/* Read local socket configuration */
	setting = config_lookup(&cfg, "local_service_cfg");

	if(setting){ /* non-NULL result */
		const char* tmp;
		if(config_setting_lookup_string(setting, "local_sock_path", (void *)&tmp)){
			strncpy(CONFIG->LOCAL_socketPath, tmp, MAX_SOCK_PATH_LENGTH);
			D(printf("\n\tLocal socket service at: %s", tmp));
		} else {
			D(printf("\n\tNo 'local_sock_path' set in configuration file.\n\t"
									"Using default path %s", CFG_DEFAULT_LOCAL_SOCK));
			strncpy(c->LOG_path, CFG_DEFAULT_LOCAL_SOCK, MAX_SOCK_PATH_LENGTH);
		}
	}

	/* Read radac-config */
	setting = config_lookup(&cfg, "radac_cfg");
	if(setting){
		/* Read radac_ip */
		const char* tmp;
		if(config_setting_lookup_string(setting, "radac_ip", (void *)&tmp)){
			uint32_t ip;
			inet_pton(AF_INET, tmp, &ip);	/* Convert ip string to network-byte-order unsigned int */
			ip = ntohl(ip);		/* Convert network ordered uint to host order  */
			c->RADAC_ip = ip;

			D(printf("\n\tRadac IP: %s", tmp));
		} else {
			D(printf("\n\tNo 'radac_ip' set in configuration file.\n"));
			return 0;
		}
		/* Read radac_port*/
		if(config_setting_lookup_int(setting, "radac_port", (int *)&tmp_int)){
			c->RADAC_port = (uint16_t)tmp_int;		
			D(printf("\n\tRadac port: %d", c->RADAC_port));
		} else {
			D(printf("\n\tNo 'radac_port' setting found in configuration file.\n"));
			return 0;
		}
	}

	D(printf("\n"));

	/* Clean */
	config_destroy(&cfg);
	return 1;
}

/*TODO Unused since rev. tagged "with_default_conf" */
/* Sets hard-coded default config*/
void Config_setToDefault(Config* cfg){
	/* Get and set default configuration from #defined constants */
	uint32_t p_ip;
	inet_pton(AF_INET, CFG_DEFAULT_PEER_IP, &p_ip);
	p_ip = ntohl(p_ip);
	cfg->NETWORK_originPeerIP = p_ip;
	cfg->NETWORK_originPeerPort = CFG_DEFAULT_PEER_PORT;
	cfg->NETWORK_port = CFG_DEFAULT_PORT;
    cfg->CLIENT_id = generateUniqueID();
	cfg->CLIENT_coordRange = CFG_DEFAULT_CLIENT_COORD_RANGE;
	cfg->CLIENT_lat = CFG_DEFAULT_CLIENT_LAT;
	cfg->CLIENT_lon = CFG_DEFAULT_CLIENT_LON;
	cfg->PROTO_nodeMaxAge = CFG_DEFAULT_NODE_AGE_LIMIT;
	cfg->PROTO_timeout = CFG_DEFAULT_CLIENT_TIMEOUT;
	cfg->PROTO_timeout_variation = CFG_DEFAULT_CLIENT_TIMEOUT_VARIATION;
	cfg->PROTO_N = CFG_DEFAULT_P2PDPRD_CONSTANT_N;
	cfg->PROTO_M = CFG_DEFAULT_P2PDPRD_CONSTANT_M;
	cfg->PROTO_K = CFG_DEFAULT_P2PDPRD_CONSTANT_K;
	inet_pton(AF_INET, CFG_DEFAULT_RADAC_IP, &p_ip);
	cfg->RADAC_ip = ntohl(p_ip);
	cfg->RADAC_port = CFG_DEFAULT_RADAC_PORT;
	strncpy(cfg->LOG_path, CFG_DEFAULT_LOG_PATH, MAX_LOG_PATH_LENGTH);
	strncpy(cfg->LOCAL_socketPath, CFG_DEFAULT_LOCAL_SOCK, MAX_SOCK_PATH_LENGTH);
	cfg->NETWORK_ownIP = getHostIPAddress();
}

void Config_set(int argc, char* argv, Config* cfg){
	/* Check if config-path parameter is set from command line */
	if(argc > 1){
		/* A file-path has been specified upon start. Make sure file exists */
		if(access(argv, R_OK) != 0){
			printf("Filepath %s is not valid. Using default config.\n"
				   "Usage: ./p2p-dprd /path/to/config.cfg\n", argv);
			Config_setToDefault(cfg);
		} else {
			D(printf("Found config file at %s\n", argv));
			if(!Config_readFromFile(argv, cfg)){
				printf("\nExiting...\n");
				exit(0);
			}
		}
	} else {
		D(printf("No config file specified.\n"
				 "Usage: ./p2p-dprd /path/to/config\n"));
		}
}


void Config_setNodePosition(double lat, double lon, Config* cfg){
	cfg->CLIENT_lat = lat;
	cfg->CLIENT_lon = lon;

	log_event(LOG_DEBUG, "Position has been updated - lat: %f, lon: %f ", lat, lon);
}

void Config_setNodeCoordinationRange(uint16_t coord_range, Config* cfg){
	cfg->CLIENT_coordRange = coord_range;
	log_event(LOG_DEBUG, "Coordination range has been updated:  %d", cfg->CLIENT_coordRange);
}

Config* Config_new(){
	return malloc(sizeof(Config));
}

void Config_destroy(Config* cfg){
	free(cfg);
}

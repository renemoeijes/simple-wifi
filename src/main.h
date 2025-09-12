// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 R. Moeijes

/** @file main.h
 * @brief Main program definitions and configuration structures
 * @author R. Moeijes
 * @date 2025
 * @copyright GPL v2+
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#define WIFI_CONFIG_AP_VERSION "0.1.0"

/* Configuration structure for Gavotte */
typedef struct {
    char *configfile;
    int debuglevel;
    int maxclients;
    char *gw_name;
    char *gw_interface;
    int gw_port;
    char *webroot;
    char *splashpage;
    int log_syslog;
    char *gw_address;
    char *gw_http_name;
    char *gw_http_name_port;
    char *gw_ip;
    char *gw_iprange;
    char *gw_domain;
    char *ssl_cert_file;
    char *ssl_key_file;
    int syslog_facility;
} s_config;

/* Function declarations */
s_config *config_get_config(void);

/* Config locking macros - simplified for single-threaded use */
#define LOCK_CONFIG() do {} while(0)
#define UNLOCK_CONFIG() do {} while(0)

#define MINIMUM_STARTED_TIME 1178487900 /* 2007-05-06 */

/** @brief exits cleanly and clear the firewall rules. */
void termination_handler(int s);


#endif /* _MAIN_H_ */

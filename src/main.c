// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 R. Moeijes

/** @file main.c
 * @brief wifi-config-ap main program and configuration
 * @author R. Moeijes
 * @date 2025  
 * @version 1.0.17
 * @copyright GPL v2+
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <microhttpd.h>

#include "main.h"
#include "http_server.h"

// Simple hardcoded config
static s_config config = {
    .configfile = "/etc/wifi-config-ap/wifi-config-ap.conf",
    .gw_name = "WiFi Setup Portal",
    .gw_interface = "wlan0", 
    .gw_port = 2050,
    .webroot = "/etc/wifi-config-ap/htdocs",
    .splashpage = "splash.html",
    .debuglevel = 0,
    .maxclients = 20,
    .log_syslog = 1,
    .gw_ip = "192.168.4.1",
    .gw_address = "192.168.4.1",
    .gw_http_name = "192.168.4.1",
    .gw_http_name_port = "192.168.4.1",
    .gw_iprange = "192.168.4.0/24",
    .gw_domain = NULL,
    .syslog_facility = LOG_DAEMON
};

static struct MHD_Daemon *webserver = NULL;

// Clean exit function
void termination_handler(int sig) {
    printf("Shutting down wifi-config-ap...\n");
    
    if (webserver) {
        MHD_stop_daemon(webserver);
    }
    
    exit(0);
}

// Config getter function
s_config *config_get_config(void) {
    return &config;
}

int main(int argc, char **argv) {
    // Handle version/help
    if (argc > 1) {
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
            printf("wifi-config-ap %s\n", WIFI_CONFIG_AP_VERSION);
            return 0;
        }
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printf("wifi-config-ap %s - WiFi Captive Portal\n", WIFI_CONFIG_AP_VERSION);
            printf("Usage: %s [-v|--version] [-h|--help]\n", argv[0]);
            return 0;
        }
    }
    
    printf("Starting wifi-config-ap %s...\n", WIFI_CONFIG_AP_VERSION);
    
    // Setup signal handlers for clean exit
    signal(SIGTERM, termination_handler);
    signal(SIGINT, termination_handler);  // Ctrl+C
    signal(SIGQUIT, termination_handler);
    signal(SIGALRM, termination_handler);  // Auto-exit after WiFi config
    
    // Start web server
    printf("Starting web server on port %d...\n", config.gw_port);
    webserver = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,  // Use threading
        config.gw_port,                   // Port
        NULL, NULL,                       // No connection restrictions
        libmicrohttpd_cb, NULL,          // Our request handler
        MHD_OPTION_END                   // End of options
    );
    
    if (!webserver) {
        printf("ERROR: Failed to start web server!\n");
        return 1;
    }
    
    printf("wifi-config-ap running! Press Ctrl+C to stop.\n");
    printf("Portal available at: http://%s/\n", config.gw_address);
    
    // HTTP daemon runs on its own threads - main() can just wait for termination signal
    // No infinite loops needed - just wait once for a signal and exit cleanly
    pause();  // Suspend until signal received, then termination_handler() will clean up and exit
    
    // This should never be reached (termination_handler calls exit())
    // But if it somehow does, clean up properly
    printf("Signal received, shutting down...\n");
    if (webserver) {
        MHD_stop_daemon(webserver);
    }
    
    // This line should never be reached
    return 0;
}

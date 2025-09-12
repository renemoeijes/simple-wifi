/** @file http_server.h
 * @brief HTTP server interface definitions
 * @author R. Moeijes
 * @date 2025
 * @copyright GPL v2+
 */

#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <stdio.h>
#include <microhttpd.h>

struct MHD_Connection;

/** @brief Get an IP's MAC address from the ARP cache.*/
int arp_get(char mac_addr[18], const char req_ip[]);


enum MHD_Result libmicrohttpd_cb (void *cls,
					struct MHD_Connection *connection,
					const char *url,
					const char *method,
					const char *version,
					const char *upload_data, size_t *upload_data_size, void **ptr);


#endif /* _HTTP_SERVER_H_ */

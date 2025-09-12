// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2025 R. Moeijes

/** @file http_server.c
 * @brief WiFi captive portal HTTP server using libmicrohttpd
 * @author R. Moeijes  
 * @date 2025
 * @version 1.0.17
 * @copyright GPL v2+
 */


#define _GNU_SOURCE

#include <microhttpd.h>
#include <stdint.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>

// #include "common.h" // No longer needed
#include "http_server.h"
#include "main.h"
// #include "mimetypes.h" // No longer needed
// #include "util.h" // No longer needed

#define QUERYMAXLEN 4096

/* Mimetypes struct and list */
struct mimetype {
    const char *extn;
    const char *mime;
};

static const struct mimetype uh_mime_types[] = {
    { "jpg", "image/jpeg" },
    { "jpeg", "image/jpeg" },
    { "gif", "image/gif" },
    { "png", "image/png" },
    { "svg", "image/svg+xml" },
    { "css", "text/css" },
    { "js", "application/javascript" },
    { "json", "application/json" },
    { "html", "text/html" },
    { "htm", "text/html" },
    { "ico", "image/x-icon" },
    { "txt", "text/plain" },
    { NULL, NULL }
};

/* Path normalization function - prevents directory traversal attacks */
static void buffer_path_simplify(char *dest, const char *src)
{
	/* current character, the one before, and the one before that from input */
	char c, pre1, pre2;
	char *start, *slash, *out;
	const char *walk;

	if (dest == NULL || src == NULL) return;

	if (strlen(src) == 0) {
		strcpy(dest, "");
		return;
	}

	walk  = src;
	start = dest;
	out   = dest;
	slash = dest;

	/* skip leading spaces */
	while (*walk == ' ') {
		walk++;
	}
	if (*walk == '.') {
		if (walk[1] == '/' || walk[1] == '\0')
			++walk;
		else if (walk[1] == '.' && (walk[2] == '/' || walk[2] == '\0'))
			walk+=2;
	}

	pre1 = 0;
	c = *(walk++);

	while (c != '\0') {
		pre2 = pre1;
		pre1 = c;

		c    = *walk;
		*out = pre1;

		out++;
		walk++;

		if (c == '/' || c == '\0') {
			const size_t toklen = out - slash;
			if (toklen == 3 && pre2 == '.' && pre1 == '.' && *slash == '/') {
				/* "/../" or ("/.." at end of string) */
				out = slash;
				/* if there is something before "/..", there is at least one
				 * component, which needs to be removed */
				if (out > start) {
					out--;
					while (out > start && *out != '/') out--;
				}

				/* don't kill trailing '/' at end of path */
				if (c == '\0') out++;
			} else if (toklen == 1 || (pre2 == '/' && pre1 == '.')) {
				/* "//" or "/./" or ("/" or "/.") at end of string) */
				out = slash;
				/* don't kill trailing '/' at end of path */
				if (c == '\0') out++;
			}

			slash = out;
		}
	}

	dest[out - start] = '\0';
}

/* Constants */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define DEFAULT_MIME_TYPE "application/octet-stream"

/* Connection info for POST data processing */
typedef struct {
    struct MHD_PostProcessor *postprocessor;
    char *ssid;
    char *password;
} connection_info_t;

/* Forward declarations */
static enum MHD_Result handle_request(struct MHD_Connection *connection, const char *url, 
                                     const char *method);
static enum MHD_Result handle_post_request(struct MHD_Connection *connection, 
                                          const char *upload_data, size_t *upload_data_size, 
                                          void **ptr);
static enum MHD_Result serve_splash_page(struct MHD_Connection *connection);
static enum MHD_Result serve_static_file(struct MHD_Connection *connection, const char *url);
static enum MHD_Result send_error_page(struct MHD_Connection *connection, int error_code);
static enum MHD_Result process_post_data(void *coninfo_cls, enum MHD_ValueKind kind, 
                                        const char *key, const char *filename, 
                                        const char *content_type, const char *transfer_encoding,
                                        const char *data, uint64_t off, size_t size);

static const char *get_file_extension(const char *filename);
static const char *get_mime_type(const char *filename);
static void save_wifi_config(const char *ssid, const char *password);

// static bool is_foreign_host(const char *host);
// static bool is_splash_page_request(const char *host, const char *url);
static const char *get_file_extension(const char *filename);
static const char *get_mime_type(const char *filename);
static void save_wifi_config(const char *ssid, const char *password);

/* URL encoding function 
static int url_encode(char *dest, size_t dest_size, const char *src, size_t src_len)
{
	const char *hex_chars = "0123456789ABCDEF";
	size_t dest_pos = 0;
	size_t i;

	for (i = 0; i < src_len && dest_pos < (dest_size - 4); i++) {
		unsigned char c = (unsigned char)src[i];
		
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
		    (c >= '0' && c <= '9') || c == '-' || c == '_' || 
		    c == '.' || c == '~') {
			dest[dest_pos++] = c;
		} else {
			dest[dest_pos++] = '%';
			dest[dest_pos++] = hex_chars[(c >> 4) & 0x0F];
			dest[dest_pos++] = hex_chars[c & 0x0F];
		}
	}
	
	if (dest_pos >= dest_size) {
		return -1;
	}
	
	dest[dest_pos] = '\0';
	return 0;
}
*/
/**
 * @brief Main HTTP request callback for libmicrohttpd
 */
enum MHD_Result libmicrohttpd_cb(void *cls, struct MHD_Connection *connection,
                                const char *_url, const char *method, const char *version,
                                const char *upload_data, size_t *upload_data_size, void **ptr)
{
	char url[PATH_MAX] = {0};

	/* Sanitize URL path */
	buffer_path_simplify(url, _url);
	printf("Request: %s %s\n", method, url);

	/* Handle POST requests */
	if (strcmp(method, "POST") == 0) {
		/* Only allow POST to /save endpoint */
		if (strcmp(url, "/save") == 0) {
			return handle_post_request(connection, upload_data, upload_data_size, ptr);
		} else {
			printf("POST to invalid endpoint: %s\n", url);
			return send_error_page(connection, 404);
		}
	}

	/* Only allow GET for other requests */
	if (strcmp(method, "GET") != 0) {
		printf("Unsupported HTTP method: %s\n", method);
		return send_error_page(connection, 503);
	}

	return handle_request(connection, url, method);
}

/**
 * @brief Handle GET requests
 */
static enum MHD_Result handle_request(struct MHD_Connection *connection, const char *url,
                                     const char *method)
{
	const char *ext = get_file_extension(url);

	/* If the request is for a specific file in our webroot (css, js, image), serve it. */
	if (ext && (strcmp(ext, "css") == 0 || strcmp(ext, "js") == 0 ||
	            strcmp(ext, "json") == 0 || strcmp(ext, "png") == 0 ||
	            strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0 ||
	            strcmp(ext, "gif") == 0 || strcmp(ext, "svg") == 0 ||
	            strcmp(ext, "ico") == 0)) {
		return serve_static_file(connection, url);
	}

	/* If the request is for /generate_204, send a 307 redirect to the splash page. */
	if (strcmp(url, "/generate_204") == 0) {
		const char *location = "http://192.168.4.1:2050/splash.html?redir=http%3A%2F%2Fconnectivitycheck.gstatic.com%2Fgenerate_204";
		const char *redirect_html = "<html><head></head><body><a href='http://192.168.4.1:2050/splash.html?redir=http%3A%2F%2Fconnectivitycheck.gstatic.com%2Fgenerate_204'>Click here to continue</a></body></html>";
	   struct MHD_Response *response = MHD_create_response_from_buffer(strlen(redirect_html), (void *)redirect_html, MHD_RESPMEM_PERSISTENT);
	   MHD_add_response_header(response, "Location", location);
	   return MHD_queue_response(connection, MHD_HTTP_TEMPORARY_REDIRECT, response);
	}

	/* For all other requests (e.g. /, or any other captive portal check), serve the main splash page directly with a 200 OK. */
	return serve_splash_page(connection);
}
/**
 * @brief Send error page
 */
static enum MHD_Result send_error_page(struct MHD_Connection *connection, int error_code)
{
	struct MHD_Response *response = NULL;
	const char *error_page = NULL;
	const char *mime_type;
	enum MHD_Result ret = MHD_NO;
	int http_status;

	/* Error pages */
	static const char *page_400 = "<html><head><title>Bad Request</title></head>"
	                              "<body><h1>400 - Bad Request</h1></body></html>";
	static const char *page_403 = "<html><head><title>Forbidden</title></head>"
	                              "<body><h1>403 - Forbidden</h1></body></html>";
	static const char *page_404 = "<html><head><title>Not Found</title></head>"
	                              "<body><h1>404 - Not Found</h1></body></html>";
	static const char *page_500 = "<html><head><title>Internal Server Error</title></head>"
	                              "<body><h1>500 - Internal Server Error</h1></body></html>";
	static const char *page_503 = "<html><head><title>Service Unavailable</title></head>"
	                              "<body><h1>503 - Service Unavailable</h1></body></html>";

	mime_type = get_mime_type("error.html");

	switch (error_code) {
	case 400:
		error_page = page_400;
		http_status = MHD_HTTP_BAD_REQUEST;
		break;
	case 403:
		error_page = page_403;
		http_status = MHD_HTTP_FORBIDDEN;
		break;
	case 404:
		error_page = page_404;
		http_status = MHD_HTTP_NOT_FOUND;
		break;
	case 500:
		error_page = page_500;
		http_status = MHD_HTTP_INTERNAL_SERVER_ERROR;
		break;
	case 503:
		error_page = page_503;
		http_status = MHD_HTTP_SERVICE_UNAVAILABLE;
		break;
	default:
		error_page = page_500;
		http_status = MHD_HTTP_INTERNAL_SERVER_ERROR;
		break;
	}

	response = MHD_create_response_from_buffer(strlen(error_page), 
	                                          (void *)error_page, MHD_RESPMEM_PERSISTENT);
	if (response) {
		MHD_add_response_header(response, "Content-Type", mime_type);
		ret = MHD_queue_response(connection, http_status, response);
		MHD_destroy_response(response);
	}

	return ret;
}

/**
 * @brief Serve splash page with template variables
 */
static enum MHD_Result serve_splash_page(struct MHD_Connection *connection)
{
	s_config *config = config_get_config();
	struct MHD_Response *response;
	char filename[PATH_MAX];
	const char *mime_type;
	char *file_content = NULL;
	struct stat stat_buf;
	int fd, bytes_read = 0, file_size;
	enum MHD_Result ret;

	/* Build filename */
	snprintf(filename, PATH_MAX, "%s/%s", config->webroot, config->splashpage);

	/* Check if file exists */
	if (stat(filename, &stat_buf) != 0 || !S_ISREG(stat_buf.st_mode)) {
		return send_error_page(connection, 404);
	}

	/* Open file */
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		return send_error_page(connection, 404);
	}

	/* Get file size */
	file_size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	/* Allocate memory for file content */
	file_content = calloc(file_size + 1, 1);
	if (!file_content) {
		close(fd);
		return send_error_page(connection, 503);
	}

	/* Read file */
	while (bytes_read < file_size) {
		int ret_read = read(fd, file_content + bytes_read, file_size - bytes_read);
		if (ret_read < 0) {
			free(file_content);
			close(fd);
			return send_error_page(connection, 503);
		}
		bytes_read += ret_read;
	}
	close(fd);

	/* Create response directly from file content */
	mime_type = get_mime_type(filename);
	response = MHD_create_response_from_buffer(file_size, 
	                                          file_content, MHD_RESPMEM_MUST_FREE);
	if (!response) {
		free(file_content);
		return send_error_page(connection, 503);
	}

	MHD_add_response_header(response, "Content-Type", mime_type);
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return ret;
}

/**
 * @brief Serve static files from webroot
 */
static enum MHD_Result serve_static_file(struct MHD_Connection *connection, const char *url)
{
	s_config *config = config_get_config();
	struct MHD_Response *response;
	struct stat stat_buf;
	char filename[PATH_MAX];
	const char *mime_type;
	int fd;
	off_t file_size;
	enum MHD_Result ret;

	/* Build full file path */
	snprintf(filename, PATH_MAX, "%s/%s", config->webroot, url);

	/* Check if file exists and is regular file */
	if (stat(filename, &stat_buf) != 0) {
		return send_error_page(connection, 404);
	}

	if (!S_ISREG(stat_buf.st_mode)) {
#ifdef S_ISLNK
		if (!S_ISLNK(stat_buf.st_mode))
#endif
		return send_error_page(connection, 404);
	}

	/* Open file */
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		return send_error_page(connection, 404);
	}

	/* Get file size */
	file_size = lseek(fd, 0, SEEK_END);
	if (file_size < 0) {
		close(fd);
		return send_error_page(connection, 404);
	}

	/* Create response from file descriptor */
	response = MHD_create_response_from_fd(file_size, fd);
	if (!response) {
		close(fd);
		return send_error_page(connection, 503);
	}

	/* Set content type */
	mime_type = get_mime_type(filename);
	MHD_add_response_header(response, "Content-Type", mime_type);

	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return ret;
}

/**
 * @brief Get file extension from filename
 */
static const char *get_file_extension(const char *filename)
{
	int pos = strlen(filename);
	
	while (pos > 0) {
		pos--;
		if (filename[pos] == '/') {
			return NULL;
		}
		if (filename[pos] == '.') {
			return &filename[pos + 1];
		}
	}
	return NULL;
}

/**
 * @brief Get MIME type for file
 */
static const char *get_mime_type(const char *filename)
{
	const char *extension;
	int i;

	if (!filename) {
		return DEFAULT_MIME_TYPE;
	}

	extension = get_file_extension(filename);
	if (!extension) {
		return DEFAULT_MIME_TYPE;
	}

	for (i = 0; uh_mime_types[i].extn != NULL; i++) {
		if (strcmp(extension, uh_mime_types[i].extn) == 0) {
			return uh_mime_types[i].mime;
		}
	}

	printf("Info: Unknown MIME type for extension: %s\n", extension);
	return DEFAULT_MIME_TYPE;
}

/**
 * @brief Handle POST requests
 */
static enum MHD_Result handle_post_request(struct MHD_Connection *connection, 
                                          const char *upload_data, size_t *upload_data_size, 
                                          void **ptr)
{
	connection_info_t *con_info = *ptr;

	if (con_info == NULL) {
		/* First call - initialize POST processor */
		con_info = malloc(sizeof(connection_info_t));
		if (!con_info) {
			return MHD_NO;
		}
		
		con_info->ssid = NULL;
		con_info->password = NULL;
		con_info->postprocessor = MHD_create_post_processor(connection, 1024, 
		                                                   process_post_data, con_info);
		
		if (!con_info->postprocessor) {
			free(con_info);
			return MHD_NO;
		}
		
		*ptr = con_info;
		return MHD_YES;
	}

	if (*upload_data_size != 0) {
		/* Process incoming POST data */
		MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
		*upload_data_size = 0;
		return MHD_YES;
	}

	/* POST data complete - process and respond */
	if (con_info->ssid && con_info->password) {
		save_wifi_config(con_info->ssid, con_info->password);
		printf("Info: WiFi configuration saved: SSID=%s\n", con_info->ssid);
	}

	/* Cleanup */
	if (con_info->ssid) free(con_info->ssid);
	if (con_info->password) free(con_info->password);
	MHD_destroy_post_processor(con_info->postprocessor);
	free(con_info);
	*ptr = NULL;

	/* Send success response */
	const char *success_page = "WiFi configuration saved successfully!";
	struct MHD_Response *response = MHD_create_response_from_buffer(
		strlen(success_page), (void *)success_page, MHD_RESPMEM_PERSISTENT);
	
	enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	
	/* Schedule exit after successful configuration */
	alarm(5);
	return ret;
}

/**
 * @brief Process POST data fields
 */
static enum MHD_Result process_post_data(void *coninfo_cls, enum MHD_ValueKind kind, 
                                        const char *key, const char *filename, 
                                        const char *content_type, const char *transfer_encoding,
                                        const char *data, uint64_t off, size_t size)
{
	connection_info_t *con_info = coninfo_cls;

	if (strcmp(key, "ssid") == 0) {
		if (con_info->ssid) {
			free(con_info->ssid);
		}
		con_info->ssid = strndup(data, size);
	} else if (strcmp(key, "password") == 0) {
		if (con_info->password) {
			free(con_info->password);
		}
		con_info->password = strndup(data, size);
	}

	return MHD_YES;
}

/**
 * @brief Save WiFi configuration to file
 */
static void save_wifi_config(const char *ssid, const char *password)
{
	FILE *file = fopen("/tmp/wifi-config.txt", "w");
	if (file) {
		fprintf(file, "%s\n%s\n", ssid, password);
		fclose(file);
		printf("Info: WiFi config written to /tmp/wifi-config.txt\n");
	} else {
		printf("Error: Failed to write WiFi config file\n");
	}
}

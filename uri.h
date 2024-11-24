#ifndef GFS_URI_H
#define GFS_URI_H

#include "config.h"
#include <stdbool.h>
#include <stdio.h>

typedef int errn;
typedef struct uri_s uri_t;

struct uri_s {
	char const *proto;
	char       *host;
	char       *port;
	char       *path;
	bool        is_directory;
	char        store[GFS_MAX_URI+GFS_MAX_PROTO+GFS_MAX_HOST+GFS_MAX_PORT+4];
	char        url[GFS_MAX_URI+1];
};

errn uri_from_path(uri_t *_uri, char const _path[], char const _proto[]);
errn uri_from_url(uri_t *_uri, char const _url[], uri_t const *_parent_uri);
void uri_printf(uri_t const *_uri, FILE *_out);

#endif

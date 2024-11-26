#ifndef GFS_URI_H
#define GFS_URI_H

#include <stdbool.h>
#include <stdio.h>

typedef int errn;
typedef struct uri_s uri_t;
typedef enum uri_e uri_e;

enum uri_e {
	URI_INVALID = -1,
	URI_GEMINI  =  1,
	URI_GOPHER  =  2,
};

struct uri_s {
	uri_e       proto;
	char const *host;
	char const *port;
	char const *path;
	char        gopher_type[2];
	bool        is_directory;
	char       *is_file;
	bool        is_default_port;
	char        m[1];
};

void  uri_free(uri_t *_uri);
errn  uri_from_path(uri_t **_uri, char const _path[], uri_e _proto);
errn  uri_from_url(uri_t **_uri, char const _url[], uri_t const *_parent_uri);
void  uri_printf(uri_t const *_uri, FILE *_out);
uri_e uri_protocol(char const _url[], uri_t const *_opt_parent_uri, char const **_opt_host);
char const *uri_protocol_name(uri_e proto);
int   uri_to_url(uri_t const *_uri, char _b[], size_t _bsz);

bool  uri_same_machine(uri_t const *_uri1, uri_t const *_uri2);
bool  uri_has_parent(uri_t const *_uri_child, uri_t const *_uri_parent, char const **_opt_base);

#endif

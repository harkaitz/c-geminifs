#include "uri.h"
#include <string.h>
#include <strings.h>
#include <syslog.h>
#include <stdlib.h>

/* Exceptions from C99 */
extern char *strtok_r(char *restrict, const char *restrict, char **restrict);

void
uri_free (uri_t *_uri)
{
	free(_uri);
}

errn
uri_from_path(uri_t **_uri, char const _path[], uri_e _proto)
{
	char *r, *colon, *s1;
	char const *default_port;
	size_t pathsz = strlen(_path), z;
	uri_t *uri = NULL;

	uri = calloc(1, sizeof(uri_t)+pathsz+1);
	if (!uri/*err*/) { syslog(LOG_ERR, "%s: Not enough memory.", _path); goto failure; } 

	uri->proto = _proto;
	switch (uri->proto) {
	case URI_GEMINI: uri->port = default_port = "1965"; break;
	case URI_GOPHER: uri->port = default_port = "70";   break;
	default: goto failure;
	}
	strcpy(uri->m, _path);

	uri->host = strtok_r(uri->m, "/", &r);
	if (!uri->host/*err*/) { syslog(LOG_ERR, "%s: No host in path (1).", _path); goto failure; }
	if ((colon = strchr(uri->host, ':'))) {
		*colon = '\0';
		uri->port = colon + 1;
	}
	uri->path = strtok_r(NULL, "", &r);
	if (!uri->path) {
		uri->path = "";
		uri->is_directory = true;
	}
	while (*(uri->path)=='/') {
		uri->path++;
	}

	if (!strncmp(uri->path, "../", 3)) {
		uri->path += 3;
	} else if (!strncmp(uri->path, "./", 2)) {
		uri->path += 2;
	}

	if (uri->proto == URI_GOPHER && !uri->gopher_type[0] && uri->path[0] && uri->path[1] == '/') {
		uri->gopher_type[0] = uri->path[0];
		uri->is_directory = (uri->path[0] == '1');
		uri->path += 2;
	}

	if (uri->proto != URI_GOPHER) {
		uri->is_file = NULL;
		if ((s1 = strrchr(uri->path, '/'))) {
			uri->is_file = strrchr(s1, '.');
		} else {
			uri->is_file = strrchr(uri->path, '.');
		}
		uri->is_directory = (!uri->is_file);
	}

	if ((r = strrchr(uri->path, '/'))) {
		if (*(r+1) == '\0') {
			*r = '\0';
		}
	}

	if ((z = strlen(uri->path))) {
		while ((--z) > 0 && uri->path[z] == '/') {
			((char*)uri->path)[z] = '\0';
		}
	}

	uri->is_default_port = (!strcmp(uri->port, default_port));

	*_uri = uri;
	return 0;
 failure:
	if (uri) free(uri);
	return -1;
}

errn
uri_from_url(uri_t **_uri, char const *_url, uri_t const *_puri)
{
	errn        err;
	char const *host;
	int         pos   = 0;
	uri_e       proto = uri_protocol(_url, _puri, &host);
	char       *path  = NULL;

	if (host) {
		pos = snprintf(NULL, 0, "/%s", host);
	} else if (_url[0] == '/' && _puri) {
		pos = snprintf(NULL, 0, "/%s:%s%s", _puri->host, _puri->port, _url);
	} else if (_puri) {
		pos = snprintf(NULL, 0, "/%s:%s/%s/%s", _puri->host, _puri->port, _puri->path, _url);
	} else {
		syslog(LOG_ERR, "Unsupported URL (2).");
		return -1;
	}
	path = malloc(pos+1);
	if (!path/*err*/) { syslog(LOG_ERR, "Not enough memory."); return -1; }
	
	if (host) {
		sprintf(path, "/%s", host);
	} else if (_url[0] == '/' && _puri) {
		sprintf(path, "/%s:%s/%s", _puri->host, _puri->port, _url);
	} else if (_puri) {
		sprintf(path, "/%s:%s/%s/%s", _puri->host, _puri->port, _puri->path, _url);
	}
	
	err = uri_from_path(_uri, path, proto);
	free(path);
	return err;
}

void
uri_printf(uri_t const *_uri, FILE *_out)
{
	char buffer[1024] = {0};
	uri_to_url(_uri, buffer, sizeof(buffer)-1);
	fprintf(_out, "URL: %s\n",   buffer);
	fprintf(_out, "PROTO: %s\n", uri_protocol_name(_uri->proto));
	fprintf(_out, "HOST: %s\n",  _uri->host);
	fprintf(_out, "PORT: %s\n",  _uri->port);
	fprintf(_out, "PATH: %s\n",  _uri->path);
	fprintf(_out, "IS_DIR: %d\n", _uri->is_directory);
	fprintf(_out, "\n");
}

uri_e
uri_protocol(char const _url[], uri_t const *_opt_parent_uri, char const **_opt_host)
{
	char const *host = NULL;
	uri_e       proto = URI_INVALID;
	if (!strncmp(_url, "//", 2)) {
		host = _url + 2;
		if (_opt_parent_uri) {
			proto = _opt_parent_uri->proto;
		} else {
			proto = URI_INVALID;
		}
	} else if ((host = strstr(_url, "://"))) {
		host += 3;
		if (!strncasecmp(_url, "gemini://", sizeof("gemini://")-1)) {
			proto = URI_GEMINI;
		} else if (!strncasecmp(_url, "gopher://", sizeof("gopher://")-1)) {
			proto = URI_GOPHER;
		} else if (_opt_parent_uri) {
			proto = URI_INVALID;
		}
	} else {
		proto = _opt_parent_uri->proto;
	}
	if (_opt_host) {
		*_opt_host = host;
	}
	return proto;
}

char const *
uri_protocol_name(uri_e proto)
{
	switch (proto) {
	case URI_INVALID: return "invalid";
	case URI_GEMINI:  return "gemini";
	case URI_GOPHER:  return "gopher";
	default: return "invalid";
	}
}

int
uri_to_url(uri_t const *_uri, char _b[], size_t _bsz)
{
	return snprintf(
	    _b,
	    _bsz,
	    "%s://" "%s%s%s" "%s%s" "%s%s" "%s",
	    uri_protocol_name(_uri->proto),
	    _uri->host, (_uri->is_default_port)?"":":", (_uri->is_default_port)?"":_uri->port,
	    (_uri->gopher_type[0])?"/":"", _uri->gopher_type,
	    (_uri->path[0])?"/":"", _uri->path,
	    (_uri->is_directory)?"/":""
	);
}

bool
uri_same_machine(uri_t const *_uri1, uri_t const *_uri2)
{
	return \
	    !strcasecmp(_uri1->host, _uri2->host) &&
	    !strcasecmp(_uri1->port, _uri2->port);
}

bool
uri_has_parent(uri_t const *_uri_child, uri_t const *_uri_parent, char const **_opt_base) {
	if (!uri_same_machine(_uri_child, _uri_parent)) return false;
	size_t s1 = strlen(_uri_parent->path);
	size_t s2 = strlen(_uri_child->path);
	if (s1 >= s2) return false;
	if (s1) {
		if (strncasecmp(_uri_parent->path, _uri_child->path, s1)) return false;
		if (_uri_child->path[s1] != '/') return false;
	}
	char const *shift = _uri_child->path + s1;
	while (*shift == '/') shift++;
	if (_opt_base) { *_opt_base = shift; }
	if (strchr(shift, '/')) return false;
	return true;
}

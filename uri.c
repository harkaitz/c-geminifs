#define _POSIX_C_SOURCE 200809L
#include "uri.h"
#include "err.h"
#include <string.h>
#include <strings.h>

err_t *
uri_from_path(uri_t *_uri, char const *_path, char const *_proto)
{
	int pos = 0;
	char path[GFS_MAX_PATH+1], *r, *l, *colon;
	char const *default_port;
	size_t pathsz;
	bool is_default_port;

	memset(_uri, 0, sizeof(*_uri));

	/* Check input validity. */
	pathsz = strlen(_path);
	if ((pathsz) >= GFS_MAX_PATH) {
		return err_new(-1, "%s: Unsupported path: Too large", _path);
	}
	if (!strcasecmp(_proto, "gemini")) {
		_uri->proto = "gemini";
		_uri->port = "1965";
		default_port = "1965";
	} else {
		return err_new(-1, "Unsupported protocol: %s", _path);
	}

	/* Fill host and path parts. */
	strcpy(path, _path);
	for (l = strtok_r(path, "/", &r); l; l = strtok_r(NULL, "/", &r)) {
		if (!_uri->host) {
			_uri->host = _uri->store+pos;
			pos += sprintf(_uri->store+pos, "%s", l);
			pos++;
			if ((colon = strchr(_uri->host, ':'))) {
				*colon = '\0';
				_uri->port = colon + 1;
			}
		} else if (!_uri->path) {
			if (!strcmp(l, "..") || !strcmp(l, ".")) {
				
			} else {
				_uri->path = _uri->store+pos;
				pos += sprintf(_uri->store+pos, "/%s", l);
				_uri->is_directory = (!strchr(l, '.'));
			}
		} else {
			pos += sprintf(_uri->store+pos, "/%s", l);
			_uri->is_directory = (!strchr(l, '.'));
		}
	}
	if (!_uri->path) {
		_uri->path = "";
		_uri->is_directory = true;
	}

	/* Fail if the host is not present. */
	if (!_uri->host/*err*/) { return err_new(-1, "%s: No host in path (1)", path); }

	/* Create url. */
	is_default_port = (!strcmp(_uri->port, default_port));
	sprintf(
	    _uri->url, "%s://%s%s%s%s%s",
	    _uri->proto, _uri->host,
	    (is_default_port)?"":":", (is_default_port)?"":_uri->port,
	    _uri->path,
	    (_uri->is_directory)?"/":""
	);
	return NULL;
}

err_t *
uri_from_url(uri_t *_uri, char const *_url, uri_t const *_puri)
{
	char const *rest;
	int  pos = 0;
	char path[GFS_MAX_PATH+1];
	if (_url[0] == '/' && _url[1] == '/') {
		rest = _url + 2;
	} else {
		rest = strstr(_url, "://");
		if (rest) rest += 3;
	}
	if (rest) {
		pos = snprintf(path, sizeof(path), "/%s", rest);
	} else if (_url[0] == '/') {
		pos = snprintf(path, sizeof(path), "/%s:%s%s", _puri->host, _puri->port, _url);
	} else {
		pos = snprintf(path, sizeof(path), "/%s:%s/%s/%s", _puri->host, _puri->port, _puri->path, _url);
	}
	if (pos >= sizeof(path)/*err*/) { return err_new(-1, "The url is too large"); }
	return uri_from_path(_uri, path, _puri->proto);
}

void
uri_printf(uri_t const *_uri, FILE *_out)
{
	fprintf(_out, "URI: %s\n",   _uri->url);
	fprintf(_out, "PROTO: %s\n", _uri->proto);
	fprintf(_out, "HOST: %s\n",  _uri->host);
	fprintf(_out, "PORT: %s\n",  _uri->port);
	fprintf(_out, "PATH: %s\n",  _uri->path);
	fprintf(_out, "IS_DIR: %d\n", _uri->is_directory);
}

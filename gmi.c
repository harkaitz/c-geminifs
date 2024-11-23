#define _POSIX_C_SOURCE 200809L
#include "gmi.h"
#include "cnx.h"
#include <string.h>

bool
gmi_getlink(char _gmi[], uri_t const *_parent, uri_t *_uri, char **_opt_desc, char **_r1)
{
	char *gmi = _gmi, *line, *url, *r2;
	err_t *err;

 recurse:
	line = strtok_r(gmi, "\n", _r1); gmi = NULL;
	if (!line) {
		return false;
	}

	if (line[0] != '=' || line[1] != '>') {
		goto recurse;
	}

	url = strtok_r(line+2, " \t\r\n", &r2);
	if (!url) {
		goto recurse;
	}

	if (_opt_desc) {
		char *name = strtok_r(NULL, "\r\n", &r2);
		while (name && strchr(" \t", *name)) { name++; }
		*_opt_desc = name;
	}

	err = uri_from_url(_uri, url, _parent);
	if (err) {
		free(err);
		err = NULL;
		goto recurse;
	}

	return true;
}


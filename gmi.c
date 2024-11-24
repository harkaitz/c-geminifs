#include "gmi.h"
#include "cnx.h"
#include <string.h>

/* Exceptions from C99 */
extern char *strtok_r(char *restrict, const char *restrict, char **restrict);

bool
gmi_getlink(char _gmi[], uri_t const *_parent, uri_t *_uri, char **_opt_desc, char **_r1)
{
	char *gmi = _gmi, *line, *url, *r2;

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

	if (uri_from_url(_uri, url, _parent)<0) {
		goto recurse;
	}

	return true;
}


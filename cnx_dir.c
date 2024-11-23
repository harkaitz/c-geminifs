#include "cnx_dir.h"
#include "cnx.h"
#include "gmi.h"
#include <string.h>
#include <strings.h>



err_t *
gfs_cnx_readdir(
    gfs_cnx_t  *_cnx,
    uri_t      *_uri,
    char      **_opt_name,
    bool        _start,
    bool       *_found,
    char      **_r,
    char        _d[],
    size_t      _dsz
) {
	err_t *err;
	char  *lst = (_start)?_d:NULL;
	size_t parent_uri_size  = strlen(_cnx->uri.url);
	size_t parent_path_size = strlen(_cnx->uri.path);

	if (_start) {
		size_t bytes;
		err = gfs_cnx_read(_cnx, 0, _d, _dsz-1, &bytes);
		if (err) { return err; }
		_d[bytes] = '\0';
	}

	if (!strcmp(_cnx->res_meta, "text/gemini")) {
		recurse_gmi:
		*_found = gmi_getlink(lst, &_cnx->uri, _uri, NULL, _r);
		lst = NULL;
		if (!*_found) return NULL;
	} else {
		return err_new(-1, "Unsupported index format");
	}

	if (parent_uri_size >= strlen(_uri->url)) goto recurse_gmi;
	if (strncasecmp(_cnx->uri.url, _uri->url, parent_uri_size)) goto recurse_gmi;
	if (strchr(_uri->path + parent_path_size + 1, '/')) goto recurse_gmi;
	if (!_uri->path[parent_path_size + 2]) goto recurse_gmi;
	if (_opt_name) *_opt_name = _uri->path + parent_path_size + 1;

	return NULL;
}


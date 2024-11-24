#include "cnx.h"
#include "gmi.h"
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

/* Exceptions from C99 */
extern char *strtok_r(char *restrict, const char *restrict, char **restrict);
extern int strncasecmp(const char s1[], const char s2[], size_t n);

int counter = 0;

errn
gfs_cnx_open_new(gfs_cnx_t **_cnx, char const _path[])
{
	errn             err;
	gfs_cnx_t       *cnx   = NULL;
	cnx = calloc(1, sizeof(gfs_cnx_t));
	if (!cnx/*err*/) { syslog(LOG_ERR, "Not enough memory."); return -1; }
	syslog(LOG_INFO, "Openning %s ...", _path);
	err = gfs_cnx_open(cnx, _path);
	if (err<0/*err*/) { free(cnx); return err; }
	*_cnx = cnx;
	syslog(LOG_INFO, "New descriptor: %p (count:%i)", cnx, ++counter);
	return 0;
}

void
gfs_cnx_free(gfs_cnx_t *_cnx)
{
	syslog(LOG_INFO, "Closing descriptor: %p (count:%i)", _cnx, --counter);
	gfs_cnx_reset(_cnx, false);
	free(_cnx);
}

void
gfs_cnx_reset(gfs_cnx_t *_cnx, bool _reset_uri)
{
	if (_cnx->mutexed) { pthread_mutex_lock(&_cnx->mutex); }
	if (_reset_uri) { memset(&_cnx->uri, 0, sizeof(_cnx->uri)); }
	_cnx->failed = false;
	_cnx->res_code = NULL;
	_cnx->res_meta = NULL;
	_cnx->rbufsz = 0;
	if (_cnx->dbuf) { free(_cnx->dbuf); _cnx->dbuf = NULL; }
	_cnx->dend = false;
	gfs_ssl_close(&_cnx->ssl);
	if (_cnx->mutexed) { pthread_mutex_destroy(&_cnx->mutex); _cnx->mutexed = false; }
}

errn
gfs_cnx_open(gfs_cnx_t *_cnx, char const _path[])
{
	int      err;
	size_t   pos, bytes;
	char    *r;

	err = uri_from_path(&_cnx->uri, _path, "gemini");
	if (err<0/*err*/) { goto failure; }

	err = gfs_ssl_connect(&_cnx->ssl, _cnx->uri.host, _cnx->uri.port);
	if (err<0/*err*/) { goto failure; }

	err = gfs_ssl_puts(&_cnx->ssl, _cnx->uri.url);
	if (err<0/*err*/) { goto failure; }
	err = gfs_ssl_puts(&_cnx->ssl, "\r\n");
	if (err<0/*err*/) { goto failure; }
	err = gfs_ssl_read(&_cnx->ssl, _cnx->rbuf, sizeof(_cnx->rbuf)-1, &bytes);
	if (err<0/*err*/) { goto failure; }
	
	for (pos=0; pos<bytes; pos++) {
		if (_cnx->rbuf[pos]=='\n') {
			_cnx->rbuf[pos] = '\0';
			pos++;
			break;
		}
	}
	if (pos==bytes/*err*/) { syslog(LOG_ERR, "Connection error: Invalid response (1)."); goto failure; }
	_cnx->res_code = strtok_r(_cnx->rbuf, " \r\n", &r);
	_cnx->res_meta = strtok_r(NULL, "\r\n", &r);
	if (!_cnx->res_code || !_cnx->res_meta/*err*/) { syslog(LOG_ERR, "Connection error: Invalid response (1)."); goto failure; }
	if (_cnx->res_code[0] != '2') {
		syslog(LOG_ERR, "Descriptor %p: Failed request: %s", _cnx, _cnx->res_code);
		goto failure;
	}

	if (pos<bytes) {
		_cnx->dbuf = malloc(bytes-pos);
		_cnx->dbufsz = bytes-pos;
		memcpy(_cnx->dbuf, _cnx->rbuf+pos, _cnx->dbufsz);
	}

	pthread_mutex_init(&_cnx->mutex, NULL);
	_cnx->mutexed = true;

	return 0;
	failure:
	gfs_cnx_reset(_cnx, false);
	return -1;
}

errn
gfs_cnx_read(gfs_cnx_t *_cnx, size_t _seek, char *_buf, size_t _size, size_t *_opt_read)
{
	size_t   new_pos, new_size;
	size_t   new_bytes;
	errn     err, ret = -1;

	pthread_mutex_lock(&_cnx->mutex);

	syslog(LOG_ERR, "Reading %p: seek=%li size=%li", _cnx, _seek, _size);
	if (_cnx->failed /*err*/) { syslog(LOG_ERR, "Connection failed"); goto cleanup; }

	new_pos = _seek + _size;
	if (!_cnx->dend && new_pos > _cnx->dbufsz) {
		if (!_cnx->dbuf) {
			_cnx->dbuf = malloc(new_pos);
			_cnx->dbufsz = 0;
		} else {
			_cnx->dbuf = realloc(_cnx->dbuf, new_pos);
		}
		if (!_cnx->dbuf/*err*/) { syslog(LOG_ERR, "Not enough memory"); goto cleanup; }
		err = gfs_ssl_read(&_cnx->ssl, _cnx->dbuf + _cnx->dbufsz, new_pos - _cnx->dbufsz, &new_bytes);
		if (err<0/*err*/) { _cnx->failed = true; goto cleanup; }
		_cnx->dbufsz += new_bytes;
		if (new_bytes < (new_pos - _cnx->dbufsz)) {
			_cnx->dend = true;
		}
	}

	if (_seek >= _cnx->dbufsz) {
		new_size = 0;
	} else if (new_pos > _cnx->dbufsz) {
		new_size = _cnx->dbufsz - _seek;
	} else {
		new_size = _size;
	}

	if (new_size) {
		memcpy(_buf, _cnx->dbuf + _seek, new_size);
	}
	if (_opt_read) { *_opt_read = new_size; }

	ret = 0;
	cleanup:
	pthread_mutex_unlock(&_cnx->mutex);

	return ret;
}

errn
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
	int      err;
	char    *lst = (_start)?_d:NULL;
	size_t   parent_uri_size  = strlen(_cnx->uri.url);
	size_t   parent_path_size = strlen(_cnx->uri.path);

	if (_start) {
		size_t bytes;
		err = gfs_cnx_read(_cnx, 0, _d, _dsz-1, &bytes);
		if (err<0/*err*/) { return -1; }
		_d[bytes] = '\0';
	}

	if (!strcmp(_cnx->res_meta, "text/gemini")) {
		recurse_gmi:
		*_found = gmi_getlink(lst, &_cnx->uri, _uri, NULL, _r);
		lst = NULL;
		if (!*_found) return 0;
	} else {
		syslog(LOG_ERR, "Unsupported index format");
		return -1;
	}

	if (parent_uri_size >= strlen(_uri->url)) goto recurse_gmi;
	if (strncasecmp(_cnx->uri.url, _uri->url, parent_uri_size)) goto recurse_gmi;
	if (strchr(_uri->path + parent_path_size + 1, '/')) goto recurse_gmi;
	if (!_uri->path[parent_path_size + 2]) goto recurse_gmi;
	if (_opt_name) *_opt_name = _uri->path + parent_path_size + 1;

	return 0;
}

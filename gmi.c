#include "gmi.h"
#include "gem.h"
#include "uri.h"
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#define GFS_MAX_GMI_SELECTOR 1024

/* Exceptions from C99 */
extern char *strtok_r(char *restrict, const char *restrict, char **restrict);
extern int strncasecmp(const char s1[], const char s2[], size_t n);
extern int strcasecmp(const char s1[], const char s2[]);

int counter = 0;

errn
gfs_cnx_open_new(gfs_cnx_t **_cnx, char const _path[], uri_e _protocol)
{
	errn             err;
	gfs_cnx_t       *cnx   = NULL;
	cnx = calloc(1, sizeof(gfs_cnx_t));
	if (!cnx/*err*/) { syslog(LOG_ERR, "Not enough memory."); return -1; }
	syslog(LOG_INFO, "Openning %s ...", _path);
	err = gfs_cnx_open(cnx, _path, _protocol);
	if (err<0/*err*/) { free(cnx); return err; }
	*_cnx = cnx;
	syslog(LOG_INFO, "Created: %p (count:%i)", cnx, ++counter);
	return 0;
}

void
gfs_cnx_free(gfs_cnx_t *_cnx)
{
	syslog(LOG_INFO, "Closing %p (count:%i)", _cnx, --counter);
	gfs_cnx_reset(_cnx);
	free(_cnx);
}

void
gfs_cnx_reset(gfs_cnx_t *_cnx)
{
	if (_cnx->mutexed) { pthread_mutex_lock(&_cnx->mutex); }
	if (_cnx->uri) { uri_free(_cnx->uri); _cnx->uri = NULL; } 
	_cnx->failed = false;
	_cnx->res_code = NULL;
	_cnx->res_meta = NULL;
	_cnx->rbufsz = 0;
	if (_cnx->dbuf) { free(_cnx->dbuf); _cnx->dbuf = NULL; }
	_cnx->dend = false;
	ssl_close(&_cnx->ssl);
	if (_cnx->mutexed) { pthread_mutex_destroy(&_cnx->mutex); _cnx->mutexed = false; }
}

errn
gfs_cnx_open(gfs_cnx_t *_cnx, char const _path[], uri_e _protocol)
{
	int      err, shift;
	size_t   pos, bytes;
	char    *r;
	char     selector[GFS_MAX_GMI_SELECTOR+3];

	err = uri_from_path(&_cnx->uri, _path, _protocol);
	if (err<0/*err*/) { goto failure; }

	if (_cnx->uri->proto == URI_GEMINI) {
		shift = uri_to_url(_cnx->uri, selector, sizeof(selector)-3);
		if (shift>=sizeof(selector)-3/*err*/) { syslog(LOG_ERR, "%s: Too large", _path); goto failure; }
		selector[shift++] = '\r';
		selector[shift++] = '\n';
		selector[shift] = '\0';
		syslog(LOG_INFO, "%p: Gemini Request: %s", _cnx, selector);

		err = ssl_connect(&_cnx->ssl, _cnx->uri->host, _cnx->uri->port);
		if (err<0/*err*/) { goto failure; }
		err = ssl_puts(&_cnx->ssl, selector);
		if (err<0/*err*/) { goto failure; }
		err = ssl_read(&_cnx->ssl, _cnx->rbuf, sizeof(_cnx->rbuf)-1, &bytes);
		if (err<0/*err*/) { goto failure; }

		for (pos=0; pos<bytes; pos++) {
			if (_cnx->rbuf[pos]=='\n') {
				_cnx->rbuf[pos] = '\0';
				pos++;
				break;
			}
		}
		if (pos==bytes/*err*/) { syslog(LOG_ERR, "Connection error: Invalid response (1)."); goto failure; }

		syslog(LOG_INFO, "%p: Gemini response: %s", _cnx, _cnx->rbuf);
		_cnx->res_code = strtok_r(_cnx->rbuf, " \r\n", &r);
		_cnx->res_meta = strtok_r(NULL, "\r\n", &r);
		if (!_cnx->res_code || !_cnx->res_meta/*err*/) { syslog(LOG_ERR, "Connection error: Invalid response (1)."); goto failure; }
		if (_cnx->res_code[0] != '2') {
			syslog(LOG_ERR, "%p: Gemini failed request: %s", _cnx, _cnx->res_code);
			goto failure;
		}

		if (pos<bytes) {
			_cnx->dbuf = malloc(bytes-pos);
			_cnx->dbufsz = bytes-pos;
			memcpy(_cnx->dbuf, _cnx->rbuf+pos, _cnx->dbufsz);
		}
	} else {
		syslog(LOG_ERR, "%p: Unsupported protocol.", _cnx);
		goto failure;
	}

	pthread_mutex_init(&_cnx->mutex, NULL);
	_cnx->mutexed = true;

	return 0;
	failure:
	gfs_cnx_reset(_cnx);
	return -1;
}

errn
gfs_cnx_read(gfs_cnx_t *_cnx, size_t _seek, char *_buf, size_t _size, size_t *_opt_read)
{
	size_t   new_pos, new_size;
	size_t   new_bytes;
	errn     err, ret = -1;

	pthread_mutex_lock(&_cnx->mutex);

	//syslog(LOG_INFO, "Reading %p: seek=%li size=%li", _cnx, _seek, _size);
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
		if (_cnx->ssl.tls) {
			err = ssl_read(&_cnx->ssl, _cnx->dbuf + _cnx->dbufsz, new_pos - _cnx->dbufsz, &new_bytes);
		} else {
			err = -1;
		}
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
    gfs_cnx_t   *_cnx,
    uri_t      **_uri,
    bool        *_found,
    char       **_r,
    char         _d[],
    size_t       _dsz
) {
	int      err;
	if (_d) {
		size_t bytes;
		err = gfs_cnx_read(_cnx, 0, _d, _dsz-1, &bytes);
		if (err<0/*err*/) { return -1; }
		_d[bytes] = '\0';
	}
	if (!strcmp(_cnx->res_meta, "text/gemini")) {
		while (1) {
			*_found = gem_getlink(_d, _cnx->uri, _uri, NULL, _r);
			_d = NULL;
			if (!*_found) {
				break;
			}
			break;
		}
	} else {
		syslog(LOG_ERR, "Unsupported index format");
		return -1;
	}

	return 0;
}

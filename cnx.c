#include "cnx.h"
#include "pool.h"
#include <string.h>

/* The only exception from C99 */
extern char *strtok_r(char *restrict, const char *restrict, char **restrict);

int counter = 0;

err_t *
gfs_cnx_open_new(gfs_cnx_t **_cnx, char const _path[])
{
	err_t      *err   = NULL;
	gfs_cnx_t  *cnx   = NULL;
	cnx = calloc(1, sizeof(gfs_cnx_t));
	assert(cnx);
	debugf("Openning %s ...", _path);
	err = gfs_cnx_open(cnx, _path);
	if (err) { free(cnx); return err; }
	*_cnx = cnx;
	debugf("New descriptor: %p (count:%i)", cnx, ++counter);
	return NULL;
}

void
gfs_cnx_free(gfs_cnx_t *_cnx)
{
	debugf("Closing descriptor: %p (count:%i)", _cnx, --counter);
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
	if (_cnx->fp[1]) { fclose(_cnx->fp[1]); _cnx->fp[1] = NULL; }
	gfs_wire_clean(&_cnx->wire);
	if (_cnx->fp[0]) { fclose(_cnx->fp[0]); _cnx->fp[0] = NULL; }
	if (_cnx->mutexed) { pthread_mutex_destroy(&_cnx->mutex); _cnx->mutexed = false; }
}

err_t *
gfs_cnx_open(gfs_cnx_t *_cnx, char const _path[])
{
	err_t      *err    = NULL;
	int         e;
	char       *line,*r;
	int         retries = 1;
	bool        flush = false;

	err = uri_from_path(&_cnx->uri, _path, "gemini");
	if (err) { goto failure; }

	connect:
	err = gfs_pool_get_wire(&_cnx->wire, "ssl", _cnx->uri.host, _cnx->uri.port, time(NULL), flush);
	if (err) { goto failure; }
	err = gfs_wire_fdopen(&_cnx->wire, _cnx->fp);
	if (err) { goto failure; }

	e = fprintf(_cnx->fp[1], "%s\r\n", _cnx->uri.url);
	if (e == -1/*err*/) { err = err_new(-1, "cnx:%p Failed writting", _cnx); goto retry; }
	fflush(_cnx->fp[1]);

	line = fgets(_cnx->rbuf, sizeof(_cnx->rbuf)-1, _cnx->fp[0]);
	if (!line/*err*/) { err = err_new(-1, "Failed reading response"); goto retry; }
	_cnx->res_code = strtok_r(_cnx->rbuf, " \r\n", &r);
	_cnx->res_meta = strtok_r(NULL, "\r\n", &r);
	if (!_cnx->res_code || !_cnx->res_meta/*err*/) { err = err_new(-1, "Invalid response"); goto failure; }
	fclose(_cnx->fp[1]);
	_cnx->fp[1] = NULL;

	if (_cnx->res_code[0] != '2') {
		err = err_new(-1, "Descriptor %p: Failed request: %s", _cnx, _cnx->res_code);
		goto failure;
	}

	pthread_mutex_init(&_cnx->mutex, NULL);
	_cnx->mutexed = true;

	return NULL;
	failure:
	gfs_cnx_reset(_cnx, false);
	return err;
	retry:
	if (retries) {
		gfs_cnx_reset(_cnx, false);
		retries--;
		flush = true;
		goto connect;
	} else {
		goto failure;
	}
}

err_t *
gfs_cnx_read(gfs_cnx_t *_cnx, size_t _seek, char *_buf, size_t _size, size_t *_opt_read)
{
	err_t   *err   = NULL;
	size_t   new_pos, new_size;
	size_t   new_bytes;

	pthread_mutex_lock(&_cnx->mutex);

	debugf("Reading %p: seek=%li size=%li", _cnx, _seek, _size);
	if (_cnx->failed /*err*/) { err = err_new(-1, "Connection failed"); goto cleanup; }

	new_pos = _seek + _size;
	if (!_cnx->dend && new_pos > _cnx->dbufsz) {
		if (!_cnx->dbuf) {
			_cnx->dbuf = malloc(new_pos);
			_cnx->dbufsz = 0;
		} else {
			_cnx->dbuf = realloc(_cnx->dbuf, new_pos);
		}
		assert(_cnx->dbuf);
		new_bytes = fread(_cnx->dbuf + _cnx->dbufsz, 1, new_pos - _cnx->dbufsz, _cnx->fp[0]);
		_cnx->dbufsz += new_bytes;
		if (new_bytes < (new_pos - _cnx->dbufsz)) {
			_cnx->dend = true;
			err = gfs_wire_wait(&_cnx->wire);
			if (err) { _cnx->failed = true; goto cleanup; }
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

	cleanup:
	pthread_mutex_unlock(&_cnx->mutex);

	return err;
}


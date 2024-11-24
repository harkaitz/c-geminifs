#include "ssl.h"
#include <tls.h>
#include <syslog.h>
#include <string.h>

errn
gfs_ssl_init(void)
{
	int err;
	err = tls_init();
	if (err<0/*err*/) { syslog(LOG_ERR, "Internal error: TLS initialization."); return -1; }
	return 0;
}

errn
gfs_ssl_connect(gfs_ssl_t *_ssl, cstr _host, cstr _port)
{
	int err;

	_ssl->tls = NULL;
	_ssl->config = tls_config_new();
	if (!_ssl->config/*err*/) { syslog(LOG_ERR, "Internal error: TLS configuration."); goto cleanup; }
	tls_config_insecure_noverifycert(_ssl->config);
	tls_config_insecure_noverifyname(_ssl->config);
	tls_config_set_ciphers(_ssl->config, "compat");

	_ssl->tls = tls_client();
	if (!_ssl->tls/*err*/) { syslog(LOG_ERR, "Internal error: tls_client."); goto cleanup; }
	err = tls_configure(_ssl->tls, _ssl->config);
	if (err<0/*err*/) {  syslog(LOG_ERR, "Internal error: tls_configure."); goto cleanup; }
	err = tls_connect(_ssl->tls, _host, _port);
	if (err<0/*err*/) { syslog(LOG_ERR, "Connection error: %s.", tls_error(_ssl->tls)); goto cleanup; }

	return 0;
	cleanup:
	if (_ssl->tls) { tls_close(_ssl->tls); tls_free(_ssl->tls); }
	if (_ssl->config) { tls_config_free(_ssl->config); }
	return -1;
}

errn
gfs_ssl_read(gfs_ssl_t *_ssl, char _b[], size_t _bsz, size_t *_opt_bytes)
{
	size_t   pos = 0;
	ssize_t  ret;
	while (1) {
		ret = tls_read(_ssl->tls, _b+pos, _bsz-pos);
		if (ret == -2) {
			continue;
		}
		if (ret == -1) {
			syslog(LOG_ERR, "Connection error: %p: %s.", _ssl, tls_error(_ssl->tls));
			return -1;
		}
		if (ret == 0) {
			break;
		}
		pos += ret;
		if (pos == _bsz) {
			break;
		}
	}
	if (_opt_bytes) *_opt_bytes = pos;
	return 0;
}

errn
gfs_ssl_write(gfs_ssl_t *_ssl, char const _b[], size_t _bsz)
{
	size_t   pos = 0;
	ssize_t  ret;
	while (1) {
		ret = tls_write(_ssl->tls, _b+pos, _bsz-pos);
		if (ret == -2) {
			continue;
		}
		if (ret == -1 || ret == 0) {
			syslog(LOG_ERR, "Connection error: %p: %s.", _ssl, tls_error(_ssl->tls));
			return -1;
		}
		pos += ret;
		if (pos == _bsz) {
			break;
		}
	}
	return 0;
}

errn
gfs_ssl_puts(gfs_ssl_t *_ssl, char const _b[])
{
	return gfs_ssl_write(_ssl, _b, strlen(_b));
}

void
gfs_ssl_close(gfs_ssl_t *_ssl)
{
	if (_ssl->tls) { tls_close(_ssl->tls); tls_free(_ssl->tls); }
	if (_ssl->config) { tls_config_free(_ssl->config); }
	_ssl->tls = NULL;
	_ssl->config = NULL;
}

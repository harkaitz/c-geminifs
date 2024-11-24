#include "ssl.h"
#include <tls.h>
#include <syslog.h>
#include <string.h>
#ifdef DEBUG
#  define debugf(...) syslog(LOG_DEBUG, "ssl: " __VA_ARGS__)
#else
#  define debugf(...) ({})
#endif
#define errorf(...) syslog(LOG_ERR, "ssl: " __VA_ARGS__)

errn
ssl_init(void)
{
	int err;
	err = tls_init();
	if (err<0/*err*/) { errorf("Internal error: TLS initialization."); return -1; }
	return 0;
}

errn
ssl_connect(ssl_t *_ssl, char const _host[], char const _port[])
{
	int err;

	_ssl->tls = NULL;
	_ssl->config = tls_config_new();
	if (!_ssl->config/*err*/) { errorf("Internal error: TLS configuration."); goto cleanup; }
	tls_config_insecure_noverifycert(_ssl->config);
	//tls_config_insecure_noverifyname(_ssl->config);
	tls_config_set_ciphers(_ssl->config, "compat");

	_ssl->tls = tls_client();
	if (!_ssl->tls/*err*/) { errorf("Internal error: tls_client."); goto cleanup; }
	err = tls_configure(_ssl->tls, _ssl->config);
	if (err<0/*err*/) {  errorf("Internal error: tls_configure."); goto cleanup; }
	err = tls_connect(_ssl->tls, _host, _port);
	if (err<0/*err*/) { errorf("Connection error: %s.", tls_error(_ssl->tls)); goto cleanup; }

	while(1) {
		err = tls_handshake(_ssl->tls);
		if (err == TLS_WANT_POLLIN || err == TLS_WANT_POLLOUT || err == -2)
			continue;
		break;
	}

	return 0;
	cleanup:
	if (_ssl->tls) { tls_close(_ssl->tls); tls_free(_ssl->tls); }
	if (_ssl->config) { tls_config_free(_ssl->config); }
	return -1;
}

errn
ssl_read(ssl_t *_ssl, char _b[], size_t _bsz, size_t *_opt_bytes)
{
	size_t   pos = 0;
	ssize_t  ret;
	while (1) {
		ret = tls_read(_ssl->tls, _b+pos, _bsz-pos);
		if (ret == TLS_WANT_POLLIN) {
			continue;
		}
		if (ret == -1) {
			errorf("Connection error: %p: %s.", _ssl, tls_error(_ssl->tls));
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
ssl_write(ssl_t *_ssl, char const _b[], size_t _bsz)
{
	size_t   pos = 0;
	ssize_t  ret;
	while (1) {
		ret = tls_write(_ssl->tls, _b+pos, _bsz-pos);
		if (ret == -2) {
			continue;
		}
		if (ret == -1 || ret == 0) {
			errorf("Connection error: %p: %s.", _ssl, tls_error(_ssl->tls));
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
ssl_puts(ssl_t *_ssl, char const _b[])
{
	return ssl_write(_ssl, _b, strlen(_b));
}

void
ssl_close(ssl_t *_ssl)
{
	if (_ssl->tls) { tls_close(_ssl->tls); tls_free(_ssl->tls); }
	if (_ssl->config) { tls_config_free(_ssl->config); }
	_ssl->tls = NULL;
	_ssl->config = NULL;
}

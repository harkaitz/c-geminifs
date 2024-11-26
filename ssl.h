#ifndef GFS_SSL_H
#define GFS_SSL_H

#include <stddef.h>

typedef int errn;
struct tls;
struct tls_config;

typedef struct ssl_s ssl_t;

struct ssl_s {
	struct tls_config *config;
	struct tls        *tls;
};

extern errn ssl_init(void);
extern errn ssl_connect(ssl_t *_ssl, char const _host[], char const _port[]);
extern errn ssl_read(ssl_t *_ssl, char _b[], size_t _bsz, size_t *_opt_bytes);
extern errn ssl_write(ssl_t *_ssl, char const _b[], size_t _bsz);
extern errn ssl_puts(ssl_t *_ssl, char const _b[]);
extern void ssl_close(ssl_t *_ssl);

#endif

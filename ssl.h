#ifndef GFS_SSL_H
#define GFS_SSL_H

#include <stddef.h>

typedef int errn;
struct tls;
struct tls_config;
typedef char const *cstr;

typedef struct gfs_ssl_s gfs_ssl_t;

struct gfs_ssl_s {
	struct tls_config *config;
	struct tls        *tls;
};

extern errn gfs_ssl_init(void);
extern errn gfs_ssl_connect(gfs_ssl_t *_ssl, cstr _host, cstr _port);
extern errn gfs_ssl_read(gfs_ssl_t *_ssl, char _b[], size_t _bsz, size_t *_opt_bytes);
extern errn gfs_ssl_write(gfs_ssl_t *_ssl, char const _b[], size_t _bsz);
extern errn gfs_ssl_puts(gfs_ssl_t *_ssl, char const _b[]);
extern void gfs_ssl_close(gfs_ssl_t *_ssl);

#endif

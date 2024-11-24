#ifndef GFS_POOL_H
#define GFS_POOL_H

typedef int errn;
typedef struct ssl_s ssl_t;

extern errn pool_ssl_get(ssl_t *_ssl, char const _host[], char const _port[]);

#endif

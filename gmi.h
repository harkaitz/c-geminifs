#ifndef GFS_CNX_H
#define GFS_CNX_H

#include "ssl.h"
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct gfs_cnx_s gfs_cnx_t;
typedef struct uri_s uri_t;
typedef enum uri_e uri_e;

struct gfs_cnx_s {
	pthread_mutex_t mutex;
	bool            mutexed;
	uri_t          *uri;
	bool            failed;
	char const     *res_code;
	char const     *res_meta;
	ssl_t           ssl;
	char            rbuf[1024];
	char            rbufsz;
	char           *dbuf;
	size_t          dbufsz;
	bool            dend;
};

errn gfs_cnx_open_new(gfs_cnx_t **_cnx, char const _path[], uri_e _protocol);
void gfs_cnx_free(gfs_cnx_t *_cnx);

void gfs_cnx_reset(gfs_cnx_t *_cnx);
errn gfs_cnx_open(gfs_cnx_t *_cnx, char const _path[], uri_e _protocol);
errn gfs_cnx_read(gfs_cnx_t *_cnx, size_t _seek, char *_buf, size_t _size, size_t *_opt_read);

errn gfs_cnx_readdir(
    gfs_cnx_t   *_cnx,
    uri_t      **_uri,
    bool        *_found,
    /* Auxiliary. */
    char       **_r,
    char         _d[],
    size_t       _dsz
);


#endif

#ifndef GFS_CNX_H
#define GFS_CNX_H

#include "uri.h"
#include "ssl.h"
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct gfs_cnx_s gfs_cnx_t;

struct gfs_cnx_s {
	pthread_mutex_t mutex;
	bool            mutexed;
	uri_t           uri;
	bool            failed;
	char const     *res_code;
	char const     *res_meta;
	gfs_ssl_t       ssl;
	char            rbuf[1024];
	char            rbufsz;
	char           *dbuf;
	size_t          dbufsz;
	bool            dend;
};

errn gfs_cnx_open_new(gfs_cnx_t **_cnx, char const _path[]);
void gfs_cnx_free(gfs_cnx_t *_cnx);

void gfs_cnx_reset(gfs_cnx_t *_cnx, bool _reset_uri);
errn gfs_cnx_open(gfs_cnx_t *_cnx, char const _path[]);
errn gfs_cnx_read(gfs_cnx_t *_cnx, size_t _seek, char *_buf, size_t _size, size_t *_opt_read);

errn gfs_cnx_readdir(
    gfs_cnx_t  *_cnx,
    uri_t      *_uri,
    char      **_opt_name,
    bool        _start,
    bool       *_found,
    /* Auxiliary. */
    char      **_r,
    char        _d[],
    size_t      _dsz
);


#endif

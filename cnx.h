#ifndef GFS_CNX_H
#define GFS_CNX_H

#include "err.h"
#include "uri.h"
#include "pool.h"
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
	gfs_wire_t      wire;
	FILE           *fp[2];
	char            rbuf[1024];
	char            rbufsz;
	char           *dbuf;
	size_t          dbufsz;
	bool            dend;
};

err_t *gfs_cnx_open_new(gfs_cnx_t **_cnx, char const _path[]);
void   gfs_cnx_free(gfs_cnx_t *_cnx);

void   gfs_cnx_reset(gfs_cnx_t *_cnx, bool _reset_uri);
err_t *gfs_cnx_open(gfs_cnx_t *_cnx, char const _path[]);
err_t *gfs_cnx_read(gfs_cnx_t *_cnx, size_t _seek, char *_buf, size_t _size, size_t *_opt_read);

#endif

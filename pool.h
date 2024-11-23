#ifndef GFS_POOL_H
#define GFS_POOL_H

#include <time.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct gfs_wire_s gfs_wire_t;
typedef char const *cstr;
typedef struct err_s err_t;

struct gfs_wire_s {
	int    pid;
	int    p[2];
};

extern err_t *gfs_pool_init(int _opt_size, int _opt_age, int _opt_connections);

extern err_t *gfs_pool_get_wire(gfs_wire_t *_wire, cstr _proto, cstr _host, cstr _port, time_t _time, bool _flush);
extern err_t *gfs_pool_new_wire(gfs_wire_t *_wire, cstr _proto, cstr _host, cstr _port);

extern err_t *gfs_wire_fdopen(gfs_wire_t *_wire, FILE *_fp[2]);
extern err_t *gfs_wire_wait(gfs_wire_t *_wire);

extern void   gfs_wire_clean(gfs_wire_t *_wire);

#endif

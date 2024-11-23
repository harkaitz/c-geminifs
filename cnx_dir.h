#ifndef GMNI_CNX_DIR_H
#define GMNI_CNX_DIR_H

#include <stdbool.h>
#include <stddef.h>

typedef struct err_s err_t;
typedef struct gfs_cnx_s gfs_cnx_t;
typedef struct uri_s uri_t;

extern err_t *gfs_cnx_readdir(
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

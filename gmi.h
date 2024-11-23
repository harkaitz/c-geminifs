#ifndef GFS_GMI_H
#define GFS_GMI_H

#include <stdio.h>
#include <stdbool.h>

typedef struct uri_s uri_t;

extern bool gmi_getlink(char _gmi[], uri_t const *_parent, uri_t *_uri, char **_opt_name, char **_r);

#endif

#ifndef GFS_GEM_H
#define GFS_GEM_H

#include <stdio.h>
#include <stdbool.h>

typedef struct uri_s uri_t;

extern bool gem_getlink(char _gmi[], uri_t const *_parent, uri_t **_uri, char const **_opt_name, char **_r);

#endif

#ifndef ERR_OBJ_H
#define ERR_OBJ_H

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>

typedef struct err_s {
	char message[1024];
} err_t;

static inline err_t *
err_new(int _code, char const *_fmt, ...)
{
	va_list ap;
	err_t *err  = malloc(sizeof(err_t));
	assert(err);
	va_start(ap, _fmt);
	vsnprintf(err->message, sizeof(err->message), _fmt, ap);
	va_end(ap);
	return err;
}

static inline void
err_print(err_t const *_err)
{
	fputs(_err->message, stderr);
	fputs("\n", stderr);
	fflush(stderr);
}

static inline void
debugf(char const *_fmt, ...)
{
	#ifdef DEBUG
	va_list ap;
	va_start(ap, _fmt);
	vfprintf(stderr, _fmt, ap);
	fputc('\n', stderr);
	fflush(stderr);
	va_end(ap);
	#endif
}


#endif

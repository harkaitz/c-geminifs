#include "pool.h"
#include "ssl.h"
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#ifdef DEBUG
#  define debugf(...) syslog(LOG_DEBUG, "pool: " __VA_ARGS__)
#else
#  define debugf(...) ({})
#endif


extern int strcasecmp(const char s1[], const char s2[]);

typedef struct pool_s pool_t;

struct pool_s {
	bool      locked;
	bool      usable;
	time_t    created;
	char      host[256];
	char      port[10];
	ssl_t     ssl;
};

pool_t pools[20] = {0};
size_t poolsz = 20;
time_t pool_timeout = 6;
size_t pool_count = 4;

pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;


static bool
pool_match(pool_t *_pool, char const _host[], char const _port[])
{
	if (strcasecmp(_pool->host, _host)) return false;
	if (strcasecmp(_pool->port, _port)) return false;
	return true;
}

static void *
pool_thread(void *_arg)
{
	pool_t *pool = _arg;
	if (ssl_connect(&pool->ssl, pool->host, pool->port)>=0) {
		pthread_mutex_lock(&pool_mutex);
		pool->usable = true;
		pool->locked = false;
		pthread_mutex_unlock(&pool_mutex);
	} else {
		pthread_mutex_lock(&pool_mutex);
		memset(pool, 0, sizeof(pool_t));
		pthread_mutex_unlock(&pool_mutex);
	}
	return NULL;
}

errn
pool_ssl_get(ssl_t *_ssl, char const _host[], char const _port[])
{
	bool    found = false;
	time_t  now  = time(NULL);

	if (strlen(_host)>=256 || strlen(_port)>=10) {
		debugf("Creating new connection to %s:%s", _host, _port);
		return ssl_connect(_ssl, _host, _port);
	}

	pthread_mutex_lock(&pool_mutex);

	size_t matches = 0;
	for (size_t i=0; i<poolsz; i++) {
		if (pools[i].locked)  continue;
		
		if (pools[i].usable && (now-pools[i].created)>pool_timeout) {
			ssl_close(&pools[i].ssl);
			memset(&pools[i], 0, sizeof(pool_t));
			continue;
		}
		if (pools[i].usable && pool_match(&pools[i], _host, _port)) {
			if (!found) {
				debugf("Reusing %s:%s", pools[i].host, pools[i].port);
				memcpy(_ssl, &pools[i].ssl, sizeof(ssl_t));
				memset(&pools[i], 0, sizeof(pool_t));
				found = true;
			} else {
				matches++;
			}
		}
	}

	size_t to_create = (pool_count>matches)?pool_count - matches:0;
	for (size_t i=0; to_create > 0 && i<poolsz; i++) {
		if (pools[i].locked) continue;
		
		if (!pools[i].usable) {
			pools[i].locked = true;
			pools[i].usable = false;
			pools[i].created = now;
			strcpy(pools[i].host, _host);
			strcpy(pools[i].port, _port);
			pthread_t thread;
			pthread_create(&thread, NULL, pool_thread, &pools[i]);
			pthread_detach(thread);
			to_create--;
		}
	}

	for (size_t i=0; i<poolsz; i++) {
		if (!pools[i].locked && pools[i].usable) {
			debugf("State: %s:%s", pools[i].host, pools[i].port);
		}
	}

	pthread_mutex_unlock(&pool_mutex);
	if (found) return 0;

	debugf("Creating new connection to %s:%s", _host, _port);
	return ssl_connect(_ssl, _host, _port);
}

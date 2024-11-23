#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "pool.h"
#include "err.h"
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

typedef struct gfs_pool_s {
	time_t     creation;
	char       proto[GFS_MAX_PROTO+1];
	char       host[GFS_MAX_HOST+1];
	char       port[GFS_MAX_PORT+1];
	gfs_wire_t wire;
} gfs_pool_t;

gfs_pool_t      pool[GFS_MAX_POOL_SIZE] = {0};
size_t          poolsz = GFS_DEFAULT_POOL_SIZE;
int             pool_seconds = GFS_DEFAULT_POOL_AGE;
int             pool_connections = GFS_DEFAULT_POOL_CNX;
pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;

err_t *
gfs_pool_init(int _opt_size, int _opt_age, int _opt_connections)
{
	if (_opt_size > GFS_MAX_POOL_SIZE) { return err_new(-1, "Invalid pool size"); }
	if (_opt_size > 0) { poolsz = _opt_size; }
	if (_opt_age  > 0) { pool_seconds = _opt_age; }
	if (_opt_connections > 0) { pool_connections = _opt_connections; }
	signal(SIGPIPE, SIG_IGN);
	return NULL;
}

bool
gfs_pool_match(gfs_pool_t *_pool, cstr _proto, cstr _host, cstr _port)
{
	return !strcmp(_pool->proto, _proto) &&
	       !strcmp(_pool->host,  _host)  &&
	       !strcmp(_pool->port,  _port);
}

err_t *
gfs_pool_get_wire(gfs_wire_t *_wire, cstr _proto, cstr _host, cstr _port, time_t _time, bool _flush)
{
	err_t *err   = NULL;

	pthread_mutex_lock(&pool_mutex);
	
	/* Close those older that 60 seconds, search a connection, count. */
	int openned_connections = 0;
	int found_connection = -1;
	for (size_t i = 0; i < poolsz; i++) {
		if (!pool[i].creation) continue;
		if ((pool[i].creation + pool_seconds) < _time) {
			gfs_wire_clean(&pool[i].wire);
			pool[i].creation = 0;
			continue;
		}
		if (gfs_pool_match(&pool[i], _proto, _host, _port)) {
			if (_flush) {
				gfs_wire_clean(&pool[i].wire);
				pool[i].creation = 0;
				continue;
			}
			if (found_connection == -1) {
				found_connection = i;
			}
			openned_connections++;
		}
	}

	/* If found open 2 new connections. Otherwise 3. */
	int connections_to_open = ((found_connection == -1)? pool_connections : pool_connections-1) - openned_connections;
	for (size_t i = 0; i < poolsz && connections_to_open > 0; i++) {
		if (pool[i].creation) { continue; }
		err = gfs_pool_new_wire(&pool[i].wire, _proto, _host, _port);
		if (err) { goto cleanup; }
		pool[i].creation = _time;
		strncpy(pool[i].proto, _proto, sizeof(pool[i].proto)-1);
		strncpy(pool[i].host,  _host,  sizeof(pool[i].host)-1);
		strncpy(pool[i].port,  _port,  sizeof(pool[i].port)-1);
		connections_to_open--;
	}
	debugf("== Pool status ==");
	for (size_t i = 0; i < poolsz; i++) {
		if (!pool[i].creation) continue;
		debugf("Connection %i: %s://%s:%s", i, pool[i].proto, pool[i].host, pool[i].port);
	}
	debugf("== End of pool status ==");

	/* Open new connection if not found. */
	if (found_connection == -1) {
		debugf("Opening new connection ...");
		err = gfs_pool_new_wire(_wire, _proto, _host, _port);
		if (err) { goto cleanup; }
	} else {
		debugf("Reusing connection %i ...", found_connection);
		memcpy(_wire, &pool[found_connection].wire, sizeof(gfs_wire_t));
		pool[found_connection].creation = 0;
	}

	cleanup:
	pthread_mutex_unlock(&pool_mutex);
	
	return err;
}

err_t *
gfs_pool_new_wire(gfs_wire_t *_wire, cstr _proto, cstr _host, cstr _port)
{
	int         e;
	err_t      *err    = NULL;
	int         pid    = -1;
	int         pa[2]  = {-1, -1};
	int         pb[2]  = {-1, -1};

	e = (pipe(pa) == -1 || pipe(pb) == -1);
	if (e/*err*/) { err = err_new(-1, "Internal error: Can't create pipes"); goto cleanup; }

	pid = fork();
	if (pid == -1/*err*/) { err = err_new(-1, "Internal error: Can't fork"); goto cleanup; }

	if (pid == 0) {
		dup2(pa[0], 0);
		dup2(pb[1], 1);
		close(pa[0]); close(pa[1]);
		close(pb[0]); close(pb[1]);
		execl("/usr/bin/ncat", "/usr/bin/ncat", "--crlf", "--ssl", _host, _port, NULL);
		exit(1);
	}
	close(pa[0]); pa[0] = -1;
	close(pb[1]); pb[1] = -1;

	_wire->pid = pid; pid = -1;
	_wire->p[0] = pb[0]; pb[0] = -1;
	_wire->p[1] = pa[1]; pa[1] = -1;
	fcntl(_wire->p[0], F_SETFD, FD_CLOEXEC);
	fcntl(_wire->p[1], F_SETFD, FD_CLOEXEC);
	

	cleanup:
	if (pa[0] != -1) close(pa[0]);
	if (pa[1] != -1) close(pa[1]);
	if (pb[0] != -1) close(pb[0]);
	if (pb[1] != -1) close(pb[1]);

	return err;
}

err_t *
gfs_wire_fdopen(gfs_wire_t *_wire, FILE *_fp[2])
{
	char *errm;
	_fp[0] = fdopen(_wire->p[0], "r");
	_fp[1] = fdopen(_wire->p[1], "w");
	if (_fp[0] && _fp[1]) {
		_wire->p[0] = -1;
		_wire->p[1] = -1;
		return NULL;
	}
	errm = strerror(errno);
	if (_fp[0]) fclose(_fp[0]); else close(_wire->p[0]);
	if (_fp[1]) fclose(_fp[1]); else close(_wire->p[1]);
	_fp[0] = NULL; _wire->p[0] = -1;
	_fp[1] = NULL; _wire->p[1] = -1;
	return err_new(-1, "Internal error: Can't create file pointers: %s", errm);
}

err_t *
gfs_wire_wait(gfs_wire_t *_wire)
{
	int ret_code;
	if ((_wire->pid != -1) && (_wire->pid != 0)) {
		debugf("Waiting process %i ...", _wire->pid);
		waitpid(_wire->pid, &ret_code, 0);
		debugf("Waited %i.", _wire->pid);
		_wire->pid = -1;
		if (ret_code/*err*/) { return err_new(-1, "Connection error: Child process failed"); }
	}
	return NULL;
}

void
gfs_wire_clean(gfs_wire_t *_wire)
{
	if (_wire->p[1] != -1 && _wire->p[1] != 0) {
		close(_wire->p[1]);
	}
	if (_wire->pid  != -1 && _wire->pid  != 0) {
		kill(_wire->pid, SIGKILL);
		waitpid(_wire->pid, NULL, 0);
	}
	if (_wire->p[0] != -1 && _wire->p[0] != 0) {
		close(_wire->p[0]);
	}
	_wire->pid = _wire->p[0] = _wire->p[1] = -1;
}

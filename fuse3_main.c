#define _POSIX_C_SOURCE 200809L
#define FUSE_USE_VERSION 31

#include "err.h"
#include "cnx.h"
#include "cnx_dir.h"
#include "pool.h"
#include <fuse3/fuse.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

#define GFS_CONFIG_DIR "/etc/geminifs"

static char const HELP[] =
    "Usage: mount.gemini [OPTIONS...] DIRECTORY"                "\n"
    ""                                                          "\n"
    "Mount the Gemini network as a filesystem."                 "\n"
    ""                                                          "\n"
    "Examples:"                                                 "\n"
    ""                                                          "\n"
    "  # mount.gemini /gemini"                                  "\n"
    "  $ less /gemini/geminiquickst.art/index.gmi"              "\n"
    "  $ ls /gemini/geminiquickst.art"                          "\n"
    ""                                                          "\n"
    "  # echo geminiquickst.art > /etc/geminifs/servers"        "\n"
    "  $ ls /gemini"                                            "\n"
    "  geminiquickst.art"                                       "\n"
    ""                                                          "\n"
    "Options:"                                                  "\n"
    ""                                                          "\n"
    "  --proxy=HOSTNAME   Use a proxy server."                  "\n"
    "  --servers=FILE     A file with known servers."           "\n"
    ""                                                          "\n"
    "Configuration files: " GFS_CONFIG_DIR "/{servers,urls}"  "\n"
    ;

struct fs_options_s {
	char const *proxy;
	char const *servers;
	char const *urls;
	int         help;
};

static struct fs_options_s          gfs_opts = {0};
static const struct fuse_operations gfs_ops;

int
main(int _argc, char *_argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(_argc, _argv);

	static const struct fuse_opt args_spec[] = {
		{ "--proxy=%s",   offsetof(struct fs_options_s, proxy), 1 },
		{ "--servers=%s", offsetof(struct fs_options_s, servers), 1 },
		{ "-h",           offsetof(struct fs_options_s, help), 1 },
		{ "--help",       offsetof(struct fs_options_s, help), 1 },
		FUSE_OPT_END
	};

	if (fuse_opt_parse(&args, &gfs_opts, args_spec, NULL) == -1) {
		return 1;
	}

	if (gfs_opts.help) {
		printf(HELP);
		assert(fuse_opt_add_arg(&args, "--help") == 0);
		args.argv[0][0] = '\0';
	}

	gfs_pool_init(-1, -1, -1);
	int ret = fuse_main(args.argc, args.argv, &gfs_ops, NULL);
	fuse_opt_free_args(&args);
	return ret;
}

static void *
gfs_init(struct fuse_conn_info *_conn, struct fuse_config *_cfg)
{
	(void) _conn;
	//_cfg->direct_io = 1;
	_cfg->kernel_cache = 1;
	return NULL;
}

static int
gfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
	uri_t   uri  = {0};
	err_t  *err  = NULL;

	memset(stbuf, 0, sizeof(struct stat));
	if (!strcmp(path, "/")) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}

	err = uri_from_path(&uri, path, "gemini");
	if (err) { goto failure; }

	if (uri.is_directory) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = INT_MAX;
	}

	return 0;
	failure:
	if (err) {
		err_print(err);
		free(err);
	}
	return -ENOENT;
	(void) fi;
}

static int
gfs_readdir(
    const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi,
    enum fuse_readdir_flags flags
) {
	err_t        *err  = NULL;
	gfs_cnx_t     cnx  = {0};
	uri_t         uri  = {0};
	bool          found, start;
	char          buffer[1024*5];
	char         *name, *r;
	
	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	if (!strcmp(path, "/")) {
		FILE *fp = fopen(
		    (gfs_opts.servers)?gfs_opts.servers:GFS_CONFIG_DIR "/servers",
		    "r"
		);
		if (!fp) return 0;
		while (fgets(buffer, 256, fp)) {
			if ((r = strchr(buffer, '\r'))) *r = '\0';
			if ((r = strchr(buffer, '\n'))) *r = '\0';
			filler(buf, buffer, NULL, 0, 0);
		}
		fclose(fp);
		return 0;
	} else if (!strncmp(path, "/.Trash", sizeof("/.Trash")-1)) {
		return 0;
	}

	
	err = gfs_cnx_open(&cnx, path);
	if (err) { goto failure; }

	filler(buf, "index.gmi", NULL, 0, 0);
	for (start = true; true; start = false) {
		err = gfs_cnx_readdir(&cnx, &uri, &name, start, &found, &r, buffer, sizeof(buffer));
		if (err) { goto failure; }
		if (!found) break;
		filler(buf, name, NULL, 0, 0);
	}
	gfs_cnx_reset(&cnx, true);

	return 0;
	failure:
	gfs_cnx_reset(&cnx, true);
	if (err) {
		err_print(err);
		free(err);
	}
	return -ENOENT;
	(void) offset;
	(void) fi;
	(void) flags;
}

static int
gfs_open(const char *path, struct fuse_file_info *fi)
{
	gfs_cnx_t    *cnx  = NULL;
	err_t        *err  = NULL;
	if (!fi->fh) {
		err = gfs_cnx_open_new(&cnx, path);
		if (err) { err_print(err); free(err); return -ENOENT;}
		fi->fh = (uint64_t) cnx;
	}
	return 0;
}

static int
gfs_release(const char *_path, struct fuse_file_info *_fi)
{
	gfs_cnx_t *cnx = (gfs_cnx_t*)_fi->fh;
	if (cnx) {
		gfs_cnx_free(cnx);
		_fi->fh = 0;
	}
	return 0;
}


static int
gfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	size_t      bytes;
	err_t      *err  = NULL;
	gfs_cnx_t  *cnx  = (gfs_cnx_t*) fi->fh;

	err = gfs_cnx_read(cnx, offset, buf, size, &bytes);
	if (err) { goto failure; }

	return bytes;
	failure:
	if (err) {
		err_print(err);
		free(err);
	}
	return -ENOENT;
}

static const struct fuse_operations gfs_ops = {
	.init    = gfs_init,
	.getattr = gfs_getattr,
	.readdir = gfs_readdir,
	.open    = gfs_open,
	.read    = gfs_read,
	.release = gfs_release
};

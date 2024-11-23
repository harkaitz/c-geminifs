#include "cnx.h"
#include "cnx_dir.h"
#include <string.h>

char   buffer[5*1024*1024] = {0};
size_t buffersz = 0;

int
main(int _argc, char *_argv[])
{
	err_t        *err  = NULL;
	gfs_cnx_t     cnx  = {0};
	uri_t         uri  = {0};
	char         *name = NULL, *r;
	bool          more, start;
	int           ret  = 1;

	if (_argc <= 1) {
		printf("gmni ls|cat PATH\n");
		return 0;
	} else if (!strcmp(_argv[1], "cat") && _argv[2]) {
		gfs_cnx_reset(&cnx, true);
		err = gfs_cnx_open(&cnx, _argv[2]);
		if (err) { goto cleanup; }
		err = gfs_cnx_read(&cnx, 0, buffer, sizeof(buffer)-1, &buffersz);
		if (err) { goto cleanup; }
		fwrite(buffer, 1, buffersz, stdout);
	} else if (!strcmp(_argv[1], "ls") && _argv[2]) {
		gfs_cnx_reset(&cnx, true);
		err = gfs_cnx_open(&cnx, _argv[2]);
		if (err) { goto cleanup; }
		for (start = true, more = true; more; start = false) {
			err = gfs_cnx_readdir(
			    &cnx, &uri, &name,
			    start, &more, &r,
			    buffer, sizeof(buffer)
			);
			if (err) { goto cleanup; }
			if (more) {
				printf("==\nNAME: %s\n", (name)?name:"");
				uri_printf(&uri, stdout);
			}
		}
	} else {
		err = err_new(-1, "%s: Unknown command");
		goto cleanup;
	}
	ret = 0;
	cleanup:
	gfs_cnx_reset(&cnx, true);
	if (err) { err_print(err); }
	return ret;
}

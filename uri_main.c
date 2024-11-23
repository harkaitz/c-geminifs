#include "uri.h"
#include "err.h"

int
main(int _argc, char *_argv[])
{
	int   ret   = 1;
	err_t *err  = NULL;

	uri_t uri1 = {0}, uri2 = {0};

	if (!_argv[1]) {
		printf("geminifs_uri PATH [URL]\n");
		return 0;
	}

	err = uri_from_path(&uri1, _argv[1], "gemini");
	if (err) { goto cleanup; }

	if (!_argv[2]) {
		uri_printf(&uri1, stdout);
	} else {
		err = uri_from_url(&uri2, _argv[2], &uri1);
		if (err) { goto cleanup; }
		uri_printf(&uri2, stdout);
	}

	ret = 0;
	cleanup:
	if (err) { err_print(err); }
	return ret;
}

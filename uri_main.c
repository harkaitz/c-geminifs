#include "uri.h"

int
main(int _argc, char *_argv[])
{
	int      ret = 1;
	int      err;
	uri_t    uri1 = {0}, uri2 = {0};

	if (!_argv[1]) {
		printf("geminifs_uri PATH [URL]\n");
		return 0;
	}

	err = uri_from_path(&uri1, _argv[1], "gemini");
	if (err<0/*err*/) { goto cleanup; }

	if (!_argv[2]) {
		uri_printf(&uri1, stdout);
	} else {
		err = uri_from_url(&uri2, _argv[2], &uri1);
		if (err<0/*err*/) { goto cleanup; }
		uri_printf(&uri2, stdout);
	}

	ret = 0;
	cleanup:
	return ret;
}

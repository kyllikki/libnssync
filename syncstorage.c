#include <stdio.h>

#include "nssync.h"

#include "sync.h"

int main(int argc, char *argv[])
{
	struct nssync_sync *sync;
	enum nssync_error ret;
	struct nssync_provider provider = {
		.type = NSSYNC_SERVICE_MOZILLA,
		.params = {
			.mozilla = {
				.server = "https://auth.services.mozilla.com/",
				.account = "vince@kyllikki.org",
				.password = argv[1],
				.key = "y-4nkps-6yxav-i75xn-uv9ds-r472i",
			},
		},
	};

	ret = nssync_sync_new(&provider, &sync);
	if (ret != NSSYNC_ERROR_OK) {
		fprintf(stderr, "error (%d) creating sync\n", ret);
		return 1;
	}

	nssync_sync_free(sync);
	return 0;

}

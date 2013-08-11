#include <stdio.h>

#include <nssync/nssync.h>

int main(int argc, char *argv[])
{
	struct nssync_sync *sync;
	struct nssync_sync_bookmarks *sync_bookmarks;
	enum nssync_error ret;
	struct nssync_provider provider = {
		.type = NSSYNC_SERVICE_MOZILLA,
		.params = {
			.mozilla = {
				.server = "https://auth.services.mozilla.com/",
				.account = "vince@kyllikki.org",
				.password = argv[1],
				.key = "i-xsxyz-wd3yj-5ytjx-9i7mj-wiwyy",
			},
		},
	};

	if (argc <= 1) {
		fprintf(stderr, "Usage: %s <value>\n", argv[0]);
		return 1;
	}

	ret = nssync_sync_new(&provider, &sync);
	if (ret != NSSYNC_ERROR_OK) {
		fprintf(stderr, "error (%d) creating sync\n", ret);
		return 1;
	}


	ret = nssync_bookmarks_new(sync, &sync_bookmarks);
	if (ret != NSSYNC_ERROR_OK) {
		fprintf(stderr, "error (%d) creating bookmarks\n", ret);
		nssync_sync_free(sync);
		return 1;
	}

	nssync_bookmarks_free(sync_bookmarks);

	nssync_sync_free(sync);

	printf("PASS\n");

	return 0;

}

/*
 * Copyright 2013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnssync, http://www.netsurf-browser.org/
 *
 * Released under the Expat MIT License (see COPYING),
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <jansson.h>

#include <nssync/nssync.h>

struct nssync_sync_bookmarks {
	struct nssync_sync *sync;

	/* bookmarks collection object */
	struct nssync_storage_obj **bookmarksv;
	int bookmarksc;
};

enum nssync_error
nssync_bookmarks_new(struct nssync_sync *sync,
		     struct nssync_sync_bookmarks **sync_bookmarks)
{
	struct nssync_sync_bookmarks *newmarks;
	struct nssync_fetcher_param *fetch;

	newmarks = calloc(1, sizeof(*newmarks));
	if (newmarks == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	newmarks->sync = sync;



	return NSSYNC_ERROR_OK;
}

enum nssync_error
nssync_bookmarks_free(struct nssync_sync_bookmarks *sync_bookmarks)
{
	free(sync_bookmarks);
	return NSSYNC_ERROR_OK;
}

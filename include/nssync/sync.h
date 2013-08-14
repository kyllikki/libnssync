/*
 * This file is part of libnssync
 *
 * Copyright 20013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * Released under MIT licence (see COPYING file)
 */

#ifndef NSSYNC_SYNC_H
#define NSSYNC_SYNC_H

#include <nssync/error.h>
#include <nssync/fetcher.h>

struct nssync_sync;

/* service type to connect to */
enum nssync_provider_type {
	NSSYNC_SERVICE_MOZILLA, /* mozilla sync (aka weave) service */
};

/* service parameters */
struct nssync_provider {
	enum nssync_provider_type type;
	nssync_fetcher *fetcher;
	union {
		struct {
			const char *server;
			const char *account;
			const char *password;
			const char *key;
		} mozilla;
	} params;
};

enum nssync_error nssync_sync_new(const struct nssync_provider *provider, struct nssync_sync **sync_out);

enum nssync_error nssync_sync_free(struct nssync_sync *sync);

#endif

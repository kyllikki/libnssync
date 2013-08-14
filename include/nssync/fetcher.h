/*
 * This file is part of libnssync
 *
 * Copyright 20013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * Released under MIT licence (see COPYING file)
 */

#ifndef NSSYNC_FETCH_H
#define NSSYNC_FETCH_H

#include <stdint.h>

#include <nssync/error.h>

struct nssync_fetcher_param {
	const char *url;
	const char *username;
	const char *password;

	void *data;
	size_t data_size;

	size_t data_used;

	void *ctx;
};

typedef enum nssync_error(nssync_fetcher_cb)(struct nssync_fetcher_param *param, void *ctx);

typedef enum nssync_error(nssync_fetcher)(struct nssync_fetcher_param *param, nssync_fetcher_cb *cb, void *cb_ctx );

nssync_fetcher nssync_fetcher_curl;

#endif

/*
 * Copyright 2013 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of libnssync, http://www.netsurf-browser.org/
 *
 * Released under the Expat MIT License (see COPYING),
 *
 */

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <jansson.h>

#include <nssync/error.h>
#include <nssync/fetcher.h>
#include <nssync/debug.h>

#include "util.h"
#include "crypto.h"
#include "registration.h"
#include "storage.h"

struct nssync_storage_collection {
	char *name;
	time_t modified;
};

struct nssync_storage {
	nssync_fetcher *fetcher; /* fetcher to retrive data */

	char *username;
	char *password;

	char *base;

	int collectionc;
	struct nssync_storage_collection *collections;
};

struct nssync_storage_obj {
	char *id;
	uint8_t *payload;
	time_t modified;
	int sortindex;
	int ttl;
};

static nssync_error fetch_collections(struct nssync_storage *store)
{
	enum nssync_error ret;
	char *url;
	json_t *root;
	json_error_t error;
	const char *key;
	json_t *value;
	int colidx; /* collection index */
	struct nssync_fetcher_param fetch = {
		.username = store->username,
		.password = store->password,
		.data = NULL,
	};

	if (nssync__saprintf(&url, "%s/info/collections", store->base) < 0) {
		return -1;
	}

	fetch.url = url;
	ret = store->fetcher(&fetch, NULL, NULL);
	free(url);
	if (ret != NSSYNC_ERROR_OK) {
		return ret;
	}

	root = json_loads(fetch.data, 0, &error);
	free(fetch.data);
	store->collectionc = json_object_size(root);
	if (store->collectionc == 0) {
		fprintf(stderr, "error: root is not an object\n");
		json_decref(root);
		return -1;
	}

	store->collections = calloc(store->collectionc,
				    sizeof(struct nssync_storage_collection));
	if (store->collections == NULL) {
		return -1;
	}

	colidx = 0;
	json_object_foreach(root, key, value) {
		store->collections[colidx].name = strdup(key);
		store->collections[colidx].modified = json_real_value(value);
		colidx++;
	}

	json_decref(root);

	return 0;
}


nssync_error
nssync_storage_new(struct nssync_registration *reg,
		   const char *pathname,
		   nssync_fetcher *fetcher,
		   struct nssync_storage **store_out)
{
	char *server;
	struct nssync_storage *newstore; /* new storage service */
	const char *fmt;
	int ret;

	server = nssync_registration_get_storage_server(reg);
	if (server == NULL) {
		return -1;
	}

	newstore = calloc(1, sizeof(*newstore));
	if (newstore == NULL) {
		return -1;
	}

	newstore->fetcher = fetcher;
	newstore->username = strdup(nssync_registration_get_username(reg));
	newstore->password = strdup(nssync_registration_get_password(reg));

	/* alter the format specifier depending on separator requirements */
	if (server[strlen(server) - 1] ==  '/') {
		if ((pathname[0] == 0) ||
		    (pathname[strlen(pathname) - 1] == '/')) {
			fmt = "%s%s1.1/%s";
		} else {
			fmt = "%s%s/1.1/%s";
		}
	} else {
		if ((pathname[0] == 0) ||
		    (pathname[strlen(pathname) - 1] == '/')) {
			fmt = "%s/%s1.1/%s";
		} else {
			fmt = "%s/%s/1.1/%s";
		}
	}

	if (nssync__saprintf(&newstore->base, fmt, server, pathname,
		     newstore->username) < 0) {
		nssync_storage_free(newstore);
		return -1;
	}

	/* fetch the collection information */
	ret = fetch_collections(newstore);
	if (ret != 0) {
		nssync_storage_free(newstore);
		return ret;
	}

	*store_out = newstore;

	return NSSYNC_ERROR_OK;
}


nssync_error
nssync_storage_free(struct nssync_storage *store)
{
	free(store->base);
	free(store->username);
	free(store->password);
	free(store);

	return NSSYNC_ERROR_OK;
}


nssync_error
nssync_storage_obj_fetch(struct nssync_storage *store,
			 struct nssync_crypto_keybundle *keybundle,
			 const char *collection,
			 const char *object,
			 struct nssync_storage_obj **obj_out)
{
	enum nssync_error ret;
	struct nssync_storage_obj *obj;
	char *url;
	json_t *root;
	json_error_t error;
	json_t *payload;
	struct nssync_fetcher_param fetch = {
		.username = store->username,
		.password = store->password,
		.data = NULL,
	};

	if (nssync__saprintf(&url, "%s/storage/%s/%s", store->base,
			    collection, object) < 0) {
		return NSSYNC_ERROR_NOMEM;
	}

	debugf("requesting:%s\n", url);

	fetch.url = url;
	ret = store->fetcher(&fetch, NULL, NULL);
	free(url);
	if (ret != NSSYNC_ERROR_OK) {
		return ret;
	}

	obj = calloc(1, sizeof(*obj));
	if (obj == NULL) {
		free(fetch.data);
		return NSSYNC_ERROR_NOMEM;
	}

	root = json_loads(fetch.data, 0, &error);
	free(fetch.data);
	if (!root) {
		debugf("error: on line %d of reply: %s\n",
			error.line, error.text);
		return NSSYNC_ERROR_PROTOCOL;
	}

	if (!json_is_object(root)) {
		debugf("error: root is not an object\n");
		json_decref(root);
		return NSSYNC_ERROR_PROTOCOL;
	}

	payload = json_object_get(root, "payload");
	if (!json_is_string(payload)) {
		fprintf(stderr, "error: payload is not a string\n");
		json_decref(root);
		return NSSYNC_ERROR_PROTOCOL;
	}

	if (keybundle == NULL) {
		/* no decryption */
		obj->payload = (uint8_t*)strdup(json_string_value(payload));
		if (obj->payload == NULL) {
			json_decref(root);
			return NSSYNC_ERROR_NOMEM;
		}
	} else {
		ret = nssync_crypto_decrypt_record(json_string_value(payload),
						   keybundle,
						   &obj->payload,
						   NULL);
		if (ret != NSSYNC_ERROR_OK) {
			json_decref(root);
			return ret;
		}
	}

	json_decref(root);

	*obj_out = obj;

	return NSSYNC_ERROR_OK;
}

int
nssync_storage_obj_free(struct nssync_storage_obj *obj)
{
	if (obj->payload) {
		free(obj->payload);
	}
	free(obj);
	return 0;
}

uint8_t *
nssync_storage_obj_payload(struct nssync_storage_obj *obj)
{
	return obj->payload;
}

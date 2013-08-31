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
#include "registration.h"
#include "storage.h"

/* container for object */
struct nssync_storage_obj {
	char *id;
	char *payload;
	time_t modified;
	int sortindex;
	int ttl;
};

/* container for collection */
struct nssync_storage_collection {
	char *name;
	time_t modified;

	unsigned int objc; /* number of objects in collection */
	struct nssync_storage_obj **objv; /* list of objects within collection */
};

/* storage server */
struct nssync_storage {
	nssync_fetcher *fetcher; /* fetcher to retrive data */

	char *username;
	char *password;

	char *base;

	int collectionc;
	struct nssync_storage_collection *collections;
};

/** fetch the list of collections available on a storage server */
static nssync_error fetch_collections(struct nssync_storage *store)
{
	enum nssync_error ret;
	json_t *root;
	json_error_t error;
	const char *key;
	json_t *value;
	int colidx; /* collection index */
	struct nssync_fetcher_fetch fetch = {
		.username = store->username,
		.password = store->password,
	};

	if (nssync__saprintf(&fetch.url, "%s/info/collections", store->base) < 0) {
		return -1;
	}

	ret = store->fetcher(&fetch);
	free(fetch.url);
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

/* exported interface documented in storage.h */
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
		return NSSYNC_ERROR_NOMEM;
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

/* exported interface documented in storage.h */
nssync_error
nssync_storage_free(struct nssync_storage *store)
{
	free(store->base);
	free(store->username);
	free(store->password);
	free(store);

	return NSSYNC_ERROR_OK;
}


/* exported interface documented in storage.h */
nssync_error
nssync_storage_obj_fetch(struct nssync_storage *store,
			 const char *collection,
			 const char *object,
			 struct nssync_storage_obj **obj_out)
{
	enum nssync_error ret;
	struct nssync_storage_obj *obj;
	char *url;
	json_t *root;
	json_error_t error;
	json_t *value;

	struct nssync_fetcher_fetch fetch = {
		.username = store->username,
		.password = store->password,
		.data = NULL,
	};

	/* build object url */
	if (nssync__saprintf(&url, "%s/storage/%s/%s", store->base,
			    collection, object) < 0) {
		return NSSYNC_ERROR_NOMEM;
	}

	/* issue fetch for ojbect */
	fetch.url = url;
	ret = store->fetcher(&fetch);
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

	/* id string */
	value = json_object_get(root, "id");
	if (!json_is_string(value)) {
		debugf("error: id is not a string\n");
		json_decref(root);
		return NSSYNC_ERROR_PROTOCOL;
	}

	obj->id = strdup(json_string_value(value));
	if (obj->id == NULL) {
		json_decref(root);
		return NSSYNC_ERROR_NOMEM;
	}

	/* payload string */
	value = json_object_get(root, "payload");
	if (!json_is_string(value)) {
		debugf("error: payload is not a string\n");
		json_decref(root);
		return NSSYNC_ERROR_PROTOCOL;
	}

	obj->payload = strdup(json_string_value(value));
	if (obj->payload == NULL) {
		json_decref(root);
		return NSSYNC_ERROR_NOMEM;
	}

	/* modified time */
	value = json_object_get(root, "payload");
	if (json_is_real(value)) {
		obj->modified = json_real_value(value);
	}

	/* ttl integer */
	value = json_object_get(root, "ttl");
	if (json_is_integer(value)) {
		obj->ttl = json_integer_value(value);
	}

	/* sortindex integer */
	value = json_object_get(root, "sortindex");
	if (json_is_integer(value)) {
		obj->sortindex = json_integer_value(value);
	}

	json_decref(root);

	*obj_out = obj;

	return NSSYNC_ERROR_OK;
}

struct collection_fetch {
	struct nssync_fetcher_fetch fetch;
	struct nssync_storage_obj ***pobjv;
	int *pobjc;
};

static nssync_error
nssync_storage_collection_fetch_complete(struct nssync_fetcher_fetch *fetch)
{
	struct collection_fetch *cfetch = (struct collection_fetch *)fetch;
	nssync_error ret;
	json_t *root;
	json_error_t error;

	ret = cfetch->fetch.result;
	if (ret != NSSYNC_ERROR_OK) {
		goto fetch_error;
	}

	root = json_loads(cfetch->fetch.data, 0, &error);
	if (!root) {
		debugf("error: on line %d of reply: %s\n",
			error.line, error.text);
		ret = NSSYNC_ERROR_PROTOCOL;
		goto fetch_error;
	}

	if (!json_is_object(root)) {
		debugf("error: root is not an object\n");
		json_decref(root);
		ret = NSSYNC_ERROR_PROTOCOL;
		goto fetch_error;
	}

#if 0
	enum nssync_error ret;
	struct nssync_storage_obj *obj;
	json_t *root;
	json_error_t error;
	json_t *payload;
	obj = calloc(1, sizeof(*obj));
	if (obj == NULL) {
		free(fetch.data);
		return NSSYNC_ERROR_NOMEM;
	}

	payload = json_object_get(root, "payload");
	if (!json_is_string(payload)) {
		fprintf(stderr, "error: payload is not a string\n");
		json_decref(root);
		return NSSYNC_ERROR_PROTOCOL;
	}

		obj->payload = (uint8_t*)strdup(json_string_value(payload));
		if (obj->payload == NULL) {
			json_decref(root);
			return NSSYNC_ERROR_NOMEM;
		}

	json_decref(root);

	*obj_out = obj;

	ret = NSSYNC_ERROR_OK;
#else
	ret = NSSYNC_ERROR_PROTOCOL;
#endif

fetch_error:

	free(cfetch->fetch.data);
	free(cfetch->fetch.url);
	free(cfetch);

	return ret;
}

/* exported interface documented in storage.h */
nssync_error
nssync_storage_collection_fetch_async(struct nssync_storage *store,
				      const char *collection,
				      struct nssync_storage_obj ***objv_out,
				      int *objc_out)
{
	struct collection_fetch *cfetch;

	/* setup the fetch */
	cfetch = calloc(1, sizeof(*cfetch));
	if (cfetch == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	cfetch->pobjv = objv_out;
	cfetch->pobjc = objc_out;

	if (nssync__saprintf(&cfetch->fetch.url,
			     "%s/storage/%s?full=1",
			     store->base, collection) < 0) {
		free(cfetch);
		return NSSYNC_ERROR_NOMEM;
	}

	cfetch->fetch.username = store->username;
	cfetch->fetch.password = store->password;
	cfetch->fetch.completion = nssync_storage_collection_fetch_complete;

	return store->fetcher(&cfetch->fetch);
}

nssync_error
nssync_storage_collection_enum(struct nssync_storage *store,
			       const char *collection,
			       struct nssync_storage_obj **obj_out)
{
	return NSSYNC_ERROR_PROTOCOL;
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

char *
nssync_storage_obj_payload(struct nssync_storage_obj *obj)
{
	return obj->payload;
}

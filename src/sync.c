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

#include "crypto.h"
#include "registration.h"
#include "storage.h"

/* supported storage version */
#define STORAGE_VERSION 5

struct nssync_sync_engine {
	char *name;
	int version;
	char *syncid;
};

struct nssync_sync {
	struct nssync_registration *reg;
	struct nssync_storage *store;
	struct nssync_crypto_keybundle *sync_keybundle;

	struct nssync_storage_obj *metaglobal_obj;
	char *metaglobal_syncid;

	int enginec;
	struct nssync_sync_engine *engines;

	struct nssync_storage_obj *cryptokeys_obj;
	struct nssync_crypto_keybundle *default_keybundle;
};

/* extract the data engine list from json object
 *
 * engines in the sync protocol are the versioning infomation for how
 * to interpret the objects held in each collection type.
 */
static enum nssync_error
get_engines(json_t *enginej,
	    int *enginec_out,
	    struct nssync_sync_engine **engines_out)
{
	json_t *engine;
	json_t *value;
	const char *key;

	int enginec;
	struct nssync_sync_engine *engines;
	int engidx;

	enginec = json_object_size(enginej);
	if ((!json_is_object(enginej)) ||
	    (enginec == 0)) {
		/* no engines is valid (if useless) */
		*enginec_out = 0;
		*engines_out = NULL;
		return NSSYNC_ERROR_OK;
	}

	engines = calloc(enginec, sizeof(struct nssync_sync_engine));
	if (engines == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	engidx = 0;
	json_object_foreach(enginej, key, engine) {
		engines[engidx].name = strdup(key);
		value = json_object_get(engine, "version");
		engines[engidx].version = json_integer_value(value);
		value = json_object_get(engine, "syncID");
		engines[engidx].syncid = strdup(json_string_value(value));

		debugf("%s: %d %s\n", engines[engidx].name, engines[engidx].version, engines[engidx].syncid);
		engidx++;
	}

	*enginec_out = enginec;
	*engines_out = engines;
	return NSSYNC_ERROR_OK;
}

/** fetch and verify the metaglobal object
 *
 */
static enum nssync_error meta_global(struct nssync_sync *sync)
{
	enum nssync_error ret;

	json_t *value;
	json_t *root;
	json_error_t error;

	ret = nssync_storage_obj_fetch(sync->store,
				       "meta", "global",
				       &sync->metaglobal_obj);
	if (ret != 0) {
		debugf("unable to retrive metaglobal object\n");
		return ret;
	}

	root = json_loads((char *)nssync_storage_obj_payload(sync->metaglobal_obj), 0, &error);

	if (!root) {
		debugf("error: on line %d: %s\n", error.line, error.text);
		return NSSYNC_ERROR_PROTOCOL;
	}

	if (!json_is_object(root)) {
		debugf("error: root is not an object\n");
		return NSSYNC_ERROR_PROTOCOL;
	}

	/* check storage version */
	value = json_object_get(root, "storageVersion");
	if (json_integer_value(value) != STORAGE_VERSION) {
		return NSSYNC_ERROR_VERSION;
	}

	/* retrive syncid */
	value = json_object_get(root, "syncID");
	sync->metaglobal_syncid = strdup(json_string_value(value));

	/* retrive data engines */
	ret = get_engines(json_object_get(root, "engines"),
			  &sync->enginec, &sync->engines);
	if (ret != 0) {
		debugf("error retriving engine data\n");
		return ret;
	}

	json_decref(root);

	return NSSYNC_ERROR_OK;
}


/** fetch and verify the cryptokeys object
 *
 */
static enum nssync_error crypto_keys(struct nssync_sync *sync)
{
	enum nssync_error ret;
	struct nssync_storage_obj *cryptokeys_obj;
	char *record;

	json_t *root;
	json_error_t error;

	json_t *value;
	json_t *default_key;
	json_t *default_hmac;

	/* get crypto/keys object from storage */
	ret = nssync_storage_obj_fetch(sync->store,
				       "crypto", "keys",
				       &cryptokeys_obj);
	if (ret != 0) {
		debugf("unable to retrive crypto/keys object\n");
		return ret;
	}

	/* decrypt record */
	ret = nssync_crypto_decrypt_record(nssync_storage_obj_payload(cryptokeys_obj),
					   sync->sync_keybundle,
					   (uint8_t **)&record,
					   NULL);
	if (ret != NSSYNC_ERROR_OK) {
		return ret;
	}

	/* process decrypted record */
	root = json_loads(record, JSON_DISABLE_EOF_CHECK, &error);
	free(record);
	if (!root) {
		debugf("error: on line %d: %s\n", error.line, error.text);
		free(cryptokeys_obj);
		return NSSYNC_ERROR_PROTOCOL;
	}

	if (!json_is_object(root)) {
		debugf("error: root is not an object\n");
		free(cryptokeys_obj);
		return NSSYNC_ERROR_PROTOCOL;
	}

	/* default keybundle */
	value = json_object_get(root, "default");
	if (!json_is_array(value)) {
		free(cryptokeys_obj);
		return NSSYNC_ERROR_VERSION;
	}

	default_key = json_array_get(value, 0);
	default_hmac = json_array_get(value, 1);

	ret = nssync_crypto_keybundle_new_b64(json_string_value(default_key),
					      json_string_value(default_hmac),
					      &sync->default_keybundle);

	/** @todo extract the keys from the "collections" object */

	json_decref(root);

	sync->cryptokeys_obj = cryptokeys_obj;

	return NSSYNC_ERROR_OK;
}

enum nssync_error
nssync_sync_new(const struct nssync_provider *provider,
		struct nssync_sync **sync_out)
{
	enum nssync_error ret;
	struct nssync_sync *newsync;
	nssync_fetcher *fetcher; /* fetcher to retrive data */

	if (provider->type != NSSYNC_SERVICE_MOZILLA) {
		return NSSYNC_ERROR_INVAL;
	}

	newsync = calloc(1, sizeof(*newsync));
	if (newsync == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	/* setup the fetcher to call */
	if (provider->fetcher == NULL) {
		fetcher = nssync_fetcher_curl;
	} else {
		fetcher = provider->fetcher;
	}

	/* create registration from parameters */
	ret = nssync_registration_new(provider->params.mozilla.server,
				      provider->params.mozilla.account,
				      provider->params.mozilla.password,
				      fetcher,
				      &newsync->reg);
	if (ret != NSSYNC_ERROR_OK) {
		return NSSYNC_ERROR_REGISTRATION;
	}

	/* create sync key bundle */
	ret = nssync_crypto_keybundle_new_user_synckey(provider->params.mozilla.key,
				nssync_registration_get_username(newsync->reg),
				&newsync->sync_keybundle);
	if (ret != NSSYNC_ERROR_OK) {
		debugf("unable to create sync key: %d\n", ret);
		nssync_registration_free(newsync->reg);
		free(newsync);
		return ret;
	}

	/* create data store connection using reg data */
	ret = nssync_storage_new(newsync->reg, "", fetcher, &newsync->store);
	if (ret != NSSYNC_ERROR_OK) {
		debugf("unable to create store: %d\n", ret);
		nssync_registration_free(newsync->reg);
		free(newsync->sync_keybundle);
		free(newsync);
		return ret;
	}

	ret = meta_global(newsync);
	if (ret != NSSYNC_ERROR_OK) {
		debugf("error with meta/global object: %d\n", ret);
		nssync_sync_free(newsync);
		return ret;
	}

	ret = crypto_keys(newsync);
	if (ret != NSSYNC_ERROR_OK) {
		debugf("error with crypto/keys object: %d\n", ret);
		nssync_sync_free(newsync);
		return ret;
	}

	*sync_out = newsync;

	return NSSYNC_ERROR_OK;
}

enum nssync_error
nssync_sync_free(struct nssync_sync *sync)
{
	nssync_storage_obj_free(sync->metaglobal_obj);
	nssync_storage_free(sync->store);
	nssync_registration_free(sync->reg);

	free(sync);

	return  NSSYNC_ERROR_OK;
}
#if 0
nssync_error
nssync_sync_collection_open(struct nssync_sync *sync, const char *collection)
{
	nssync_error ret;
	struct nssync_crypto_keybundle *keybundle;

	ret = nssync_storage_collection_fetch_async(sync->store,
				      keybundle,
				      collection,
				      struct nssync_storage_obj ***objv_out,
				      int *objc_out);

}
#endif

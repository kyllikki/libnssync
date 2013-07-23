#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <jansson.h>

#include "nssync.h"

#include "request.h"
#include "auth.h"
#include "storage.h"
#include "sync.h"

/* supported storage version */
#define STORAGE_VERSION 5

struct nssync_sync_engine {
	char *name;
	int version;
	char *syncid;
};

struct nssync_sync {
	struct nssync_auth *auth;
	struct nssync_storage *store;

	struct nssync_storage_obj *metaglobal_obj;
	char *metaglobal_syncid;

	int enginec;
	struct nssync_sync_engine *engines;
};

/* extract the data engine list from json object
 *
 * engines in the sync protocol are the versioning infomation for how
 * to interpret the objects held in each collection type.
 */
static enum nnsync_error
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

enum nnsync_error
nssync_sync_new(const struct nssync_provider *provider,
		struct nssync_sync **sync_out)
{
	enum nnsync_error ret;
	struct nssync_sync *newsync;

	json_t *value;
	json_t *root;
	json_error_t error;

	if (provider->type != NSSYNC_SERVICE_MOZILLA) {
		return NSSYNC_ERROR_INVAL;
	}

	newsync = calloc(1, sizeof(*newsync));
	if (newsync == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	/* create auth info from parameters */
	ret = nssync_auth_new(provider->params.mozilla.server,
			      provider->params.mozilla.account,
			      provider->params.mozilla.password,
			      &newsync->auth);
	if (ret != NSSYNC_ERROR_OK) {
		return NSSYNC_ERROR_AUTH;
	}

	/* create data store connection using auth data */
	ret = nssync_storage_new(newsync->auth, "", &newsync->store);
	if (ret != NSSYNC_ERROR_OK) {
		fprintf(stderr, "unable to create store: %d\n", ret);
		nssync_auth_free(newsync->auth);
		free(newsync);
		return ret;
	}

	/* fetch the metaglobal object and check values */
	ret = nssync_storage_obj_fetch(newsync->store,
				       "meta", "global",
				       &newsync->metaglobal_obj);
	if (ret != 0) {
		debugf("unable to retrive metaglobal object\n");
		nssync_sync_free(newsync);
		return ret;
	}

	root = json_loads(nssync_storage_obj_payload(newsync->metaglobal_obj),
			  0, &error);

	if (!root) {
		fprintf(stderr, "error: on line %d: %s\n",
			error.line, error.text);
		nssync_sync_free(newsync);
		return NSSYNC_ERROR_PROTOCOL;
	}

	if (!json_is_object(root)) {
		fprintf(stderr, "error: root is not an object\n");
		nssync_sync_free(newsync);
		return NSSYNC_ERROR_PROTOCOL;
	}

	/* cheack storage version */
	value = json_object_get(root, "storageVersion");
	if (json_integer_value(value) != STORAGE_VERSION) {
		nssync_sync_free(newsync);
		return NSSYNC_ERROR_VERSION;
	}

	/* retrive syncid */
	value = json_object_get(root, "syncID");
	newsync->metaglobal_syncid = strdup(json_string_value(value));

	/* retrive data engines */
	ret = get_engines(json_object_get(root, "engines"),
			  &newsync->enginec, &newsync->engines);
	if (ret != 0) {
		debugf("error retriving engine data\n");
		nssync_sync_free(newsync);
		return ret;
	}

	json_decref(root);

	*sync_out = newsync;

	return NSSYNC_ERROR_OK;
}

enum nnsync_error
nssync_sync_free(struct nssync_sync *sync)
{
	nssync_storage_obj_free(sync->metaglobal_obj);
	nssync_storage_free(sync->store);
	nssync_auth_free(sync->auth);
	free(sync);
	return  NSSYNC_ERROR_OK;
}

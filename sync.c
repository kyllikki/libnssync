#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <jansson.h>

#include "request.h"

#include "nssync_error.h"
#include "auth.h"
#include "storage.h"
#include "sync.h"

/* supported storage version */
#define STORAGE_VERSION 5

struct nssync_sync {
	struct nssync_auth *auth;
	struct nssync_storage *store;

	struct nssync_storage_obj *metaglobal_obj;
	char *metaglobal_syncid;
};

enum nnsync_error
nssync_sync_new(const struct nssync_provider *provider,
		struct nssync_sync **sync_out)
{
	struct nssync_sync *newsync;

	json_t *root;
	json_t *value;
	json_error_t error;
	enum nnsync_error ret;

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
		fprintf(stderr, "unable to retrive metaglobal object\n");
		nssync_storage_free(newsync->store);
		nssync_auth_free(newsync->auth);
		free(newsync);
		return ret;
	}

	root = json_loads(nssync_storage_obj_payload(newsync->metaglobal_obj),
			  0, &newsync->metaglobal_obj);

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


	json_decref(root);

	*sync_out = newsync;

	return NSSYNC_ERROR_OK;

#if 0
	const char *key;
	json_t *value;

	json_object_foreach(root, key, value) {
		if (json_is_object(value)) {
			printf("%s(object):%p\n", key,
			       value);
		} else if (json_is_array(value)) {
			printf("%s(array):%p\n", key,
			       value);
		} else if (json_is_string(value)) {
			printf("%s(string):%s\n", key,
			       json_string_value(value));
		}
	}

	json_t *payload;
	payload = json_object_get(root, "payload");
	if (!json_is_object(payload)) {
		fprintf(stderr, "error: payload is not an object\n");
		return 1;
	}



	json_object_foreach(payload, key, value) {
		if (json_is_string(value)) {
			printf("%s:%s\n", key, json_string_value(value));
		}
	}

#endif

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

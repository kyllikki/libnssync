#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <jansson.h>

#include "request.h"

#include "auth.h"
#include "storage.h"

#define AUTH_SERVER "https://auth.services.mozilla.com/"
#define ACCOUNT_NAME "vince@kyllikki.org"

int main(int argc, char *argv[])
{
	json_t *root;
	json_error_t error;
	int ret;

	struct nssync_auth *auth;
	struct nssync_storage *store;
	struct nssync_storage_obj *obj;

	auth = nssync_auth_new(AUTH_SERVER, ACCOUNT_NAME, argv[1]);

	ret = nssync_storage_new(auth, "", &store);
	if (ret != 0) {
		fprintf(stderr, "unable to create storage object: %d\n", ret);
		return 1;
	}

	ret = nssync_storage_obj_fetch(store, "meta", "global", &obj);
	if (ret != 0) {
		fprintf(stderr, "unable to retrive global object\n");
		return 1;
	}

	root = json_loads(nssync_storage_obj_payload(obj), 0, &error);

	nssync_storage_obj_free(obj);

	if (!root) {
		fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
		return 1;
	}

	if(!json_is_object(root)) {
		fprintf(stderr, "error: root is not an object\n");
		return 1;
	}


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

	json_decref(root);
	return 0;

}

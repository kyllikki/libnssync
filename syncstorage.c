/*
 * Copyright (c) 2009-2012 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

#include <openssl/sha.h>
#include <jansson.h>

#include "base32.h"
#include "request.h"
#include "storage.h"

#define URL_SIZE     256

#define AUTH_SERVER "https://auth.services.mozilla.com/"
#define AUTH_PATH "%suser/1.0/%s/node/weave"
#define STORAGE_PATH "%s1.1/%s/storage/%s?full=1"
#define ACCOUNT_NAME "vince@kyllikki.org"
#define ACCOUNT_PASSWORD "insecure"

static bool isvalidusername(const char *s)
{
	const char *match = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-";

	while (*s != 0) {
		if (strchr(match, *s) == NULL) {
			return false;
		}
		s++;
	}
	return true;
}

static char *strduptolower(const char *s) {
	char *ret;
	int sloop = 0;

	ret = malloc(strlen(s) + 1);
	if (ret != NULL) {
		while (s[sloop] != 0) {
			ret[sloop] = tolower(s[sloop]);
			sloop++;
		}
		ret[sloop] = 0;
	}
	return ret;
}

/** create a username from an account name
 *
 * The requirements for username are not explained anywhere except
 * except in the source code at
 * https://hg.mozilla.org/mozilla-central/file/2cfff9240e9a/services/sync/modules/identity.js#l422
 *
 */
char *moz_sync_username_from_accountname(const char *accountname)
{
	char *username;
	SHA_CTX context;
	uint8_t digest[20];
	size_t bufflen = 80;

	if (isvalidusername(accountname)) {
		return strduptolower(accountname);
	}

	SHA1_Init(&context);
	SHA1_Update(&context, accountname, strlen(accountname));
	SHA1_Final(digest, &context);

	username = calloc(1, bufflen);
	if (username != NULL) {
		base32_encode(username, &bufflen, digest, sizeof(digest));
	}

	return username;
}




int main(int argc, char *argv[])
{
	char url[URL_SIZE];
	char *username;
	char *storage_server;
	json_t *root;
	json_error_t error;

	struct nssync_storage *store;
	struct nssync_storage_obj *obj;

	username = moz_sync_username_from_accountname(ACCOUNT_NAME);


	snprintf(url, URL_SIZE, AUTH_PATH, AUTH_SERVER, username);

	storage_server = nssync__request(url, NULL, NULL);
	if (storage_server == NULL)
		return 1;
	printf("%s\n\n", storage_server);


	store = nssync_storage_new(storage_server, "", username, ACCOUNT_PASSWORD); 

	
	obj = nssync_storage_obj_fetch(store, "storage/meta/global");

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



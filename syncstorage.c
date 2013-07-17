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
#include <curl/curl.h>

#include "base32.h"

#define BUFFER_SIZE  (256 * 1024)  /* 256 KB */

#define URL_SIZE     256

#define AUTH_SERVER "https://auth.services.mozilla.com/"
#define AUTH_PATH "%suser/1.0/%s/node/weave"
#define STORAGE_PATH "%s1.1/${USERID}/storage/%s?full=1"
#define ACCOUNT_NAME "vince@kyllikki.org"
#define PASSWORD "insecure"

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



/* Return the offset of the first newline in text or the length of
   text if there's no newline */
static int newline_offset(const char *text)
{
	const char *newline = strchr(text, '\n');
	if(!newline)
		return strlen(text);
	else
		return (int)(newline - text);
}

struct write_result
{
	char *data;
	int pos;
};

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct write_result *result = (struct write_result *)stream;

	if(result->pos + size * nmemb >= BUFFER_SIZE - 1)
	{
		fprintf(stderr, "error: too small buffer\n");
		return 0;
	}

	memcpy(result->data + result->pos, ptr, size * nmemb);
	result->pos += size * nmemb;

	return size * nmemb;
}

static char *request(const char *url)
{
	CURL *curl;
	CURLcode status;
	char *data;
	long code;

	curl = curl_easy_init();
	data = malloc(BUFFER_SIZE);
	if(!curl || !data)
		return NULL;

	struct write_result write_result = {
		.data = data,
		.pos = 0
	};

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

	status = curl_easy_perform(curl);
	if(status != 0)
	{
		fprintf(stderr, "error: unable to request data from %s:\n", url);
		fprintf(stderr, "%s\n", curl_easy_strerror(status));
		return NULL;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if(code != 200)
	{
		fprintf(stderr, "error: server responded with code %ld\n", code);
		return NULL;
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	/* zero-terminate the result */
	data[write_result.pos] = '\0';

	return data;
}


int main(int argc, char *argv[])
{
	size_t i;
	char *text;
	char url[URL_SIZE];
	char *username;

	json_t *root;
	json_error_t error;

	username = moz_sync_username_from_accountname(ACCOUNT_NAME);

	snprintf(url, URL_SIZE, AUTH_PATH, AUTH_SERVER, username);

	text = request(url);
	if (text == NULL)
		return 1;
	printf("%s\n", text);
}


#if 0
int main(int argc, char *argv[])
{
	size_t i;
	char *text;
	char url[URL_SIZE];

	json_t *root;
	json_error_t error;

	if(argc != 3)
	{
		fprintf(stderr, "usage: %s USER REPOSITORY\n\n", argv[0]);
		fprintf(stderr, "List commits at USER's REPOSITORY.\n\n");
		return 2;
	}

	snprintf(url, URL_SIZE, URL_FORMAT, argv[1], argv[2]);

	text = request(url);
	if(!text)
		return 1;

	root = json_loads(text, 0, &error);
	free(text);

	if(!root)
	{
		fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
		return 1;
	}

	if(!json_is_array(root))
	{
		fprintf(stderr, "error: root is not an array\n");
		return 1;
	}

	for(i = 0; i < json_array_size(root); i++)
	{
		json_t *data, *sha, *commit, *message;
		const char *message_text;

		data = json_array_get(root, i);
		if(!json_is_object(data))
		{
			fprintf(stderr, "error: commit data %d is not an object\n", i + 1);
			return 1;
		}

		sha = json_object_get(data, "sha");
		if(!json_is_string(sha))
		{
			fprintf(stderr, "error: commit %d: sha is not a string\n", i + 1);
			return 1;
		}

		commit = json_object_get(data, "commit");
		if(!json_is_object(commit))
		{
			fprintf(stderr, "error: commit %d: commit is not an object\n", i + 1);
			return 1;
		}

		message = json_object_get(commit, "message");
		if(!json_is_string(message))
		{
			fprintf(stderr, "error: commit %d: message is not a string\n", i + 1);
			return 1;
		}

		message_text = json_string_value(message);
		printf("%.8s %.*s\n",
		       json_string_value(sha),
		       newline_offset(message_text),
		       message_text);
	}

	json_decref(root);
	return 0;
}
#endif

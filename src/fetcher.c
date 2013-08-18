#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

#include <nssync/fetcher.h>
#include <nssync/debug.h>

#define BUFFER_SIZE  (256 * 1024)  /* 256 KB */


static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct nssync_fetcher_fetch *fetch = stream;

	if ((fetch->data_used + size * nmemb) >= (fetch->data_size - 1)) {
		fprintf(stderr, "error: too small buffer\n");
		return 0;
	}

	memcpy(((uint8_t *)fetch->data) + fetch->data_used, ptr, size * nmemb);
	fetch->data_used += size * nmemb;

	return size * nmemb;
}


enum nssync_error
nssync_fetcher_curl(struct nssync_fetcher_fetch *fetch)
{
	CURL *curl;
	CURLcode status;
	long code;

	debugf("fetching:%s\n", fetch->url);

	if (fetch->data == NULL) {
		fetch->data_size = BUFFER_SIZE;
		fetch->data = malloc(fetch->data_size);
	}

	if (fetch->data == NULL) {
		return NSSYNC_ERROR_NOMEM;
	}

	curl = curl_easy_init();

	if (!curl) {
		return NSSYNC_ERROR_FETCH;
	}

	curl_easy_setopt(curl, CURLOPT_URL, fetch->url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fetch);

	if (fetch->username != NULL) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, fetch->username);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, fetch->password);
	}

	status = curl_easy_perform(curl);
	if (status != 0) {
		fprintf(stderr, "error: unable to request data from %s:\n", fetch->url);
		fprintf(stderr, "%s\n", curl_easy_strerror(status));
		curl_easy_cleanup(curl);
		return NSSYNC_ERROR_FETCH;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if (code != 200) {
		fprintf(stderr, "error: server responded with code %ld\n", code);
		curl_easy_cleanup(curl);
		return NSSYNC_ERROR_FETCH;
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	/* zero-terminate the result */
	((uint8_t *)fetch->data)[fetch->data_used] = '\0';

	/* call the callback */
	if (fetch->completion != NULL) {
		return fetch->completion(fetch);
	}

	return NSSYNC_ERROR_OK;
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

#include <nssync/fetcher.h>

#define BUFFER_SIZE  (256 * 1024)  /* 256 KB */

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


static char *
nssync__request(const char *url, const char *username, const char *password)
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

	if (username != NULL) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, username);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
	}

	status = curl_easy_perform(curl);
	if(status != 0)
	{
		fprintf(stderr, "error: unable to request data from %s:\n", url);
		fprintf(stderr, "%s\n", curl_easy_strerror(status));
		return NULL;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if (code != 200) {
		fprintf(stderr, "error: server responded with code %ld\n", code);
		return NULL;
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	/* zero-terminate the result */
	data[write_result.pos] = '\0';

	return data;
}

enum nssync_error 
nssync_fetcher_curl(struct nssync_fetcher_param *param, 
		    nssync_fetcher_cb *cb, 
		    void *cb_ctx )
{

	param->data = nssync__request(param->url, 
				      param->username, 
				      param->password);

	if (param->data == NULL) {
		return NSSYNC_ERROR_FETCH;
	}
		
	return NSSYNC_ERROR_OK;
}

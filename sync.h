#include "nssync_error.h"

struct nssync_sync;

/* service type to connect to */
enum nssync_provider_type {
	NSSYNC_SERVICE_MOZILLA, /* mozilla sync (aka weave) service */
};

struct nssync_provider {
	enum nssync_provider_type type;
	union {
		struct {
			const char *server;
			const char *account;
			const char *password;
			const char *key;
		} mozilla; 
	} params;
};

enum nnsync_error nssync_sync_new(const struct nssync_provider *provider, struct nssync_sync **sync_out);

enum nnsync_error nssync_sync_free(struct nssync_sync *sync);

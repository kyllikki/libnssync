struct nssync_auth;

struct nssync_auth *nssync_auth_new(const char *server, const char *account, const char *password);

int nssync_auth_free(struct nssync_auth *auth);

char *nssync_auth_get_storage_server(struct nssync_auth *auth);
char *nssync_auth_get_username(struct nssync_auth *auth);
char *nssync_auth_get_password(struct nssync_auth *auth);

